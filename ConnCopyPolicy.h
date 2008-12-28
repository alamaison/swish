/*  Copy-policy class for copying CHostItem wrapped PIDL to PITEMID_CHILD.

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

#pragma once

#include "HostPidl.h"

#include <vector>
using std::vector;

class CConnCopyPolicy  
{
public:
	static void init(PITEMID_CHILD*) { /* No init needed */ }
    
    static HRESULT copy(
		__deref_out PITEMID_CHILD *pTo, __in const CHostItem *pFrom)
    {
		try
		{
			ATLASSERT(pFrom->IsValid());
			*pTo = pFrom->CopyTo();
		}
		catchCom()

		return S_OK;
    }

    static void destroy(__in PITEMID_CHILD *p) 
    {
		::ILFree(*p);
    }
};

typedef CComEnumOnSTL<IEnumIDList, &__uuidof(IEnumIDList), PITEMID_CHILD,
                      CConnCopyPolicy, vector<CHostItem> >
		CEnumIDListImpl;
