/**
    @file

	Handler for the Shell Folder View's interaction with Explorer.

    @if licence

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "ExplorerCallback.h"

#include "data_object/ShellDataObject.hpp"  // PidlFormat
#include "Registry.h"
#include "swish/shell_folder/commands/host/host.hpp" // host commands
#include "swish/debug.hpp"
#include "swish/catch_com.hpp" // catchCom
#include "swish/exception.hpp"

#include <strsafe.h>          // For StringCchCopy

#include <string>
#include <vector>

using ATL::CComPtr;
using ATL::CComQIPtr;
using ATL::CString;

using std::wstring;
using std::vector;

using swish::exception::com_exception;
using swish::shell_folder::data_object::PidlFormat;
using swish::shell_folder::commands::host::Add;
using swish::shell_folder::commands::host::Remove;

#define SFVM_SELECTIONCHANGED 8

HRESULT CExplorerCallback::Initialize( PCIDLIST_ABSOLUTE pidl )
{
	m_pidl = ::ILCloneFull(pidl);
	return (m_pidl) ? S_OK : E_OUTOFMEMORY;
}


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
	ATLASSUME(m_pidl);

	switch (uMsg)
	{
	case SFVM_WINDOWCREATED:
		m_hwndView = (HWND)wParam;
		return S_OK;
	case SFVM_GETNOTIFY:
		// Tell the shell that we might notify it of update events that
		// apply to this folder (specified using our absolute PIDL)
		*reinterpret_cast<LONG*>(lParam) = 
			SHCNE_UPDATEDIR | SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER |
			SHCNE_DELETE | SHCNE_RMDIR;
		*reinterpret_cast<PCIDLIST_ABSOLUTE*>(wParam) = m_pidl; // Owned by us
		return S_OK;
	case SFVM_FSNOTIFY:
		// The shell is telling us that an event (probably a SHChangeNotify
		// of some sort) has affected one of our item.  Just nod.
		return S_OK;
	case SFVM_MERGEMENU:
		{
			// The DEFVIEW is asking us if we want to merge any items into
			// the menu it has created before it adds it to the Explorer window

			QCMINFO *pInfo = (LPQCMINFO)lParam;
			ATLASSERT(pInfo);
			ATLASSERT(pInfo->idCmdFirst >= FCIDM_SHVIEWFIRST );
			ATLASSERT(pInfo->idCmdLast <= FCIDM_SHVIEWLAST );
			//ATLASSERT(::IsMenu(pInfo->hmenu));
			m_idCmdFirst = pInfo->idCmdFirst;

			// Try to get a handle to the  Explorer Tools menu and insert 
			// add and remove connection menu items into it if we find it
			m_hToolsMenu = _GetToolsMenu(pInfo->hmenu);
			if (m_hToolsMenu)
			{
				try
				{
					Add add(m_hwndView, m_pidl);
					ATLVERIFY_REPORT(
						::InsertMenu(
							m_hToolsMenu, 2, MF_BYPOSITION, 
							m_idCmdFirst + MENUIDOFFSET_ADD,
							add.title(
								_GetSelectionDataObject().p).c_str()),
						::GetLastError());
					
					Remove remove(m_hwndView, m_pidl);
					ATLVERIFY_REPORT(
						::InsertMenu(
							m_hToolsMenu, 3, MF_BYPOSITION | MF_GRAYED, 
							m_idCmdFirst + MENUIDOFFSET_REMOVE,
							remove.title(
								_GetSelectionDataObject().p).c_str()),
						::GetLastError());
				}
				catchCom()

				// Return value of last menu ID plus 1
				pInfo->idCmdFirst += MENUIDOFFSET_LAST+1; // Added 2 items
			}

			return S_OK;

			// I would have expected to have to remove these menu items
			// in SFVM_UNMERGEMENU but this seems to happen automatically
		}
	case SFVM_SELECTIONCHANGED:
		// Update the menus to match the current selection.
		if (m_hToolsMenu)
		{
			UINT flags;
			try
			{
				Add add(m_hwndView, m_pidl);
				flags = add.disabled(_GetSelectionDataObject().p, false) ? 
					MF_GRAYED : MF_ENABLED;
				ATLVERIFY(::EnableMenuItem(
					m_hToolsMenu, m_idCmdFirst + MENUIDOFFSET_ADD,
					MF_BYCOMMAND | flags) >= 0);

				Remove remove(m_hwndView, m_pidl);
				flags = remove.disabled(_GetSelectionDataObject().p, false) ?
					MF_GRAYED : MF_ENABLED;
				ATLVERIFY(::EnableMenuItem(
					m_hToolsMenu, m_idCmdFirst + MENUIDOFFSET_REMOVE,
					MF_BYCOMMAND | flags) >= 0);
			}
			catchCom()

			return S_OK;
		}
	case SFVM_INVOKECOMMAND:
		{
			// The DEFVIEW is telling us that a menu or toolbar item has been
			// invoked in the Explorer window and is giving us a chance to
			// react to it

			try
			{
				UINT idCmd = (UINT)wParam;
				if (idCmd == MENUIDOFFSET_ADD)
				{
					Add command(m_hwndView, m_pidl);
					command(_GetSelectionDataObject().p, NULL);
					return S_OK;
				}
				else if (idCmd == MENUIDOFFSET_REMOVE)
				{
					
					Remove command(m_hwndView, m_pidl);
					command(_GetSelectionDataObject().p, NULL);
					return S_OK;
				}
			}
			catchCom()

			return E_NOTIMPL;
		}
	case SFVM_GETHELPTEXT:
		{
			try
			{
				UINT idCmd = (UINT)(LOWORD(wParam));
				UINT cchMax = (UINT)(HIWORD(wParam));
				LPTSTR pszText = (LPTSTR)lParam;

				if (idCmd == MENUIDOFFSET_ADD)
				{
					Add command(m_hwndView, m_pidl);
					return ::StringCchCopy(
						pszText, cchMax, command.tool_tip(
							_GetSelectionDataObject().p).c_str());
				}
				else if (idCmd == MENUIDOFFSET_REMOVE)
				{
					Remove command(m_hwndView, m_pidl);
					return ::StringCchCopy(
						pszText, cchMax, command.tool_tip(
							_GetSelectionDataObject().p).c_str());
				}
			}
			catchCom()

			return E_NOTIMPL;
		}
	default:
		return E_NOTIMPL;
	}

	return E_NOTIMPL;
}


/*----------------------------------------------------------------------------*
 * Private functions
 *----------------------------------------------------------------------------*/

/**
 * Get handle to explorer 'Tools' menu.
 *
 * The menu we want to insert into is actually the @e submenu of the
 * Tools menu @e item.  Confusing!
 */
HMENU CExplorerCallback::_GetToolsMenu(HMENU hParentMenu)
{
	MENUITEMINFO info;
	info.cbSize = sizeof(MENUITEMINFO);
	info.fMask = MIIM_SUBMENU; // Item we are requesting

	BOOL fSucceeded = ::GetMenuItemInfo(
		hParentMenu, FCIDM_MENU_TOOLS, FALSE, &info);
	REPORT(fSucceeded);

	return (fSucceeded) ? info.hSubMenu : NULL;
}

/**
 * Return whether the Remove Host menu should be enabled.
 */
bool CExplorerCallback::_ShouldEnableRemove()
{
	try
	{
		PidlFormat format(_GetSelectionDataObject().p);
		return format.pidl_count() == 1;
	}
	catch (com_exception)
	{
		return false;
	}
}

/**
 * Return the single item in the ShellView that is currently selected.
 * Fail if more than one item is selected or if none is.
 */
CAbsolutePidl CExplorerCallback::_GetSelectedItem()
{
	PidlFormat format(_GetSelectionDataObject().p);
	if (format.pidl_count() != 1)
		AtlThrow(E_FAIL);
	return format.file(0).get();
}

/**
 * Return a DataObject representing the items currently selected.
 *
 * @return NULL if nothing is selected.
 */
CComPtr<IDataObject> CExplorerCallback::_GetSelectionDataObject()
{
	CComPtr<IShellView> spView = _GetShellView();
	CComPtr<IDataObject> spDataObject;
	spView->GetItemObject(
		SVGIO_SELECTION, __uuidof(IDataObject), (void **)&spDataObject);

	// We don't care if getting the DataObject succeded - if it did, great;
	// return it.  If not we will return a NULL pointer indicating that no
	// items were selected

	return spDataObject;
}

/**
 * Return the parent IShellView from the site set through IObjectWithSite.
 *
 * @throw AtlException if interface not found.
 */
CComPtr<IShellView> CExplorerCallback::_GetShellView()
{
	CComPtr<IShellBrowser> spBrowser = _GetShellBrowser();
	CComPtr<IShellView> spView;
	ATLENSURE_SUCCEEDED(spBrowser->QueryActiveShellView(&spView));
	return spView;
}

/**
 * Return the parent IShellBrowser from the site set through IObjectWithSite.
 *
 * @throw AtlException if interface not found.
 */
CComPtr<IShellBrowser> CExplorerCallback::_GetShellBrowser()
{
	if (!m_spUnkSite)
		AtlThrow(E_NOINTERFACE);

	CComQIPtr<IServiceProvider> spSP = m_spUnkSite;
	if (!spSP)
		AtlThrow(E_NOINTERFACE);

	CComPtr<IShellBrowser> spBrowser;
	ATLENSURE_SUCCEEDED(spSP->QueryService(SID_SShellBrowser, &spBrowser));
	return spBrowser;
}