/*  Declaration of the PIDL manager superclass

    Copyright (C) 2007  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef PIDLMANAGER_H
#define PIDLMANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string.h>
#include "remotelimits.h"

class CPidlManager  
{
public:
	CPidlManager();
	virtual ~CPidlManager();

   LPITEMIDLIST Copy( LPCITEMIDLIST );
   void Delete( LPITEMIDLIST );
   UINT GetSize( LPCITEMIDLIST );

   LPITEMIDLIST GetNextItem( LPCITEMIDLIST );
   LPITEMIDLIST GetLastItem( LPCITEMIDLIST );

protected:
	PITEMID_CHILD GetDataSegment( LPCITEMIDLIST pidl );
	HRESULT CopyWSZString( PWSTR pwszDest, USHORT cchDest, PCWSTR pwszSrc );
};

#endif // PIDLMANAGER_H
