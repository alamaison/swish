/**
    @file

    Handler for Shell Folder View's interaction with Explorer.

    @if license

    Copyright (C) 2008, 2009, 2010, 2011
    Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "ExplorerCallback.hpp"

#include "swish/nse/task_pane.hpp" // make_ui_command
#include "swish/shell_folder/commands/Command.hpp" // MenuCommandTitleAdapter
#include "swish/shell_folder/commands/host/host.hpp" // host commands

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE
#include <winapi/error.hpp> // last_error
#include <winapi/shell/services.hpp> // shell_browser, shell_view
#include <winapi/trace.hpp> // trace

#include <boost/exception/diagnostic_information.hpp> // diagnostic_information
#include <boost/exception/errinfo_file_name.hpp> // errinfo_file_name
#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <stdexcept> // logic_error
#include <string>

using swish::nse::IEnumUICommand;
using swish::nse::IUIElement;
using swish::shell_folder::commands::host::Add;
using swish::shell_folder::commands::host::Remove;
using swish::shell_folder::commands::host::host_folder_task_pane_tasks;
using swish::shell_folder::commands::host::host_folder_task_pane_titles;
using swish::shell_folder::commands::MenuCommandTitleAdapter;

using comet::com_error;
using comet::com_ptr;

using winapi::last_error;
using winapi::shell::pidl::apidl_t;
using winapi::shell::shell_browser;
using winapi::shell::shell_view;
using winapi::trace;

using boost::diagnostic_information;
using boost::enable_error_info;
using boost::errinfo_api_function;

using std::pair;
using std::wstring;

namespace swish {
namespace shell_folder {

namespace {

	/// @name  Undocumented messages
	// @{
	const UINT SFVM_SELECTIONCHANGED = 8;
	const UINT SFVM_GET_WEBVIEW_CONTENT = 83;
	const UINT SFVM_GET_WEBVIEW_TASKS = 84;
	// @}

	// Menu command ID offsets for Explorer Tools menu
	enum MENUOFFSET
	{
		MENUIDOFFSET_FIRST = 0,
		MENUIDOFFSET_ADD = MENUIDOFFSET_FIRST,
		MENUIDOFFSET_REMOVE,
		MENUIDOFFSET_LAST = MENUIDOFFSET_REMOVE
	};

	HMENU submenu_from_menu(HMENU parent_menu, UINT menu_id)
	{
		MENUITEMINFO info = MENUITEMINFO();
		info.cbSize = sizeof(MENUITEMINFO);
		info.fMask = MIIM_SUBMENU; // Item we're requesting

		BOOL success = ::GetMenuItemInfo(parent_menu, menu_id, FALSE, &info);
		if (success == FALSE)
			BOOST_THROW_EXCEPTION(
				enable_error_info(last_error()) << 
				errinfo_api_function("GetMenuItemInfo"));

		return info.hSubMenu;
	}

	/**
	 * Get handle to explorer 'Tools' menu.
	 *
	 * The menu we want to insert into is actually the @e submenu of the
	 * Tools menu @e item.  Confusing!
	 */
	HMENU tools_menu_with_fallback(HMENU parent_menu)
	{
		try
		{
			return submenu_from_menu(parent_menu, FCIDM_MENU_TOOLS);
		}
		catch (const std::exception& e)
		{
			trace("Failed getting tools menu: %s") % diagnostic_information(e);

			// Fall back to using File menu
			return submenu_from_menu(parent_menu, FCIDM_MENU_FILE);
		}
	}


	
	/**
	 * Return a DataObject representing the items currently selected.
	 *
	 * @return NULL if nothing is selected.
	 */
	com_ptr<IDataObject> selection_data_object(com_ptr<IShellBrowser> browser)
	{
		com_ptr<IShellView> view = shell_view(browser);

		com_ptr<IDataObject> data_object;
		view->GetItemObject(
			SVGIO_SELECTION, data_object.iid(),
			reinterpret_cast<void **>(data_object.out()));

		// We don't care if getting the DataObject succeded - if it did, great;
		// return it.  If not we will return a NULL pointer indicating that no
		// items were selected

		return data_object;
	}

	bool is_vista_or_greater()
	{
		OSVERSIONINFO version = OSVERSIONINFO();
		version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if (::GetVersionEx(&version) == FALSE)
			BOOST_THROW_EXCEPTION(
				enable_error_info(last_error()) << 
				errinfo_api_function("GetVersionEx"));

		return version.dwMajorVersion > 5;
	}
}

/**
 * Create customisation callback object for Explorer default shell view.
 *
 * @param folder_pidl  Absolute PIDL to the folder for whom we are
 *                     creating this callback object.
 */
CExplorerCallback::CExplorerCallback(const apidl_t& folder_pidl) :
	m_folder_pidl(folder_pidl), m_hwnd_view(NULL), m_tools_menu(NULL),
	m_first_command_id(0) {}

/**
 * The folder window is being created.
 *
 * The shell is notifying us of the folder view's window handle.
 */
bool CExplorerCallback::on_window_created(HWND hwnd_view)
{
	m_hwnd_view = hwnd_view;
	return true;
}

/**
 * Which events should the shell monitor for changes?
 *
 * We are notified via SFVM_FSNOTIFY if any events indicated here occurr.
 *
 * @TODO: It's possible that @a events already has events set in its bitmask
 *        so maybe we should 'or' our extra events with it.
 */
bool CExplorerCallback::on_get_notify(
	PCIDLIST_ABSOLUTE& pidl_monitor, LONG& events)
{
	// Tell the shell that we might notify it of update events that
	// apply to this folder (specified using our absolute PIDL)
	events = SHCNE_UPDATEDIR | SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER |
		SHCNE_DELETE | SHCNE_MKDIR | SHCNE_RMDIR;
	pidl_monitor = m_folder_pidl.get(); // Owned by us
	return true;
}

/**
 * An event has occurred affecting one of our items.
 *
 * Returning false prevents the default view from refreshing to reflect the
 * change.
 */
bool CExplorerCallback::on_fs_notify(
	PCIDLIST_ABSOLUTE /*pidl*/, LONG /*event*/)
{
	// The shell is telling us that an event (probably a SHChangeNotify
	// of some sort) has affected one of our item.  Just nod.
	// If we don't it doesn't work.
	return true;
}

/**
 * The DEFVIEW is asking us if we want to merge any items into
 * the menu it has created before it adds it to the Explorer window.
 */
bool CExplorerCallback::on_merge_menu(QCMINFO& menu_info)
{
	assert(menu_info.idCmdFirst >= FCIDM_SHVIEWFIRST);
	assert(menu_info.idCmdLast <= FCIDM_SHVIEWLAST);
	//assert(::IsMenu(menu_info.hmenu));
	m_first_command_id = menu_info.idCmdFirst;

	// Try to get a handle to the  Explorer Tools menu and insert 
	// add and remove connection menu items into it if we find it
	m_tools_menu = tools_menu_with_fallback(menu_info.hmenu);
	if (m_tools_menu)
	{
		MenuCommandTitleAdapter<Add> add(m_hwnd_view, m_folder_pidl);
		BOOL success = ::InsertMenu(
			m_tools_menu, 2, MF_BYPOSITION,
			m_first_command_id + MENUIDOFFSET_ADD,
			add.title(NULL).c_str());
		if (success == FALSE)
			BOOST_THROW_EXCEPTION(
				enable_error_info(last_error()) << 
				errinfo_api_function("InsertMenu"));

		MenuCommandTitleAdapter<Remove> remove(m_hwnd_view, m_folder_pidl);
		success = ::InsertMenu(
			m_tools_menu, 3, MF_BYPOSITION | MF_GRAYED, 
			m_first_command_id + MENUIDOFFSET_REMOVE,
			remove.title(NULL).c_str());
		if (success == FALSE)
			BOOST_THROW_EXCEPTION(
				enable_error_info(last_error()) << 
				errinfo_api_function("InsertMenu"));

		// Return value of last menu ID plus 1
		menu_info.idCmdFirst += MENUIDOFFSET_LAST + 1; // Added 2 items
	}

	return true;

	// I would have expected to have to remove these menu items
	// in SFVM_UNMERGEMENU but this seems to happen automatically
}

bool CExplorerCallback::on_selection_changed(
	SFV_SELECTINFO& /*selection_info*/)
{
	update_menus();
	return true;
}

/**
 * The view is about to display a popup menu.
 *
 * This gives us the chance to modify the menu before it is displayed.
 *
 * @param first_command_id  First ID reserved for client commands.
 * @param menu_index        Menu's index.
 * @param menu              Menu's handle.
 */
bool CExplorerCallback::on_init_menu_popup(
	UINT /*first_command_id*/, int /*menu_index*/, HMENU /*menu*/)
{
	update_menus();
	return true;
}

/**
 * DEFVIEW is telling us that a menu or toolbar item has been invoked 
 * in the Explorer window and is giving us a chance to react to it.
 */
bool CExplorerCallback::on_invoke_command(UINT command_id)
{
	if (command_id == MENUIDOFFSET_ADD)
	{
		Add command(m_hwnd_view, m_folder_pidl);
		command(selection(), NULL);
		return true;
	}
	else if (command_id == MENUIDOFFSET_REMOVE)
	{
		Remove command(m_hwnd_view, m_folder_pidl);
		command(selection(), NULL);
		return true;
	}

	return false;
}

#pragma warning(push)
#pragma warning(disable:4996) // std::copy ... may be unsafe

/**
 * Specify help text for menu or toolbar items.
 */
bool CExplorerCallback::on_get_help_text(
	UINT command_id, UINT buffer_size, LPTSTR buffer)
{
	wstring help_text;
	if (command_id == MENUIDOFFSET_ADD)
	{
		Add command(m_hwnd_view, m_folder_pidl);
		help_text = command.tool_tip(selection());
	}
	else if (command_id == MENUIDOFFSET_REMOVE)
	{
		Remove command(m_hwnd_view, m_folder_pidl);
		help_text = command.tool_tip(selection());
	}
	else
	{
		return false;
	}
	
	size_t copied = help_text.copy(buffer, buffer_size - 1);
	buffer[copied] = _T('\0');
	return true;
}

#pragma warning(pop)

/**
 * The shell view is requesting our expando title info.
 * Undocumented by Microsoft.
 *
 * @see http://www.codeproject.com/KB/shell/foldertasks.aspx
 * @see http://www.eggheadcafe.com/forumarchives/platformsdkshell/Feb2006/post25949644.asp
 */
bool CExplorerCallback::on_get_webview_content(
	SFV_WEBVIEW_CONTENT_DATA& content_out)
{
	assert(content_out.pFolderTasksExpando == NULL);
	assert(content_out.pExtraTasksExpando == NULL);
	assert(content_out.pEnumRelatedPlaces == NULL);

	// HACK: webview conflicts with ExplorerCommands so we disable it if
	//       ExplorerCommands are likely to be used.
	if (is_vista_or_greater())
		return false;

	pair< com_ptr<IUIElement>, com_ptr<IUIElement> > tasks =
		host_folder_task_pane_titles(m_hwnd_view, m_folder_pidl);

	content_out.pExtraTasksExpando = tasks.first.detach();
	content_out.pFolderTasksExpando = tasks.second.detach();
	return true;
}

/**
 * The shell view is requesting our expando members.
 * Undocumented by Microsoft.
 *
 * @see http://www.codeproject.com/KB/shell/foldertasks.aspx
 * @see http://www.eggheadcafe.com/forumarchives/platformsdkshell/Feb2006/post25949644.asp
 */
bool CExplorerCallback::on_get_webview_tasks(
	SFV_WEBVIEW_TASKSECTION_DATA& tasks_out)
{
	//for some reason this fails on 64-bit
	//assert(tasks_out.pEnumExtraTasks == NULL);

	assert(tasks_out.pEnumFolderTasks == NULL);


	// HACK: webview conflicts with ExplorerCommands so we disable it if
	//       ExplorerCommands are likely to be used.
	if (is_vista_or_greater())
		return false;

	pair< com_ptr<IEnumUICommand>, com_ptr<IEnumUICommand> > commands =
		host_folder_task_pane_tasks(m_hwnd_view, m_folder_pidl);

	tasks_out.pEnumExtraTasks = commands.first.detach();
	tasks_out.pEnumFolderTasks = commands.second.detach();
	return true;
}

/**
 * Callback method for shell DEFVIEW to inform HostFolder as things happen.
 *
 * This is the way in which the default @c IShellView object that we created
 * using @c SHCreateShellFolderView allows us to still have a say in what is 
 * going on.  As things happen in the view, messages are sent to this
 * callback allowing us to react to them.
 *
 * @param message  The @c SFVM_* message type that the view is sending us.
 * @param wparam   One of the possible parameters (varies with message type).
 * @param lparam   Another possible parameter (varies with message type).
 *
 * @returns @c S_OK if we handled the message or @c E_NOTIMPL if we did not.
 */
STDMETHODIMP CExplorerCallback::MessageSFVCB(
	UINT message, WPARAM wparam, LPARAM lparam)
{
	bool handled = false;
	try
	{
		switch (message)
		{
		case SFVM_WINDOWCREATED:
			handled = on_window_created(reinterpret_cast<HWND>(wparam));
			break;

		case SFVM_GETNOTIFY:
			if (wparam == 0 || lparam == 0)
				BOOST_THROW_EXCEPTION(com_error(E_POINTER));
			handled = on_get_notify(
				*reinterpret_cast<PCIDLIST_ABSOLUTE*>(wparam),
				*reinterpret_cast<LONG*>(lparam));
			break;

		case SFVM_FSNOTIFY:
			handled = on_fs_notify(
				reinterpret_cast<PCIDLIST_ABSOLUTE>(wparam), lparam);
			break;

		case SFVM_MERGEMENU:
			if (lparam == 0)
				BOOST_THROW_EXCEPTION(com_error(E_POINTER));
			handled = on_merge_menu(*reinterpret_cast<QCMINFO*>(lparam));
			break;
		  
		case SFVM_SELECTIONCHANGED:
			// wparam's meaning is unknown
			if (lparam == 0)
				BOOST_THROW_EXCEPTION(com_error(E_POINTER));
			handled = on_selection_changed(
				*reinterpret_cast<SFV_SELECTINFO*>(lparam));
			break;

		case SFVM_INITMENUPOPUP:
			handled = on_init_menu_popup(
				LOWORD(wparam), HIWORD(wparam),
				reinterpret_cast<HMENU>(lparam));
			break;

		case SFVM_INVOKECOMMAND:
			handled = on_invoke_command(wparam);
			break;

		case SFVM_GETHELPTEXT:
			handled = on_get_help_text(
				LOWORD(wparam), HIWORD(wparam), 
				reinterpret_cast<LPTSTR>(lparam));
			break;

		case SFVM_GET_WEBVIEW_CONTENT:
			if (lparam == 0)
				BOOST_THROW_EXCEPTION(com_error(E_POINTER));
			handled = on_get_webview_content(
				*reinterpret_cast<SFV_WEBVIEW_CONTENT_DATA*>(lparam));
			break;

		case SFVM_GET_WEBVIEW_TASKS:
			if (lparam == 0)
				BOOST_THROW_EXCEPTION(com_error(E_POINTER));
			handled = on_get_webview_tasks(
				*reinterpret_cast<SFV_WEBVIEW_TASKSECTION_DATA*>(lparam));
			break;

		default:
			handled = false;
			break;
		}

		if (!handled)
		{
			// special treatment fo FSNOTIFY because it uses S_FALSE to
			// suppress default processing.
			if (message == SFVM_FSNOTIFY)
				BOOST_THROW_EXCEPTION(com_error(S_FALSE));
			else
				BOOST_THROW_EXCEPTION(com_error(E_NOTIMPL));
		}
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();

	return S_OK;
}

/**
 * Items currently selected in the folder view.
 */
com_ptr<IDataObject> CExplorerCallback::selection()
{
	com_ptr<IShellBrowser> browser = shell_browser(ole_site());

	return selection_data_object(browser);
}

/**
 * Update the menus to match the current selection.
 */
void CExplorerCallback::update_menus()
{
	if (!m_tools_menu)
		BOOST_THROW_EXCEPTION(std::logic_error("Missing menu"));

	UINT flags;
	int rc; (void)rc;
	// Despite being declared as a BOOL, the return value of EnableMenuItem is
	// not treated that way.  Only a value < 0 (technically -1) is an error.

	Add add(m_hwnd_view, m_folder_pidl);
	flags = add.disabled(selection(), false) ? MF_GRAYED : MF_ENABLED;
	rc = ::EnableMenuItem(
		m_tools_menu, m_first_command_id + MENUIDOFFSET_ADD,
		MF_BYCOMMAND | flags);
	assert(rc > -1);

	Remove remove(m_hwnd_view, m_folder_pidl);
	flags = remove.disabled(selection(), false) ? MF_GRAYED : MF_ENABLED;
	rc = ::EnableMenuItem(
		m_tools_menu, m_first_command_id + MENUIDOFFSET_REMOVE,
		MF_BYCOMMAND | flags);
	assert(rc > -1);
}

}} // namespace swish::shell_folder
