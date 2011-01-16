/**
    @file

	Pool of reusuable SFTP connections.

    @if license

    Copyright (C) 2007, 2008, 2009, 2010, 2011
    Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    @endif
*/

#include "Pool.h"

#include "swish/remotelimits.h" // Text field limits
#include "swish/utils.hpp" // running_object_table
#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer

#include <winapi/com/object.hpp> // object_from_moniker_name

#include <comet/error.h> // com_error
#include <comet/interface.h> // uuidof, comtype

#include <boost/lexical_cast.hpp> // lexical_cast
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cstring> // memset

using winapi::com::create_bind_context;
using winapi::com::object_from_moniker_name;

using comet::com_error;
using comet::com_ptr;
using comet::critical_section;
using comet::auto_cs;
using comet::uuidof;

using boost::lexical_cast;

using std::wstring;


template<> struct comet::comtype<IBindStatusCallback>
{
	static const IID& uuid() throw() { return IID_IBindStatusCallback; }
	typedef IUnknown base;
};

namespace {

	/**
	 * Create an moniker string for the session with the given parameters.
	 *
	 * e.g. clsid:b816a864-5022-11dc-9153-0090f5284f85:!user@host:port
	 */
	wstring provider_moniker_name(
		const wstring& user, const wstring& host, int port)
	{
		wstring item_name = 
			L"clsid:b816a864-5022-11dc-9153-0090f5284f85:!" + user + L'@' + 
			host + L':' + lexical_cast<wstring>(port);
		return item_name;
	}

}

critical_section CPool::m_cs;

namespace {

/**
 * UI-supressing bind status callback.
 */
class CBindCallbackStub : public comet::simple_object<IBindStatusCallback>
{
public:
	IFACEMETHODIMP OnStartBinding(DWORD /*dwReserved*/, IBinding* /*pib*/)
	{ return S_OK; }
	
	IFACEMETHODIMP GetPriority(LONG* /*pnPriority*/)
	{ return E_NOTIMPL; }
	
	IFACEMETHODIMP OnLowResource(DWORD /*reserved*/)
	{ return E_NOTIMPL; }
	
	IFACEMETHODIMP OnProgress(
		ULONG /*ulProgress*/, ULONG /*ulProgressMax*/, ULONG /*ulStatusCode*/,
		LPCWSTR /*szStatusText*/)
	{ return S_OK; }
	
	IFACEMETHODIMP OnStopBinding(HRESULT /*hresult*/, LPCWSTR /*szError*/)
	{ return S_OK; }
	
	IFACEMETHODIMP GetBindInfo(DWORD* grfBINDF, BINDINFO* /*pbindinfo*/)
	{
		*grfBINDF = BINDF_NO_UI | BINDF_SILENTOPERATION;
		return S_OK;
	}
	
	IFACEMETHODIMP OnDataAvailable(
		DWORD /*grfBSCF*/, DWORD /*dwSize*/, FORMATETC* /*pformatetc*/,
		STGMEDIUM* /*pstgmed*/)
	{ return S_OK; };
	
	IFACEMETHODIMP OnObjectAvailable(REFIID /*riid*/, IUnknown* /*punk*/)
	{ return S_OK; };	
};

}

/**
 * Retrieves an SFTP session for a global pool or creates it if none exists.
 *
 * Pointers to the session objects are stored in the Running Object Table (ROT)
 * making them available to any client that needs one under the same 
 * Winstation (login).  They are identified by item monikers of the form 
 * "!username@hostname:port".
 *
 * If an existing session can't be found in the ROT (as will happen the first
 * a connection is made) this function creates a new (Provider) 
 * connection with the given parameters.  In the future this may be extended to
 * give a choice of the type of connection to make.
 *
 * @param hwnd  Owner Window for any elevation dialogues.  If NULL, Windows
 *              will call GetActiveWindow in order to find a suitable owner.
 *              This may cause problems with focus.
 *
 * @throws AtlException if any error occurs.
 *
 * @returns pointer to the session (ISftpProvider).
 */
com_ptr<ISftpProvider> CPool::GetSession(
	const wstring& host, const wstring& user, int port, HWND hwnd)
{
	if (host.empty()) BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));
	if (host.empty()) BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));
	if (port > MAX_PORT) BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

	auto_cs lock(m_cs);

	// Try to get the session from the global pool
	wstring display_name = provider_moniker_name(user, host, port);

	// Just in case elevation is needed (it shouldn't be) we pass our
	// windows handle so that the elevation dialogue will be correctly
	// rooted.  We probably don't need this but as we've implemented
	// it, is can't hurt.
	BIND_OPTS3 bo;
	std::memset(&bo, 0, sizeof(bo));
	bo.cbStruct = sizeof(bo);
	bo.hwnd = hwnd;
	bo.dwClassContext = CLSCTX_ALL;
	bo.grfMode = STGM_READWRITE;

	com_ptr<IBindCtx> bind_context = create_bind_context();
	HRESULT hr = bind_context->SetBindOptions(&bo);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(
			comet::com_error_from_interface(bind_context, hr));

	// The default class moniker's BindStatusCallback creates a progress
	// dialogue which steals window focus even though it's never
	// displayed!!  The only way I've found to prevent this is to use a
	// custom callback object which does nothing except specify that
	// UI is forbidden.
	com_ptr<IBindStatusCallback> callback = new CBindCallbackStub();
	hr = ::RegisterBindStatusCallback(
		bind_context.get(), callback.get(), NULL, 0);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(comet::com_error(hr)) <<
			boost::errinfo_api_function("RegisterBindStatusCallback"));

	return object_from_moniker_name<ISftpProvider>(
		display_name, bind_context);
}
