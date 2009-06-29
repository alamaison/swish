/*  Wrapper class for Shell Data Objects containing lists of PIDLs.

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

#include "ShellDataObject.h"

#define GetPIDLFolder(pida) \
	(PCIDLIST_ABSOLUTE)(((LPBYTE)pida)+(pida)->aoffset[0])
#define GetPIDLItem(pida, i) \
	(PCIDLIST_RELATIVE)(((LPBYTE)pida)+(pida)->aoffset[i+1])

CShellDataObject::CShellDataObject( IDataObject *pDataObj ) :
	m_spDataObj(pDataObj)
{
	UINT nCFSTR_SHELLIDLIST = ::RegisterClipboardFormat(CFSTR_SHELLIDLIST);

	FORMATETC fetc = {
		static_cast<CLIPFORMAT>(nCFSTR_SHELLIDLIST),
		NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
	};

	HRESULT hr = m_spDataObj->GetData(&fetc, &m_medium);
	ATLENSURE_SUCCEEDED(hr);

	m_glock.Attach(m_medium.hGlobal);
}

CShellDataObject::~CShellDataObject()
{
}

CAbsolutePidl CShellDataObject::GetParentFolder() throw(...)
{
	CIDA *pcida = m_glock.GetCida();
	ATLENSURE_THROW(pcida, E_UNEXPECTED);

	CAbsolutePidl pidl = GetPIDLFolder(pcida);
	return pidl;
}

CRelativePidl CShellDataObject::GetRelativeFile(UINT i) throw(...)
{
	CIDA *pcida = m_glock.GetCida();
	ATLENSURE_THROW(pcida, E_UNEXPECTED);

	if (i >= pcida->cidl)
		throw std::range_error(
			"The index is greater than the number of PIDLs in the Data Object");

	CRelativePidl pidl = GetPIDLItem(pcida, i);
	return pidl;
}

CAbsolutePidl CShellDataObject::GetFile(UINT i) throw(...)
{
	CAbsolutePidl pidlFolder(GetParentFolder(), GetRelativeFile(i));
	return pidlFolder;
}

UINT CShellDataObject::GetPidlCount() throw(...)
{
	CIDA *pcida = m_glock.GetCida();
	ATLENSURE_THROW(pcida, E_UNEXPECTED);

	return pcida->cidl;
}
