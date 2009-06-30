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

#pragma once

#include "DataObject.h"
#include "Pidl.h"

class CShellDataObject
{
public:
	CShellDataObject(__in IDataObject *pDataObj);
	~CShellDataObject();

	CAbsolutePidl GetParentFolder() throw(...);
	CRelativePidl GetRelativeFile(UINT i) throw(...);
	CAbsolutePidl GetFile(UINT i) throw(...);
	UINT GetPidlCount() throw(...);

private:
	ATL::CComPtr<IDataObject> m_spDataObj;
	CStorageMedium m_medium;
	CGlobalLock m_glock;
};