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

#include "NewConnDialog.h"
#include "ShellDataObject.h"
#include "Registry.h"
#include "host_management.hpp"
#include "swish/debug.hpp"
#include "swish/catch_com.hpp"
#include "swish/exception.hpp"

#include <strsafe.h>          // For StringCchCopy

#include <string>
#include <vector>

using ATL::CComPtr;
using ATL::CComQIPtr;
using ATL::CString;

using std::wstring;
using std::vector;

using swish::host_management::AddConnectionToRegistry;
using swish::host_management::RemoveConnectionFromRegistry;
using swish::host_management::ConnectionExists;
using swish::exception::com_exception;

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
				ATLVERIFY_REPORT(
					::InsertMenu(
						m_hToolsMenu, 2, MF_BYPOSITION, 
						m_idCmdFirst + MENUIDOFFSET_ADD,
						L"&Add SFTP Connection..."),
					::GetLastError());
				ATLVERIFY_REPORT(
					::InsertMenu(
						m_hToolsMenu, 3, MF_BYPOSITION | MF_GRAYED, 
						m_idCmdFirst + MENUIDOFFSET_REMOVE,
						L"&Remove SFTP Connection..."),
					::GetLastError());

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
			UINT flags = (_ShouldEnableRemove()) ? MF_ENABLED : MF_GRAYED;
			ATLVERIFY(::EnableMenuItem(
				m_hToolsMenu, m_idCmdFirst + MENUIDOFFSET_REMOVE,
				MF_BYCOMMAND | flags) >= 0);

			return S_OK;
		}
	case SFVM_INVOKECOMMAND:
		{
			// The DEFVIEW is telling us that a menu or toolbar item has been
			// invoked in the Explorer window and is giving us a chance to
			// react to it

			UINT idCmd = (UINT)wParam;
			if (idCmd == MENUIDOFFSET_ADD)
			{
				HRESULT hr = _AddNewConnection();
				if (SUCCEEDED(hr))
					_RefreshView();
				return hr;
			}
			else if (idCmd == MENUIDOFFSET_REMOVE)
			{
				HRESULT hr = _RemoveConnection();
				if (SUCCEEDED(hr))
					_RefreshView();
				return hr;
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

HRESULT CExplorerCallback::_RemoveConnection()
{
	ATLASSUME(m_hwndView);

	try
	{
		CHostItemAbsolute pidl_selected = _GetSelectedItem();
		wstring label = pidl_selected.FindHostPidl().GetLabel();
		ATLENSURE_THROW(label.size() > 0, E_UNEXPECTED);

		RemoveConnectionFromRegistry(label);
	}
	catchCom()

	return S_OK;
}

HRESULT CExplorerCallback::_AddNewConnection()
{
	ATLASSUME(m_hwndView);

	try
	{
		// Display dialog to get connection info from user
		wstring label, user, host, path;
		UINT port;
		CNewConnDialog dlgNewConnection;
		dlgNewConnection.SetPort( 22 ); // Sensible default
		if (dlgNewConnection.DoModal(m_hwndView) == IDOK)
		{
			label = dlgNewConnection.GetName();
			user = dlgNewConnection.GetUser();
			host = dlgNewConnection.GetHost();
			path = dlgNewConnection.GetPath();
			port = dlgNewConnection.GetPort();
		}
		else
			AtlThrow(E_FAIL);

		if (ConnectionExists(label))
			AtlThrow(E_FAIL);

		AddConnectionToRegistry(label, host, port, user, path);
	}
	catchCom()

	return S_OK;
}

/**
 * Cause Explorer to refresh any windows displaying the owning folder.
 */
void CExplorerCallback::_RefreshView()
{
	ATLASSUME(m_pidl);

	// Inform shell that something in our folder changed (we don't know exactly
	// what the new PIDL is until we reload from the registry, hence UPDATEDIR)
	::SHChangeNotify( SHCNE_UPDATEDIR, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, 
		m_pidl, NULL );
}

/**
 * Return whether the Remove Host menu should be enabled.
 */
bool CExplorerCallback::_ShouldEnableRemove()
{
	try
	{
		CShellDataObject data_object = _GetSelectionDataObject();
		return data_object.GetPidlCount() == 1;
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
	CShellDataObject data_object = _GetSelectionDataObject();
	if (data_object.GetPidlCount() != 1)
		AtlThrow(E_FAIL);
	return data_object.GetFile(0);
}

/**
 * Return a DataObject representing the items currently selected.
 *
 * @throw AtlException if interface not found.
 */
CComPtr<IDataObject> CExplorerCallback::_GetSelectionDataObject()
{
	CComPtr<IShellView> spView = _GetShellView();
	CComPtr<IDataObject> spDataObject;
	HRESULT hr = spView->GetItemObject(
		SVGIO_SELECTION, __uuidof(IDataObject), (void **)&spDataObject);
	if (FAILED(hr))
		AtlThrow(hr); // Legal to fail here - maybe nothing selected
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