/*  Declaration of the host object context menu handler

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

#ifndef HOSTCONTEXTMENU_H
#define HOSTCONTEXTMENU_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "resource.h"       // main symbols

// CHostContextMenu
[
	coclass,
	default(IContextMenu),
	threading(apartment),
	vi_progid("Swish.HostContextMenu"),
	progid("Swish.HostContextMenu.1"),
	version(1.0),
	uuid("b816a840-5022-11dc-9153-0090f5284f85"),
	helpstring("HostContextMenu Class")
]
class ATL_NO_VTABLE CHostContextMenu :
	public IContextMenu
{
public:
	CHostContextMenu()
	{
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	void FinalRelease()
	{
	}

	STDMETHOD(Initialize) ( PCIDLIST_ABSOLUTE pidl );

	// IContextMenu
	IFACEMETHODIMP QueryContextMenu( HMENU hmenu, UINT indexMenu, 
									 UINT idCmdFirst, UINT idCmdLast,
									 UINT uFlags );        
	IFACEMETHODIMP InvokeCommand( CMINVOKECOMMANDINFO *pici );
    IFACEMETHODIMP GetCommandString( UINT_PTR idCmd, UINT uType, 
									 UINT *pReserved, LPSTR pszName,
									 UINT cchMax );

private:
	// Menu command ID offsets for host connection objects context menu
	enum MENUOFFSET
	{
		MENUOFFSET_NULL = 0,
		MENUOFFSET_FIRST = 1,
		MENUOFFSET_CONNECT = MENUOFFSET_FIRST,
		MENUOFFSET_LAST = MENUOFFSET_CONNECT
	};

	// Absolute PIDL to this menu's corresponding remote folder object
	// Used to execute ShellExecuteEx on
	PIDLIST_ABSOLUTE m_pidl;
};

#endif // HOSTCONTEXTMENU_H
