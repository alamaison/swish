/*  Helper to create context menu for host objects and execute user choice

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

#include "stdafx.h"
#include "HostContextMenu.h"

/*------------------------------------------------------------------------------
 * CHostContextMenu::Initialize
 * Initialises the context menu object for a given host connection object.
 * Used in place of passing parameters to a constructor due to ATL template.
 * The PIDL passed in is an absolute PIDL to the host connection RemoteFolder
 * and is needed to perform ShellExecuteEx on if the Connect item is chosen.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CHostContextMenu::Initialize( PCIDLIST_ABSOLUTE pidl )
{
	ATLTRACE("CHostContextMenu::Initialize called\n");
	if (pidl == NULL)
		return E_POINTER;

	// TODO: use a PidlManager to verify PIDLs?

	m_pidl = static_cast<PIDLIST_ABSOLUTE>( ::ILClone(pidl) );
	return S_OK;
}

// IContextMenu

/*------------------------------------------------------------------------------
 * CHostContextMenu::QueryContextMenu : IContextMenu
 * Adds items to the given context menu (hMenu).
 * The first position at which the item should be inserted is given in 
 * indexMenu (TODO: when would this not be 0?).  The menu command IDs should
 * lie between idCmdFirst and idCmdLast and are saved for later use.
 * We return the largest command id of the menu items created plus one.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CHostContextMenu::QueryContextMenu(
	__in HMENU hMenu, __in UINT indexMenu, __in UINT idCmdFirst,
	__in UINT idCmdLast, __in UINT uFlags)
{
	ATLTRACE("CHostContextMenu::QueryContextMenu called\n");
	ATLASSERT(idCmdFirst + MENUOFFSET_LAST < idCmdLast);

	// Add menu item at next position with menu command calculated from offset
	InsertMenu( hMenu, indexMenu++, MF_BYPOSITION, 
		idCmdFirst + MENUOFFSET_CONNECT, _T("&Connect") );

	// The CMF_DEFAULTONLY flag tells namespace extensions to add only 
	// the default menu item - we only have one at all, currently, but
	// when we have more we will need: if (uFlags & CMF_DEFAULTONLY)

	// Set Connect menu verb as default
	if (!(uFlags & CMF_NODEFAULT))
		SetMenuDefaultItem( hMenu, idCmdFirst + MENUOFFSET_CONNECT, false );

    // Return largest menu offset + 1
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(MENUOFFSET_LAST + 1));
}

/*------------------------------------------------------------------------------
 * CHostContextMenu::GetCommandString : IContextMenu
 * Language-independent verb or status bar help string requested.
 * The request can be for either an ANSI or a unicode version (indicated
 * with uType) and both should be supported.  Returns S_OK or E_INVALIDARG if
 * idCmd is not valid for this menu.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CHostContextMenu::GetCommandString(
	__in UINT_PTR idCmd, __in UINT uType, __reserved UINT *pReserved, 
	__out_awcount(!(uType & GCS_UNICODE), cchMax) LPSTR pszName, 
	__in UINT cchMax)
{
	ATLTRACE("CRemoteFolder::GetCommandString called\n");
	(void)pReserved;
	HRESULT hr = E_INVALIDARG;

	// Validate idCmd (deals with GCS_VALIDATE into the bargain)
	if(idCmd < MENUOFFSET_FIRST || idCmd > MENUOFFSET_LAST)
        return hr;

	// If the code reaches an ANSI GCS_ case it is most likely because the 
	// unicode version failed and Explorer is trying ANSI as a fallback

	switch(uType)
	{
	case GCS_HELPTEXTA:
		if (idCmd == MENUOFFSET_CONNECT)
			hr = StringCchCopyA(pszName, cchMax,
				"Connect to remote filesystem over SFTP");
		break;

	case GCS_HELPTEXTW:
		if (idCmd == MENUOFFSET_CONNECT)
			hr = StringCchCopyW((PWSTR)pszName, cchMax,
				L"Connect to remote filesystem over SFTP");
		break; 

	case GCS_VERBA:
		if (idCmd == MENUOFFSET_CONNECT)
			hr = StringCchCopyA(pszName, cchMax, "connect");
		break;

	case GCS_VERBW:
		if (idCmd == MENUOFFSET_CONNECT)
			hr = StringCchCopyW((PWSTR)pszName, cchMax, L"connect");
		break;

	default:
		hr = S_OK;
		break; 
	}
	return hr;
}

/*------------------------------------------------------------------------------
 * CHostContextMenu::InvokeCommand : IContextMenu
 * A menu command has been selected to execute on the PIDL.
 * The chosen command can be either an ANSI verb, a unicode VERB or a menu ID.
 * We parse the value passed to determing which one and then execute the 
 * chosen command.
 * Currently, only the 'connect' command is supported and we simply invoke
 * the default action for a folder type (open).
 * If the command verb/id is not recognised we return E_FAIL.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CHostContextMenu::InvokeCommand(
	__in CMINVOKECOMMANDINFO *pici )
{
	ATLTRACE("CRemoteFolder::InvokeCommand called\n");

	BOOL fUnicode = FALSE;		// Is CMINVOKECOMMANDINFO struct unicode?
	MENUOFFSET menuCmd = MENUOFFSET_NULL;
								// Store chosen command from parsing
	CMINVOKECOMMANDINFOEX *piciEx = { 0 };
								// pici cast to unicode version (if needed)

	if (pici->cbSize == sizeof(CMINVOKECOMMANDINFOEX) &&
	   (pici->fMask & CMIC_MASK_UNICODE))
	{
		// Given command info struct is actually a unicode version,
		// cast accordingly
		fUnicode = TRUE;
		piciEx = (CMINVOKECOMMANDINFOEX *) pici;
	}

	if (!fUnicode && HIWORD(pici->lpVerb))				// ANSI
	{
		if (!StrCmpIA( pici->lpVerb, "connect" ))
			menuCmd = MENUOFFSET_CONNECT;
	}
	else if(fUnicode && HIWORD(piciEx->lpVerbW))		// Unicode
	{
		if(StrCmpIW( piciEx->lpVerbW, L"connect" ))
			menuCmd = MENUOFFSET_CONNECT;
	}
	else if(LOWORD(pici->lpVerb) == MENUOFFSET_CONNECT)	// Menu ID
	{
		menuCmd = MENUOFFSET_CONNECT;
	}
	else												// Invalid verb/ID
	{
		// An attempt was made to invoke a verb/ID not supported by
		// this menu's PIDL. For the time being, we are assuming this
		// doesn't happen and all cases are caught by one of the above.
		UNREACHABLE;
		return E_FAIL;
	}

	ATLASSERT(menuCmd >= MENUOFFSET_FIRST && menuCmd <= MENUOFFSET_LAST);

	// Set up ShellExecute to execute chosen verb on this menu's PIDL (m_pidl)
	SHELLEXECUTEINFO sei;
	ZeroMemory(&sei, sizeof(sei));
	sei.cbSize = sizeof(sei);
	// Needed if multiple menu items used
	// if (menuCmd == MENUOFFSET_CONNECT)
	//     sei.lpVerb = _T("connect");
	sei.fMask = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME;
	sei.lpIDList = ::ILClone(m_pidl);
	sei.lpClass = _T("folder");
	sei.hwnd = pici->hwnd;
	sei.nShow = SW_SHOWNORMAL;

	HRESULT hr = (ShellExecuteEx(&sei)) ? S_OK : E_FAIL;

	::ILFree(static_cast<PIDLIST_RELATIVE>( sei.lpIDList ));

	return hr;
}

// CHostContextMenu

