/**
    @file

	Pool of reusuable SFTP connections.

    @if licence

    Copyright (C) 2007, 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer

#include <comet/bstr.h> // bstr_t
#include <comet/interface.h>  // uuidof, comtype

#include <boost/lexical_cast.hpp> // lexical_cast
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

using swish::exception::com_exception;

using boost::lexical_cast;

using comet::com_ptr;
using comet::bstr_t;

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
	 * Create an item moniker for the session with the given parameters.
	 *
	 * e.g. !user@host:port
	 */
	com_ptr<IMoniker> create_item_moniker(
		const wstring& host, const wstring& user, int port)
	{
		wstring moniker_name = 
			user + L'@' + host + L':' + lexical_cast<wstring>(port);

		com_ptr<IMoniker> moniker;
		HRESULT hr = ::CreateItemMoniker(
			OLESTR("!"), moniker_name.c_str(), moniker.out());
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		return moniker;
	}

	/**
	 * Get the local Winstation Running Object Table.
	 */
	com_ptr<IRunningObjectTable> running_object_table()
	{
		com_ptr<IRunningObjectTable> rot;

		HRESULT hr = ::GetRunningObjectTable(0, rot.out());
		assert(SUCCEEDED(hr));
		assert(rot);

		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		return rot;
	}

	/**
	 * Fetch a session from the Running Object Table.
	 *
	 * If there is no session in the ROT that matches the given paramters,
	 * return NULL.
	 */
	com_ptr<ISftpProvider> session_from_rot(
		const wstring& host, const wstring& user, int port)
	{
		com_ptr<IMoniker> moniker = create_item_moniker(host, user, port);
		com_ptr<IRunningObjectTable> rot = running_object_table();

		com_ptr<IUnknown> unknown;
		// We don't care if this fails - return NULL to indicate not found
		rot->GetObject(moniker.in(), unknown.out());

		com_ptr<ISftpProvider> provider = com_cast(unknown);
		assert(provider || !unknown); // failure should not be due to QI
		return provider;
	}

	void store_session_in_rot(
		const com_ptr<ISftpProvider>& provider, const wstring& host,
		const wstring& user, int port)
	{
		HRESULT hr;

		com_ptr<IMoniker> moniker = create_item_moniker(host, user, port);
		com_ptr<IRunningObjectTable> rot = running_object_table();

		DWORD dwCookie;
		com_ptr<IUnknown> unknown = provider;
		hr = rot->Register(
			ROTFLAGS_REGISTRATIONKEEPSALIVE, unknown.in(), moniker.in(), 
			&dwCookie);
		assert(hr == S_OK);
		if (hr == MK_S_MONIKERALREADYREGISTERED)
		{
			// This should never get called.  In case it does, we revoke 
			// the duplicate.  
			rot->Revoke(dwCookie);
		}
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));
		// TODO: find way to revoke normal case when finished with them
	}

	com_ptr<ISftpProvider> create_new_session(
		const com_ptr<ISftpConsumer>& consumer, const bstr_t& host,
		const bstr_t& user, int port)
	{
		HRESULT hr;

		// Create SFTP Provider from ProgID and initialise

		com_ptr<ISftpProvider> provider(L"Provider.Provider");
		hr = provider->Initialize(consumer.in(), user.in(), host.in(), port);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		return provider;
	}
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
 * @throws AtlException if any error occurs.
 *
 * @returns pointer to the session (ISftpProvider).
 */
com_ptr<ISftpProvider> CPool::GetSession(
	const com_ptr<ISftpConsumer>& consumer, const wstring& host, 
	const wstring& user, int port)
{
	if (!consumer) BOOST_THROW_EXCEPTION(com_exception(E_POINTER));
	if (host.empty()) BOOST_THROW_EXCEPTION(com_exception(E_INVALIDARG));
	if (host.empty()) BOOST_THROW_EXCEPTION(com_exception(E_INVALIDARG));
	if (port > MAX_PORT) BOOST_THROW_EXCEPTION(com_exception(E_INVALIDARG));

	// Try to get the session from the global pool
	com_ptr<ISftpProvider> provider = session_from_rot(host, user, port);

	if (!provider)
	{
		// No existing session; create new one and add to the pool
		provider = create_new_session(consumer, host, user, port);
		store_session_in_rot(provider, host, user, port);
	}
	else
	{
		// Existing session found; switch it to use new SFTP consumer
		HRESULT hr = provider->SwitchConsumer(consumer.in());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));
	}

	assert(provider);
	return provider;
}
