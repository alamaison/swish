/*  Helper class for Swish registry access.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include "Registry.h"

/**
 * Load all the connections stored in the registry into PIDLs.
 *
 * It's possible that there aren't any connections in 
 * the @c Software\\Swish\\Connections key of the registry, in which case 
 * the vector is left empty.
 *
 * @returns  Vector of PIDLs containing the details of all the SFTP
 *           stored in the registry.
 * @throws  CAtlException if something unexpected happens such as corrupt
 *          registry structure.
 */
/* static */ vector<CHostItem> CRegistry::LoadConnectionsFromRegistry()
throw(...)
{
	CRegKey regConnections;
	LSTATUS rc = ERROR_SUCCESS;
	vector<CHostItem> vecConnections;

	rc = regConnections.Open(
		HKEY_CURRENT_USER, L"Software\\Swish\\Connections");

	if (rc == ERROR_SUCCESS) // Legal to fail here - may be first ever connection
	{
		int iSubKey = 0;
		wchar_t wszLabel[2048]; 
		do {
			DWORD cchLabel = 2048;
			rc = regConnections.EnumKey(iSubKey, wszLabel, &cchLabel);
			if (rc == ERROR_SUCCESS)
			{
				vecConnections.push_back(
					_GetConnectionDetailsFromRegistry(wszLabel));
			}
			iSubKey++;
		} while (rc == ERROR_SUCCESS);

		ATLASSERT_REPORT(rc == ERROR_NO_MORE_ITEMS, rc);
		ATLVERIFY(regConnections.Close() == ERROR_SUCCESS);
	}

	return vecConnections; // May be empty
}

/**
 * Get registry keys for HostFolder connection association info.
 *
 * This list is not required for Windows Vista but, on any earlier 
 * version, it must be passed to CDefFolderMenu_Create2 in order to display 
 * the default context menu.
 *
 * Host connection items are treated as folders so the list of keys is:  
 *   HKCU\Directory
 *   HKCU\Directory\Background
 *   HKCU\Folder
 *   HKCU\*
 *   HKCU\AllFileSystemObjects
 */
HRESULT CRegistry::GetHostFolderAssocKeys(UINT *pcKeys, HKEY **paKeys)
{
	try
	{
		vector<HKEY> vecKeynames = _GetFolderAssocRegistryKeys();
		return _GetHKEYArrayFromVector(vecKeynames, pcKeys, paKeys);
	}
	catchCom()
}


/*----------------------------------------------------------------------------*
 * Private functions
 *----------------------------------------------------------------------------*/

/**
 * Get registry keys which provide association info for Folder items.  
 *
 * Such a list is required by Windows XP and earlier in order to display the 
 * default context menu. The keys are:
 *   HKCU\Directory
 *   HKCU\Directory\Background
 *   HKCU\Folder
 *   HKCU\*
 *   HKCU\AllFileSystemObjects
 */
/* static */ vector<HKEY> CRegistry::_GetFolderAssocRegistryKeys()
throw(...)
{
	LSTATUS rc = ERROR_SUCCESS;	
	vector<CString> vecKeynames;

	// Add directory-specific items
	vecKeynames.push_back(L"Directory");
	vecKeynames.push_back(L"Directory\\Background");
	vecKeynames.push_back(L"Folder");

	// Add names of keys that apply to items of all types
	vecKeynames.push_back(L"AllFilesystemObjects");
	vecKeynames.push_back(L"*");

	// Create list of registry handles from list of keys
	vector<HKEY> vecKeys;
	for (UINT i = 0; i < vecKeynames.size(); i++)
	{
		CRegKey reg;
		CString strKey = vecKeynames[i];
		ATLASSERT(strKey.GetLength() > 0);

		TRACE("Opening HKCR\\%s", strKey);
		rc = reg.Open(HKEY_CLASSES_ROOT, strKey, KEY_READ);
		ATLASSERT(rc == ERROR_SUCCESS);
		if (rc == ERROR_SUCCESS)
			vecKeys.push_back(reg.Detach());
	}

	return vecKeys;
}

/* static */ HRESULT CRegistry::_GetHKEYArrayFromVector(
	vector<HKEY> vecKeys, UINT *pcKeys, HKEY **paKeys)
{
	ATLASSERT( vecKeys.size() >= 3 );  // The minimum we must have added
	ATLASSERT( vecKeys.size() <= 16 ); // CDefFolderMenu_Create2's maximum

	HKEY *aKeys = (HKEY *)::SHAlloc(vecKeys.size() * sizeof HKEY); 
	for (UINT i = 0; i < vecKeys.size(); i++)
		aKeys[i] = vecKeys[i];

	*pcKeys = static_cast<UINT>(vecKeys.size());
	*paKeys = aKeys;

	return S_OK;
}

/**
 * Get a single connection from the registry as a PIDL.
 *
 * @pre The @c Software\\Swish\\Connections registry key exists.
 * @pre The connection is present as a subkey of the 
 *      @c Software\\Swish\\Connections registry key whose name is given
 *      by @p pwszLabel.
 *
 * @param pwszLabel  Friendly name of the connection to load.
 *
 * @returns  A host PIDL holding the connection details.
 * @throws  CAtlException: E_FAIL if the registry key does not exist
 *          and E_UNEXPECTED if the registry is corrupted.
 */
/* static */ CHostItem CRegistry::_GetConnectionDetailsFromRegistry(
	__in PCWSTR pwszLabel)
throw(...)
{
	CRegKey regConnection;
	LSTATUS rc = ERROR_SUCCESS;

	// Target variables to load values into
	wchar_t wszHost[MAX_HOSTNAME_LENZ]; ULONG cchHost = MAX_HOSTNAME_LENZ;
	DWORD dwPort;
	wchar_t wszUser[MAX_USERNAME_LENZ]; ULONG cchUser = MAX_USERNAME_LENZ;
	wchar_t wszPath[MAX_PATH_LENZ];     ULONG cchPath = MAX_PATH_LENZ;

	// Open HKCU\Software\Swish\Connections\<pwszLabel> registry key
	CString strKey = CString(L"Software\\Swish\\Connections\\") + pwszLabel;
	rc = regConnection.Open(HKEY_CURRENT_USER, strKey);
	ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_FAIL);

	// Load values
	rc = regConnection.QueryStringValue(L"Host", wszHost, &cchHost); // Host

	rc = regConnection.QueryDWORDValue(L"Port", dwPort);            // Port
	ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_UNEXPECTED);
	ATLASSERT(dwPort >= MIN_PORT);
	ATLASSERT(dwPort <= MAX_PORT);
	USHORT uPort = static_cast<USHORT>(dwPort);

	rc = regConnection.QueryStringValue(L"User", wszUser, &cchUser); // User
	ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_UNEXPECTED);

	rc = regConnection.QueryStringValue(L"Path", wszPath, &cchPath); // Path
	ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_UNEXPECTED);

	// Create new PIDL to return
	CHostItem pidl(wszUser, wszHost, uPort, wszPath, pwszLabel);

	ATLVERIFY(regConnection.Close() == ERROR_SUCCESS);

	return pidl;
}
