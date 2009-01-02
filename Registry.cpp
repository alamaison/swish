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
#include "RemotePidl.h"

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
 *   HKCU\\Directory
 *   HKCU\\Directory\\Background
 *   HKCU\\Folder
 *   HKCU\\*
 *   HKCU\\AllFileSystemObjects
 *
 * @param[out] pcKeys  Number of HKEYS allocated in the array @p paKeys.
 * @param[out] paKeys  Location in which to return th allocated array.
 */
/* static */ HRESULT CRegistry::GetHostFolderAssocKeys(
	UINT *pcKeys, HKEY **paKeys)
throw()
{
	try
	{
		return _GetHKEYArrayFromKeynames(
			_GetHostFolderAssocKeynames(), pcKeys, paKeys);
	}
	catchCom()
}

/**
 * Get registry keys for HostFolder connection association info.
 *
 * This list is not required for Windows Vista but, on any earlier 
 * version, it must be passed to CDefFolderMenu_Create2 in order to display 
 * the default context menu.
 *
 * A (ficticious) example might include:
 *   HKCU\\.ppt
 *   HKCU\\PowerPoint.Show
 *   HKCU\\PowerPoint.Show.12
 *   HKCU\\SystemFileAssociations\\.ppt
 *   HKCU\\SystemFileAssociations\\presentation
 *   HKCU\\*
 *   HKCU\\AllFileSystemObjects
 * for a file and:
 *   HKCU\\Directory
 *   HKCU\\Directory\\Background
 *   HKCU\\Folder
 *   HKCU\\*
 *   HKCU\\AllFileSystemObjects
 * for a folder.
 *
 * @param[in]  pidl    Remote PIDL representing the file whose association 
 *                     information is being requested.
 * @param[out] pcKeys  Number of HKEYS allocated in the array @p paKeys.
 * @param[out] paKeys  Location in which to return th allocated array.
 */
HRESULT CRegistry::GetRemoteFolderAssocKeys(
	CRemoteItemHandle pidl, UINT *pcKeys, HKEY **paKeys)
throw()
{
	try
	{
		return _GetHKEYArrayFromKeynames(
			_GetRemoteFolderAssocKeynames(pidl), pcKeys, paKeys);
	}
	catchCom()
}


/*----------------------------------------------------------------------------*
 * Private functions
 *----------------------------------------------------------------------------*/

/**
 * Get names of registry keys which provide association info for Folder items.
 *
 * Such a list is required by Windows XP and earlier in order to display the 
 * default context menu. The keys are:
 *   HKCU\\Directory
 *   HKCU\\Directory\\Background
 *   HKCU\\Folder
 *   HKCU\\*
 *   HKCU\\AllFileSystemObjects
 */
/* static */ vector<CString> CRegistry::_GetHostFolderAssocKeynames()
throw()
{
	vector<CString> vecNames;

	// Add directory-specific items
	vecNames = _GetKeynamesForFolder();

	// Add names of keys that apply to items of all types
	vector<CString> vecCommon = _GetKeynamesCommonToAll();
	vecNames.insert(vecNames.end(), vecCommon.begin(), vecCommon.end());

	return vecNames;
}

/**
 * Get a list names of registry keys for the types of the selected file.
 *
 * A (ficticious) example might include:
 *   HKCU\\.ppt
 *   HKCU\\PowerPoint.Show
 *   HKCU\\PowerPoint.Show.12
 *   HKCU\\SystemFileAssociations\\.ppt
 *   HKCU\\SystemFileAssociations\\presentation
 *   HKCU\\*
 *   HKCU\\AllFileSystemObjects
 * for a file and:
 *   HKCU\\Directory
 *   HKCU\\Directory\\Background
 *   HKCU\\Folder
 *   HKCU\\*
 *   HKCU\\AllFileSystemObjects
 * for a folder.
 *
 * @param pidl  Remote PIDL representing the file whose association 
 *              information is being requested.
 */
/* static */ vector<CString> CRegistry::_GetRemoteFolderAssocKeynames(
	CRemoteItemHandle pidl)
throw(...)
{
	vector<CString> vecNames;

	// If this is a directory, add directory-specific items
	if (pidl.IsFolder())
	{
		vecNames = _GetKeynamesForFolder();
	}
	else
	{
		// Get extension-specific keys
		// We don't want to add the {.ext} key itself to the list
		// of keys but rather, we should use it's default value to 
		// look up its file class.
		// e.g:
		//   HKCR\.txt => (Default) txtfile
		// so we look up the following key
		//   HKCR\txtfile
		vecNames = _GetKeynamesForExtension(pidl.GetExtension());
	}

	// Add names of keys that apply to items of all types
	vector<CString> vecCommon = _GetKeynamesCommonToAll();
	vecNames.insert(vecNames.end(), vecCommon.begin(), vecCommon.end());

	return vecNames;
}

/**
 * Get list of directory-specific association key names.
 */
/* static */ vector<CString> CRegistry::_GetKeynamesForFolder()
throw()
{
	vector<CString> vecKeynames;

	vecKeynames.push_back(L"Directory");
	vecKeynames.push_back(L"Directory\\Background");
	vecKeynames.push_back(L"Folder");

	return vecKeynames;
}

/**
 * Get list of directory-specific association key names.
 */
/* static */ vector<CString> CRegistry::_GetKeynamesCommonToAll()
throw()
{
	vector<CString> vecKeynames;

	vecKeynames.push_back(L"AllFilesystemObjects");
	vecKeynames.push_back(L"*");

	return vecKeynames;
}

/**
 * Get the list of names of registry keys related to a specific file extension.
 *
 * @param pwszExtension  File extension whose keys will be returned.
 */
/* static */ vector<CString> CRegistry::_GetKeynamesForExtension(
	__in PCWSTR pwszExtension)
throw()
{
	vector<CString> vecKeynames;
	CString strExtension = CString(L".") + pwszExtension;

	// Start digging at HKCR\.{szExtension}
	CRegKey reg;
	if (reg.Open(HKEY_CLASSES_ROOT, strExtension, KEY_READ)	== ERROR_SUCCESS)
	{
		vecKeynames.push_back(strExtension);

		// Try to get registered file class key (extensions's default val)
		wchar_t wszClass[2048]; ULONG cchClass = 2048;
		if (reg.QueryStringValue(L"", wszClass, &cchClass) == ERROR_SUCCESS
		 && cchClass > 1
		 && reg.Open(HKEY_CLASSES_ROOT, wszClass, KEY_READ) == ERROR_SUCCESS)
		{
			vecKeynames.push_back(wszClass);

			// Does this class contain a CurVer subkey pointing to another
			// version of this file.
			//   e.g.: PowerPoint.Show\CurVer => PowerPoint.Show.12
			CString strCurVer = wszClass;
			strCurVer += L"\\CurVer";
			if (reg.Open(HKEY_CLASSES_ROOT, strCurVer, KEY_READ)
				== ERROR_SUCCESS)
			{
				// Does this CurVer exist?
				wchar_t wszCurVer[2048]; ULONG cchCurVer = 2048;
				if (reg.QueryStringValue(L"", wszCurVer, &cchCurVer) ==
					ERROR_SUCCESS && cchCurVer > 1
				 && reg.Open(HKEY_CLASSES_ROOT, wszCurVer, KEY_READ) == 
					ERROR_SUCCESS)
				{
					vecKeynames.push_back(wszCurVer);
				}
			}
		}
	}

	// Dig again at HKCR\SystemFileAssociations\.{sxExtension}
	CString strSysFileAssocExt = L"SystemFileAssociations\\" + strExtension;
	if (reg.Open(HKEY_CLASSES_ROOT, strSysFileAssocExt, KEY_READ)
		== ERROR_SUCCESS)
	{
		vecKeynames.push_back(strSysFileAssocExt);
	}

	// Dig again at HKCR\.{szExtension}\PerceivedType
	if (reg.Open(HKEY_CLASSES_ROOT, strExtension, KEY_READ)
		== ERROR_SUCCESS)
	{
		wchar_t wszPerceivedType[2048]; ULONG cchPerceivedType = 2048;
		if (reg.QueryStringValue(
				L"PerceivedType", wszPerceivedType, &cchPerceivedType)
			== ERROR_SUCCESS && cchPerceivedType > 1)
		{
			CString strPerceivedType = 
				CString(L"SystemFileAssociations\\") + wszPerceivedType;

			if (reg.Open(HKEY_CLASSES_ROOT, strPerceivedType, KEY_READ)
				== ERROR_SUCCESS)
				vecKeynames.push_back(strPerceivedType);
		}
	}

	if (!vecKeynames.size())
		vecKeynames.push_back(L"Unknown");

	ATLASSERT( vecKeynames.size() <= 5 ); 
	return vecKeynames;
}

/**
 * Create SHAlloced array of HKEYs from a list of registry key names.
 */
/* static */ HRESULT CRegistry::_GetHKEYArrayFromKeynames(
	const vector<CString> vecNames, UINT *pcKeys, HKEY **paKeys)
throw()
{
	vector<HKEY> vecKeys = _GetKeysFromKeynames(vecNames);
	return _GetHKEYArrayFromVector(vecKeys, pcKeys, paKeys);
}

/**
 * Create SHAlloced array of HKEYs from a list of HKEYs.
 */
/* static */ HRESULT CRegistry::_GetHKEYArrayFromVector(
	const vector<HKEY> vecKeys, UINT *pcKeys, HKEY **paKeys)
throw()
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
 * Create list of registry handles from list of keys names.
 */
/* static */ vector<HKEY> CRegistry::_GetKeysFromKeynames(
	const vector<CString> vecKeynames)
throw()
{
	LSTATUS rc = ERROR_SUCCESS;
	vector<HKEY> vecKeys;
	
	for each (CString strKeyname in vecKeynames)
	{
		CRegKey reg;
		rc = reg.Open(HKEY_CLASSES_ROOT, strKeyname, KEY_READ);
		ATLASSERT(rc == ERROR_SUCCESS);
		if (rc == ERROR_SUCCESS)
			vecKeys.push_back(reg.Detach());
	}

	return vecKeys;
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

	rc = regConnection.QueryDWORDValue(L"Port", dwPort);             // Port
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
