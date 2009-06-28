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

#include "pch.h"
#include "Pool.h"

#include "swish/remotelimits.h"  // Text field limits

#include <atlstr.h>  // CString

using ATL::CComPtr;
using ATL::CComQIPtr;
using ATL::CString;
using ATL::CComBSTR;

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
CComPtr<ISftpProvider> CPool::GetSession(
	ISftpConsumer *pConsumer, PCWSTR pszHost, PCWSTR pszUser, UINT uPort )
	throw(...)
{
	ATLENSURE_THROW(pConsumer, E_POINTER);
	ATLENSURE_THROW(pszHost[0] != '\0', E_INVALIDARG);
	ATLENSURE_THROW(pszUser[0] != '\0', E_INVALIDARG);
	ATLENSURE_THROW(uPort < MAX_PORT, E_INVALIDARG);

	// Try to get the session from the global pol
	CComPtr<ISftpProvider> spProvider = 
		_GetSessionFromROT(pszHost, pszUser, uPort);

	if (spProvider == NULL)
	{
		// No existing session; create new one and add to the pool
		spProvider = _CreateNewSession(pConsumer, pszHost, pszUser, uPort);
		_StoreSessionInROT(spProvider, pszHost, pszUser, uPort);
	}
	else
	{
		// Existing session found; switch it to use new SFTP consumer
		spProvider->SwitchConsumer(pConsumer);
	}

	ATLENSURE(spProvider);
	return spProvider;
}

CComPtr<IMoniker> CPool::_CreateMoniker(
	PCWSTR pszHost, PCWSTR pszUser, UINT uPort ) throw(...)
{
	CString strMonikerName;
	strMonikerName.Format(L"%ls@%ls:%d", pszUser, pszHost, uPort);

	CComPtr<IMoniker> spMoniker;
	ATLENSURE_SUCCEEDED(
		::CreateItemMoniker(OLESTR("!"), strMonikerName, &spMoniker));

	return spMoniker;
}

CComPtr<ISftpProvider> CPool::_GetSessionFromROT(
	PCWSTR pszHost, PCWSTR pszUser, UINT uPort ) throw(...)
{
	CComPtr<IMoniker> spMoniker = _CreateMoniker(pszHost, pszUser, uPort);
	CComPtr<IRunningObjectTable> spROT;
	ATLENSURE_SUCCEEDED(::GetRunningObjectTable(NULL, &spROT));

	CComPtr<IUnknown> spUnk;
	if (spROT->GetObject(spMoniker, &spUnk) == S_OK)
	{
		CComQIPtr<ISftpProvider> spProvider = spUnk;
		ATLENSURE_THROW(spProvider, E_NOINTERFACE);

		return spProvider;
	}
	else
		return NULL;
}

void CPool::_StoreSessionInROT(
	ISftpProvider *pProvider, PCWSTR pszHost, PCWSTR pszUser, UINT uPort )
	throw(...)
{
	HRESULT hr;

	CComPtr<IMoniker> spMoniker = _CreateMoniker(pszHost, pszUser, uPort);
	CComPtr<IRunningObjectTable> spROT;
	ATLENSURE_SUCCEEDED(::GetRunningObjectTable(NULL, &spROT));

	DWORD dwCookie;
	CComPtr<IUnknown> spUnk = pProvider;
	hr = spROT->Register(
		ROTFLAGS_REGISTRATIONKEEPSALIVE, spUnk, spMoniker, &dwCookie);
	ATLASSERT(hr == S_OK);
	if (hr == MK_S_MONIKERALREADYREGISTERED)
	{
		// This should never get called.  In case it does, we revoke 
		// the duplicate.  
		spROT->Revoke(dwCookie);
	}
	ATLENSURE_SUCCEEDED(hr);
	// TODO: find way to revoke normal case when finished with them
}

CComPtr<ISftpProvider> CPool::_CreateNewSession(
	ISftpConsumer *pConsumer, PCWSTR pszHost, PCWSTR pszUser, UINT uPort )
	throw(...)
{
	HRESULT hr;

	// Create SFTP Provider from ProgID and initialise

	CComPtr<ISftpProvider> spProvider;
	hr = spProvider.CoCreateInstance(OLESTR("Provider.Provider"));
	ATLENSURE_SUCCEEDED(hr);

	hr = spProvider->Initialize(
		pConsumer, CComBSTR(pszUser), CComBSTR(pszHost), uPort);
	ATLENSURE_SUCCEEDED(hr);

	return spProvider;
}
