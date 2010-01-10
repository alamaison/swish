/**
    @file

	Pool of reusuable SFTP connections.

    @if licence

    Copyright (C) 2007, 2008, 2009, 2010
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
#include "swish/exception.hpp" // com_exception
#include "swish/utils.hpp" // running_object_table
#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer

#include <comet/interface.h>  // uuidof, comtype

#include <boost/lexical_cast.hpp> // lexical_cast
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

using swish::exception::com_exception;

using boost::lexical_cast;

using comet::com_ptr;
using comet::critical_section;
using comet::auto_cs;
using comet::uuidof;

using std::wstring;


namespace comet {

template<> struct comtype<ISftpProvider>
{
	static const IID& uuid() throw() { return IID_ISftpProvider; }
	typedef IUnknown base;
};


template<> struct comtype<ISftpConsumer>
{
	static const IID& uuid() throw() { return IID_ISftpConsumer; }
	typedef IUnknown base;
};

} // namespace comet

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

	/**
	 * Get an object instance by its moniker display name.
	 *
	 * Corresponds to CoGetObject Windows API function with default BIND_OPTS.
	 */
	template<typename T>
	com_ptr<T> object_from_moniker_name(const wstring& display_name)
	{
		com_ptr<T> object;
		HRESULT hr = ::CoGetObject(
			display_name.c_str(), NULL, 
			uuidof(object.in()), reinterpret_cast<void**>(object.out()));
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		return object;
	}
}

critical_section CPool::m_cs;

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
 * @throws AtlException if any error occurs.
 *
 * @returns pointer to the session (ISftpProvider).
 */
com_ptr<ISftpProvider> CPool::GetSession(
	const wstring& host, const wstring& user, int port)
{
	if (host.empty()) BOOST_THROW_EXCEPTION(com_exception(E_INVALIDARG));
	if (host.empty()) BOOST_THROW_EXCEPTION(com_exception(E_INVALIDARG));
	if (port > MAX_PORT) BOOST_THROW_EXCEPTION(com_exception(E_INVALIDARG));

	auto_cs lock(m_cs);

	// Try to get the session from the global pool
	wstring display_name = provider_moniker_name(user, host, port);
	com_ptr<ISftpProvider> provider;
	return object_from_moniker_name<ISftpProvider>(display_name);
}
