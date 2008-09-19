/*  Pool of reusuable SFTP connections.

    Copyright (C) 2007, 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
*/

#include "stdafx.h"
#include "remotelimits.h"
#include "XPool.h"

/*HRESULT CXPool::GetConnection(
	ISftpConsumer *pConsumer, BSTR bstrHost, BSTR bstrUser, UINT uPort,
	ISftpProvider **ppProvider )
{
	ATLENSURE_RETURN_HR(::SysStringLen(bstrHost) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(::SysStringLen(bstrUser) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(uPort < MAX_PORT, E_INVALIDARG);

	HRESULT hr = S_OK;
	std::wstring strHost(bstrHost), strUser(bstrUser);

	CComPtr<ISftpProvider> spProvider = m_mapSessions[strHost][strUser][uPort];

	if (spProvider == NULL)
	{
		// Create SFTP Provider from ProgID and initialise
		hr = spProvider.CoCreateInstance(
			OLESTR("Libssh2Provider.Libssh2Provider"));
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
		hr = spProvider->Initialize(pConsumer, bstrUser, bstrHost, uPort);
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);

		// Add to pool
		m_mapSessions[strHost][strUser][uPort] = spProvider;
	}
	else
	{
		// Switch SFTP Provider's SFTP consumer
		spProvider->SwitchConsumer(pConsumer);
	}

	// Return provider from pool
	*ppProvider = spProvider.Detach();

	return hr;
}*/

HRESULT CXPool::GetConnection(
	ISftpConsumer *pConsumer, BSTR bstrHost, BSTR bstrUser, UINT uPort,
	ISftpProvider **ppProvider )
{
	ATLENSURE_RETURN_HR(::SysStringLen(bstrHost) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(::SysStringLen(bstrUser) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(uPort < MAX_PORT, E_INVALIDARG);

	HRESULT hr = S_OK;

	try
	{
		CComPtr<ISftpProvider> spProvider =
			_GetConnectionFromROT(bstrHost, bstrUser, uPort);

		if (spProvider == NULL)
		{
			spProvider = _CreateNewConnection(pConsumer, bstrHost, bstrUser, uPort);

			// Add to pool
			_StoreConnectionInROT(spProvider, bstrHost, bstrUser, uPort);
		}
		else
		{
			// Switch SFTP Provider's SFTP consumer
			spProvider->SwitchConsumer(pConsumer);
		}

		// Return provider from pool
		*ppProvider = spProvider.Detach();
	}
	catch (...)
	{
		return E_FAIL;
	}

	return hr;
}

CComPtr<IMoniker> CXPool::_CreateMoniker(
	BSTR bstrHost, BSTR bstrUser, UINT uPort ) throw(...)
{
	CString strMonikerName;
	strMonikerName += bstrUser;
	strMonikerName += L"@";
	strMonikerName += bstrHost;
	strMonikerName.AppendFormat(L":%d", uPort);

	CComPtr<IMoniker> spMoniker;
	ATLENSURE_SUCCEEDED(
		::CreateItemMoniker(OLESTR("!"), strMonikerName, &spMoniker));

	return spMoniker;
}

CComPtr<ISftpProvider> CXPool::_GetConnectionFromROT(
	BSTR bstrHost, BSTR bstrUser, UINT uPort ) throw(...)
{
	CComPtr<IMoniker> spMoniker = _CreateMoniker(bstrHost, bstrUser, uPort);
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

void CXPool::_StoreConnectionInROT(
	ISftpProvider *pProvider, BSTR bstrHost, BSTR bstrUser, UINT uPort )
	throw(...)
{
	HRESULT hr;

	CComPtr<IMoniker> spMoniker = _CreateMoniker(bstrHost, bstrUser, uPort);
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

CComPtr<ISftpProvider> CXPool::_CreateNewConnection(
	ISftpConsumer *pConsumer, BSTR bstrHost, BSTR bstrUser, UINT uPort )
	throw(...)
{
	HRESULT hr;

	// Create SFTP Provider from ProgID and initialise

	CComPtr<ISftpProvider> spProvider;
	hr = spProvider.CoCreateInstance(OLESTR("Libssh2Provider.Libssh2Provider"));
	ATLENSURE_SUCCEEDED(hr);

	hr = spProvider->Initialize(pConsumer, bstrUser, bstrHost, uPort);
	ATLENSURE_SUCCEEDED(hr);

	return spProvider;
}