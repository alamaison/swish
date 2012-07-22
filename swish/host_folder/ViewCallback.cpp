/**
    @file

    Handler for host folder's interaction with Explorer Shell Folder View.

    @if license

    Copyright (C) 2008, 2009, 2010, 2011, 2012
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

#include "ViewCallback.hpp"

#include "swish/host_folder/commands/commands.hpp" // host commands
#include "swish/host_folder/commands/Add.hpp"
#include "swish/host_folder/commands/Remove.hpp"
#include "swish/nse/Command.hpp" // MenuCommandTitleAdapter

#include <winapi/error.hpp> // last_error
#include <winapi/gui/menu/basic_menu.hpp> // find_first_item_with_id
#include <winapi/gui/menu/button/string_button_description.hpp>
#include <winapi/gui/menu/item/command_item_description.hpp>
#include <winapi/gui/menu/item/command_item.hpp>
#include <winapi/gui/menu/item/item_state.hpp> // selectability
#include <winapi/gui/menu/item/separator_item.hpp>
#include <winapi/gui/menu/item/sub_menu_item.hpp>
#include <winapi/shell/services.hpp> // shell_browser, shell_view
#include <winapi/trace.hpp> // trace

#include <boost/exception/diagnostic_information.hpp> // diagnostic_information
#include <boost/exception/errinfo_file_name.hpp> // errinfo_file_name
#include <boost/lambda/lambda.hpp> // _1
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <algorithm> // find_if
#include <cassert> // assert
#include <stdexcept> // logic_error
#include <string>

using swish::frontend::winsparkle_shower;
using swish::host_folder::commands::Add;
using swish::host_folder::commands::Remove;
using swish::host_folder::commands::host_folder_task_pane_tasks;
using swish::host_folder::commands::host_folder_task_pane_titles;
using swish::nse::IEnumUICommand;
using swish::nse::IUIElement;
using swish::nse::MenuCommandTitleAdapter;

using comet::com_ptr;

using namespace winapi::gui::menu;
using winapi::last_error;
using winapi::shell::pidl::apidl_t;
using winapi::shell::shell_browser;
using winapi::shell::shell_view;
using winapi::trace;

using boost::diagnostic_information;
using boost::enable_error_info;
using boost::errinfo_api_function;

using std::find_if;
using std::pair;
using std::wstring;

template<> struct comet::comtype<IDataObject>
{
	static const IID& uuid() throw() { return IID_IDataObject; }
	typedef IUnknown base;
};

namespace swish {
namespace host_folder {

namespace {

	// Menu command ID offsets for Explorer Tools menu
	enum MENUOFFSET
	{
		MENUIDOFFSET_FIRST = 0,
		MENUIDOFFSET_ADD = MENUIDOFFSET_FIRST,
		MENUIDOFFSET_REMOVE,
		MENUIDOFFSET_LAST = MENUIDOFFSET_REMOVE
	};

	template<typename DescriptionType, typename HandleCreator>
	item item_from_menu(
		const basic_menu<DescriptionType, HandleCreator>& parent_menu,
		UINT menu_id)
	{
		return *find_first_item_with_id(
			parent_menu.begin(), parent_menu.end(), menu_id);
	}

	/**
	 * Get handle to explorer 'Tools' menu.
	 *
	 * The menu we want to insert into is actually the @e submenu of the
	 * Tools menu @e item.  Confusing!
	 */
	item tools_menu_with_fallback(const menu_bar& parent_menu)
	{
		try
		{
			return item_from_menu(parent_menu, FCIDM_MENU_TOOLS);
		}
		catch (const std::exception& e)
		{
			trace("Failed getting tools menu: %s") % diagnostic_information(e);

			// Fall back to using File menu
			return item_from_menu(parent_menu, FCIDM_MENU_FILE);
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
CViewCallback::CViewCallback(const apidl_t& folder_pidl) :
	m_folder_pidl(folder_pidl), m_hwnd_view(NULL), m_first_command_id(0),
	m_winsparkle(
		"http://www.swish-sftp.org/autoupdate/appcast.xml", L"Swish",
		L"0.6.3", L"", "Software\\Swish\\Updates") {}

/**
 * The folder window is being created.
 *
 * The shell is notifying us of the folder view's window handle.
 */
bool CViewCallback::on_window_created(HWND hwnd_view)
{
	m_hwnd_view = hwnd_view;

	if (hwnd_view)
		m_winsparkle.show();

	return true;
}

/**
 * Tell the shell that we might notify it of update events that apply to this
 * folder (specified using our absolute PIDL).
 *
 * We are notified via SFVM_FSNOTIFY if any events indicated here occurr.
 *
 * @TODO: It's possible that @a events already has events set in its bitmask
 *        so maybe we should 'or' our extra events with it.
 */
bool CViewCallback::on_get_notify(
	PCIDLIST_ABSOLUTE& pidl_monitor, LONG& events)
{
	events = SHCNE_UPDATEDIR | SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER |
		SHCNE_DELETE | SHCNE_MKDIR | SHCNE_RMDIR;
	pidl_monitor = m_folder_pidl.get(); // Owned by us
	return true;
}

/**
 * The shell is telling us that an event (probably a SHChangeNotify of some
 * sort) has affected one of our item.  Just nod. If we don't it doesn't work.
 */
bool CViewCallback::on_fs_notify(
	PCIDLIST_ABSOLUTE /*pidl*/, LONG /*event*/)
{
	return true;
}

class merge_command_items
{
public:

	typedef void result_type;

	merge_command_items(HWND hwnd_view, apidl_t pidl, UINT first_command_id)
		: m_hwnd_view(hwnd_view), m_folder_pidl(pidl),
		  m_first_command_id(first_command_id) {}

	void operator()(sub_menu_item& sub_menu)
	{
		menu::iterator insert_position = sub_menu.menu().begin();
		insert_position += 2;
		MenuCommandTitleAdapter<Add> add(m_hwnd_view, m_folder_pidl);

		command_item_description add_item(
			string_button_description(add.title(NULL).c_str()),
			m_first_command_id + MENUIDOFFSET_ADD);
		sub_menu.menu().insert(add_item, insert_position++);

		MenuCommandTitleAdapter<Remove> remove(m_hwnd_view, m_folder_pidl);

		command_item_description remove_item(
			string_button_description(remove.title(NULL).c_str()),
			m_first_command_id + MENUIDOFFSET_REMOVE);
		remove_item.selectability(selectability::disabled);
		sub_menu.menu().insert(remove_item, insert_position++);
	}

	void operator()(command_item&)
	{
		BOOST_THROW_EXCEPTION(
			std::logic_error("Cannot insert into command item"));
	}

	void operator()(separator_item&)
	{
		BOOST_THROW_EXCEPTION(
			std::logic_error("Cannot insert into separator"));
	}

private:
	HWND m_hwnd_view;
	apidl_t m_folder_pidl;
	UINT m_first_command_id;
};

bool CViewCallback::on_merge_menu(QCMINFO& menu_info)
{
	assert(menu_info.idCmdFirst >= FCIDM_SHVIEWFIRST);
	assert(menu_info.idCmdLast <= FCIDM_SHVIEWLAST);
	//assert(::IsMenu(menu_info.hmenu));
	m_first_command_id = menu_info.idCmdFirst;

	// Try to get a handle to the  Explorer Tools menu and insert 
	// add and remove connection menu items into it if we find it
	m_tools_menu = tools_menu_with_fallback(
		menu_handle::foster_handle(menu_info.hmenu));
	m_tools_menu->accept(
		merge_command_items(m_hwnd_view, m_folder_pidl, m_first_command_id));

	// Return value of last menu ID plus 1
	menu_info.idCmdFirst += MENUIDOFFSET_LAST + 1; // Added 2 items

	return true;

	// I would have expected to have to remove these menu items
	// in SFVM_UNMERGEMENU but this seems to happen automatically
}

bool CViewCallback::on_selection_changed(
	SFV_SELECTINFO& /*selection_info*/)
{
	update_menus();
	return true;
}

bool CViewCallback::on_init_menu_popup(
	UINT /*first_command_id*/, int /*menu_index*/, HMENU /*menu*/)
{
	update_menus();
	return true;
}

bool CViewCallback::on_invoke_command(UINT command_id)
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

bool CViewCallback::on_get_help_text(
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

bool CViewCallback::on_get_webview_content(
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

bool CViewCallback::on_get_webview_tasks(
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
 * Items currently selected in the folder view.
 */
com_ptr<IDataObject> CViewCallback::selection()
{
	com_ptr<IShellBrowser> browser = shell_browser(ole_site());

	return selection_data_object(browser);
}

class update_command_items
{
public:

	typedef void result_type;

	update_command_items(
		HWND hwnd_view, apidl_t pidl, com_ptr<IDataObject> selection,
		UINT first_command_id)
		: m_hwnd_view(hwnd_view), m_folder_pidl(pidl), m_selection(selection),
		  m_first_command_id(first_command_id) {}

	class selectability_setter
	{
	public:
		typedef void result_type;

		selectability_setter(BOOST_SCOPED_ENUM(selectability) selectability)
			: m_selectability(selectability) {}

		void operator()(command_item& item)
		{
			item.selectability(m_selectability);
		}

		template<typename T>
		void operator()(T&)
		{
			BOOST_THROW_EXCEPTION(
				std::logic_error("Unexpected menu item type"));
		}

	private:
		BOOST_SCOPED_ENUM(selectability) m_selectability;
	};

	void operator()(sub_menu_item& sub_menu)
	{
		Add add(m_hwnd_view, m_folder_pidl);
		BOOST_SCOPED_ENUM(selectability) add_state =
			add.disabled(m_selection, false) ?
			selectability::disabled : selectability::enabled;

		item add_item = item_from_menu(
			sub_menu.menu(), m_first_command_id + MENUIDOFFSET_ADD);
		add_item.accept(selectability_setter(add_state));

		Remove remove(m_hwnd_view, m_folder_pidl);
		BOOST_SCOPED_ENUM(selectability) remove_state =
			remove.disabled(m_selection, false) ?
			selectability::disabled : selectability::enabled;

		item remove_item = item_from_menu(
			sub_menu.menu(), m_first_command_id + MENUIDOFFSET_REMOVE);
		remove_item.accept(selectability_setter(remove_state));
	}

	void operator()(command_item&)
	{
		BOOST_THROW_EXCEPTION(
			std::logic_error("Cannot insert into command item"));
	}

	void operator()(separator_item&)
	{
		BOOST_THROW_EXCEPTION(
			std::logic_error("Cannot insert into separator"));
	}

private:
	HWND m_hwnd_view;
	apidl_t m_folder_pidl;
	com_ptr<IDataObject> m_selection;
	UINT m_first_command_id;
};

/**
 * Update the menus to match the current selection.
 */
void CViewCallback::update_menus()
{
	if (!m_tools_menu)
		BOOST_THROW_EXCEPTION(std::logic_error("Missing menu"));

	m_tools_menu->accept(
		update_command_items(
			m_hwnd_view, m_folder_pidl, selection(), m_first_command_id));
}

}} // namespace swish::host_folder
