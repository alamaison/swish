/*  Implementation of handler for Shell Folder View interaction with Explorer.

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
#include "ExplorerCallback.h"

/**
 * Callback method for shell DEFVIEW to inform HostFolder as things happen.
 *
 * This is the way in which the default @c IShellView object that we created
 * using @c SHCreateShellFolderView allows us to still have a say in what is 
 * going on.  As things happen in the view, messages are sent to this
 * callback allowing us to react to them.
 *
 * @param uMsg The @c SFVM_* message type that the view is sending us.
 * @param wParam One of the possible parameters (varies with message type).
 * @param lParam Another possible parameter (varies with message type).
 *
 * @returns @c S_OK if we handled the message or @c E_NOTIMPL if we did not.
 */
STDMETHODIMP CExplorerCallback::MessageSFVCB( UINT uMsg, 
                                              WPARAM wParam, LPARAM lParam )
{	
	switch (uMsg)
	{
	case SFVM_MERGEMENU:
		{
			// The DEFVIEW is asking us if we want to merge any items into
			// the menu it has created before it adds it to the Explorer window

			QCMINFO *pInfo = (LPQCMINFO)lParam;
			ATLASSERT(pInfo);
			ATLASSERT(pInfo->idCmdFirst >= FCIDM_SHVIEWFIRST );
			ATLASSERT(pInfo->idCmdLast <= FCIDM_SHVIEWLAST );
			//ATLASSERT(::IsMenu(pInfo->hmenu));

			// Get handle to explorer Tools menu (index 4)
			HMENU hSubMenu = ::GetSubMenu(pInfo->hmenu, 4);

			// Insert add and remove connection menu items into it
			ATLASSERT_REPORT(
				::InsertMenu(
					hSubMenu, 2, MF_BYPOSITION, 
					pInfo->idCmdFirst + MENUIDOFFSET_ADD,
					_T("&Add SFTP Connection")),
				::GetLastError()
			);
			ATLASSERT_REPORT(
				::InsertMenu(
					hSubMenu, 3, MF_BYPOSITION, 
					pInfo->idCmdFirst + MENUIDOFFSET_REMOVE,
					_T("&Remove SFTP Connection")),
				::GetLastError()
			);

			// Return value of last menu ID plus 1
			pInfo->idCmdFirst += MENUIDOFFSET_LAST+1; // Added 2 items

			return S_OK;

			// I would have expected to have to remove these menu items
			// in SFVM_UNMERGEMENU but this seems to happen automatically
		}
	case SFVM_INVOKECOMMAND:
		{
			// The DEFVIEW is telling us that a menu or toolbar item has been
			// invoked in the Explorer window and is giving us a chance to
			// react to it

			UINT idCmd = (UINT)wParam;
			if (idCmd == MENUIDOFFSET_ADD)
			{
				//::MessageBox(NULL, _T("Add invoked"), NULL, MB_OK);
				return S_OK;
			}
			else if (idCmd == MENUIDOFFSET_REMOVE)
			{
				//::MessageBox(NULL, _T("Remove invoked"), NULL, MB_OK);
				return S_OK;
			}

			return E_NOTIMPL;
		}
	case SFVM_GETHELPTEXT:
		{
			UINT idCmd = (UINT)(LOWORD(wParam));
			UINT cchMax = (UINT)(HIWORD(wParam));
			LPTSTR pszText = (LPTSTR)lParam;

			if (idCmd == MENUIDOFFSET_ADD)
			{
				return ::StringCchCopy(pszText, cchMax, 
					_T("Create a new SFTP connection with Swish."));
			}
			else if (idCmd == MENUIDOFFSET_REMOVE)
			{
				return ::StringCchCopy(pszText, cchMax, 
					_T("Remove a SFTP connection created with Swish."));
			}

			return E_NOTIMPL;
		}
	default:
		return E_NOTIMPL;
	}

	return E_NOTIMPL;
}

// CExplorerCallback
