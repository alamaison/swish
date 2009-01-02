/*  Component allowing icon extraction based on file extension.

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

#ifndef ICONEXTRACTOR_H
#define ICONEXTRACTOR_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma once
#include "stdafx.h"
#include "resource.h"       // main symbols

// CIconExtractor
[
	coclass,
	default(IUnknown),
	threading(apartment),
	vi_progid("Swish.IconExtractor"),
	progid("Swish.IconExtractor.1"),
	version(1.0),
	uuid("b816a849-5022-11dc-9153-0090f5284f85"),
	helpstring("IconExtractor Class")
]
class ATL_NO_VTABLE CIconExtractor :
	public IExtractIconW,
	public IExtractIconA
{
public:
	CIconExtractor() : m_fForFolder(false) {}

	void Initialize(PCTSTR szFilename, bool fIsFolder);

	// IExtractIconW
	IFACEMETHODIMP GetIconLocation(
		UINT uFlags, __out_ecount(cchMax) PWSTR pwszIconFile, UINT cchMax,
		__out int *piIndex, __out UINT *pwFlags);

	IFACEMETHODIMP Extract(
		PCWSTR pszFile, UINT nIconIndex, __out_opt HICON *phiconLarge,
		__out_opt HICON *phiconSmall, UINT nIconSize);

	// IExtractIconA
	IFACEMETHODIMP GetIconLocation(
		UINT uFlags, __out_ecount(cchMax) LPSTR szIconFile, UINT cchMax,
		__out int *piIndex, __out UINT *pwFlags);

    IFACEMETHODIMP Extract(
		PCSTR pszFile, UINT nIconIndex, __out_opt HICON *phiconLarge,
		__out_opt HICON *phiconSmall, UINT nIconSize);

private:
	int _GetIconIndex(UINT uFlags);

	bool m_fForFolder;      ///< Are we trying to extract the icon for a Folder?
	CString m_strFilename;  ///< File to get default icon for.
};

#endif // ICONEXTRACTOR_H