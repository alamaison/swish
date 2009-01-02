/*  Implementation of icon extraction handler.

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
#include "IconExtractor.h"

/**
 * Sets file or folder that this IconExtractor is being used for.
 *
 * @param szFilename The filename of the file or folder whose icon we want.
 * @param fIsFolder  Flag whether this is a file (@c false) or folder (@c true).
 */
void CIconExtractor::Initialize(PCTSTR szFilename, bool fIsFolder)
{
	m_fForFolder = fIsFolder;
	m_strFilename = szFilename;
}

/**
 * Retrieves location of the appropriate icon as an index into the system list.
 *
 * @implementing IExtractIconW
 *
 * @param[in]  uFlags        Flags that determine what type of icon is being
 *                           requested.
 * @param[out] pwszIconFile  The name of the file to find the icon in. In our 
 *                           case we return '*' to indicate that the icon is in 
 *                           the system index and the value returned in 
 *                           @p piIndex is the index to it.
 * @param[in]  cchMax        The size of the buffer passed as @p szIconFile.
 * @param[out] piIndex       The index to the icon in the system list.
 * @param[out] pwFlags       Output flags. In our case set to indicate that 
 *                           @p szIconFile is not a real filename.
 *             
 */
STDMETHODIMP CIconExtractor::GetIconLocation(
	UINT uFlags, PWSTR pwszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags )
{
	ATLTRACE("CIconExtractor::GetIconLocation called\n");

	// Look for icon's index into system list
	int index = _GetIconIndex(uFlags);
	if (index < 0)
		return S_FALSE; // None found - make shell use Unknown

	// Output * as filename to indicate icon is in system list
	ATLVERIFY(SUCCEEDED(
		::StringCchCopyW(pwszIconFile, cchMax, L"*")));
	*pwFlags = GIL_NOTFILENAME;
	*piIndex = index;

	return S_OK;
}

/**
 * @overload
 * @implementing IExtractIconA
 */
STDMETHODIMP CIconExtractor::GetIconLocation(
	UINT uFlags, LPSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
	ATLTRACE("CIconExtractor::GetIconLocation called\n");

	// Look for icon's index into system list
	int index = _GetIconIndex(uFlags);
	if (index < 0)
		return S_FALSE; // None found - make shell use Unknown

	// Output * as filename to indicate icon is in system list
	ATLVERIFY(SUCCEEDED(
		::StringCchCopyA(szIconFile, cchMax, "*")));
	*pwFlags = GIL_NOTFILENAME;
	*piIndex = index;

	return S_OK;
}

/**
 * Extract an icon bitmap given the information passed.
 *
 * @implementing IExtractIconW
 *
 * @returns S_FALSE to tell the shell to extract the icons itself.
 */
STDMETHODIMP CIconExtractor::Extract(PCWSTR, UINT, HICON *, HICON *, UINT)
{
	ATLTRACE("CIconExtractor::Extract called\n");
	return S_FALSE;
}

/**
 * @overload
 * @implementing IExtractIconA
 */
STDMETHODIMP CIconExtractor::Extract(PCSTR, UINT, HICON *, HICON *, UINT)
{
	ATLTRACE("CIconExtractor::Extract (A) called\n");
	return S_FALSE;
}

int CIconExtractor::_GetIconIndex(UINT uFlags)
{
	if (uFlags & GIL_DEFAULTICON)
		return 0;

	DWORD dwAttributes = 
		(m_fForFolder) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;

	UINT uInfoFlags = SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX;
	if (uFlags & GIL_OPENICON)
		uInfoFlags |= SHGFI_OPENICON;

	// Look up index to default icon in current system list
	{
		// Get index to default icon in system list
		SHFILEINFO shfi;
		if(::SHGetFileInfo(m_strFilename, dwAttributes, &shfi, 
			sizeof(shfi), uInfoFlags))
		{
			return shfi.iIcon;
		}
		else
			return -1; // None found
	}
}

// CIconExtractor