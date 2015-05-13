/**
    @file

    Helper class for Swish registry access.

    @if license

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "Registry.h"

#include "swish/debug.hpp"
#include "swish/remote_folder/remote_pidl.hpp" // remote_itemid_view

#include <washer/com/catch.hpp> // WASHER_COM_CATCH_AUTO_INTERFACE

#include <boost/filesystem/path.hpp> // wpath

#include <string>

using swish::remote_folder::remote_itemid_view;

using ATL::CRegKey;
using ATL::CString;

using boost::filesystem::wpath;

using std::vector;
using std::wstring;

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
    WASHER_COM_CATCH();
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
 *   HKCU\\AllFileSystemObjects
 * for a folder.
 *
 * @param[in]  itemid  Remote PIDL representing the file whose association 
 *                     information is being requested.
 * @param[out] pcKeys  Number of HKEYS allocated in the array @p paKeys.
 * @param[out] paKeys  Location in which to return th allocated array.
 */
HRESULT CRegistry::GetRemoteFolderAssocKeys(
    remote_itemid_view itemid, UINT *pcKeys, HKEY **paKeys)
{
    try
    {
        return _GetHKEYArrayFromKeynames(
            _GetRemoteFolderAssocKeynames(itemid), pcKeys, paKeys);
    }
    WASHER_COM_CATCH();
}

namespace {

CRegistry::KeyNames remote_folder_background_key_names() 
{
    CRegistry::KeyNames names;

    names.push_back(L"Directory\\Background");

    return names;
}

}

HRESULT CRegistry::GetRemoteFolderBackgroundAssocKeys(
    UINT *pcKeys, HKEY **paKeys)
{
    try
    {
        return _GetHKEYArrayFromKeynames(
            remote_folder_background_key_names(), pcKeys, paKeys);
    }
    WASHER_COM_CATCH();
}


/*----------------------------------------------------------------------------*
 * Private functions
 *----------------------------------------------------------------------------*/

/**
 * Get names of registry keys which provide association info for Folder items.
 *
 * Such a list is required by Windows XP and earlier in order to display the 
 * default context menu. Only 'HKCR\\Folder' is relevant as the Swish hosts are
 * virtual folder items with no filesystem parallel.  'HKCR\\Directory' and
 * 'HKCR\AllFileSystemObjects' are for real filesystem items.  'HKCR\\*' is
 * not for folders at all.
 */
/* static */ CRegistry::KeyNames CRegistry::_GetHostFolderAssocKeynames()
throw()
{
    KeyNames vecNames;

    // Add virtual folder specific items
    vecNames.push_back(L"Folder");

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
 *   HKCU\\Folder
 *   HKCU\\Directory
 *   HKCU\\Directory\\Background
 *   HKCU\\AllFileSystemObjects
 * for a folder.
 *
 * @param itemid  Remote PIDL representing the file whose association 
 *                information is being requested.
 */
/* static */ CRegistry::KeyNames CRegistry::_GetRemoteFolderAssocKeynames(
    remote_itemid_view itemid)
throw(...)
{
    KeyNames vecNames;

    // If this is a directory, add directory-specific items
    if (itemid.is_folder())
    {
        vecNames = _GetKeynamesForFolder();
    }
    else
    {
        wstring extension = wpath(itemid.filename()).extension();
        wstring::size_type dot_index = extension.find(L".");
        if (dot_index != wstring::npos)
            extension.erase(dot_index);

        // Get extension-specific keys
        // We don't want to add the {.ext} key itself to the list
        // of keys but rather, we should use it's default value to 
        // look up its file class.
        // e.g:
        //   HKCR\.txt => (Default) txtfile
        // so we look up the following key
        //   HKCR\txtfile
        vecNames = _GetKeynamesForExtension(extension.c_str());
    }

    // Add names of keys that apply to items of all types
    KeyNames vecCommon = _GetKeynamesCommonToAll();
    vecNames.insert(vecNames.end(), vecCommon.begin(), vecCommon.end());

    return vecNames;
}

/**
 * Get list of directory-specific association key names.
 */
/* static */ CRegistry::KeyNames CRegistry::_GetKeynamesForFolder()
throw()
{
    KeyNames vecKeynames;

    vecKeynames.push_back(L"Folder");
    vecKeynames.push_back(L"Directory");
    vecKeynames.push_back(L"Directory\\Background");

    return vecKeynames;
}

/**
 * Get list of directory-specific association key names.
 */
/* static */ CRegistry::KeyNames CRegistry::_GetKeynamesCommonToAll()
throw()
{
    KeyNames vecKeynames;

    vecKeynames.push_back(L"AllFilesystemObjects");

    return vecKeynames;
}

/**
 * Get the list of names of registry keys related to a specific file extension.
 *
 * @param pwszExtension  File extension whose keys will be returned.
 *
 * @todo  Some files, e.g. PDFs, need
 *        HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.ext
 */
/* static */ CRegistry::KeyNames CRegistry::_GetKeynamesForExtension(
    __in PCWSTR pwszExtension)
throw()
{
    KeyNames vecKeynames;
    CString strExtension = CString(L".") + pwszExtension;

    // Start digging at HKCR\.{szExtension}
    CRegKey reg;
    if (reg.Open(HKEY_CLASSES_ROOT, strExtension, KEY_READ)    == ERROR_SUCCESS)
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

    vecKeynames.push_back(L"*");

    ATLASSERT( vecKeynames.size() <= 6 ); 
    return vecKeynames;
}

/**
 * Create SHAlloced array of HKEYs from a list of registry key names.
 */
/* static */ HRESULT CRegistry::_GetHKEYArrayFromKeynames(
    const KeyNames vecNames, UINT *pcKeys, HKEY **paKeys)
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
    const KeyNames vecKeynames)
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
