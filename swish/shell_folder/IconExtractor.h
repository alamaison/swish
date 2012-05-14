/**
    @file

    Component allowing icon extraction based on file extension.

    @if license

    Copyright (C) 2008, 2009, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#pragma once

#include "swish/CoFactory.hpp" // CoObject factory mixin

#include "swish/atl.hpp"  // Common ATL setup
#include <atlstr.h>     // CString

#include <shlobj.h>     // Windows Shell API

class ATL_NO_VTABLE CIconExtractor :
	public IExtractIconW,
	public IExtractIconA,
	public ATL::CComObjectRoot,
	public swish::CCoFactory<CIconExtractor>
{
public:

	BEGIN_COM_MAP(CIconExtractor)
		COM_INTERFACE_ENTRY(IExtractIconW)
		COM_INTERFACE_ENTRY(IExtractIconA)
	END_COM_MAP()

	CIconExtractor() : m_fForFolder(false) {}

	static ATL::CComPtr<IExtractIconW> Create(
		PCTSTR szFilename, bool fIsFolder);
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

	bool m_fForFolder;           ///< Are we trying to extract the icon 
	                             ///< for a Folder?
	ATL::CString m_strFilename;  ///< File to get default icon for.
};