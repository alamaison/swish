/**
    @file

    Handler for host folder's interaction with Explorer Shell Folder View.

    @if license

    Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013
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
#include "swish/utils.hpp" // Utf8StringToWideString
#include "swish/versions/version.hpp" // release_version

#include <washer/error.hpp> // last_error
#include <washer/shell/services.hpp> // shell_browser, shell_view
#include <washer/window/window.hpp>

#include <boost/exception/errinfo_api_function.hpp>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <string>
#include <utility> // pair

using swish::host_folder::commands::host_folder_task_pane_tasks;
using swish::host_folder::commands::host_folder_task_pane_titles;
using swish::nse::IEnumUICommand;
using swish::nse::IUIElement;
using swish::release_version;
using swish::utils::Utf8StringToWideString;

using comet::com_ptr;

using washer::last_error;
using washer::shell::pidl::apidl_t;
using washer::shell::shell_browser;
using washer::shell::shell_view;;
using washer::window::window;
using washer::window::window_handle;

using boost::enable_error_info;
using boost::errinfo_api_function;

using std::auto_ptr;
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
 * @param folder  The folder for whom we are creating this callback object.
 */
CViewCallback::CViewCallback(const apidl_t& folder) :
    m_folder(folder),
    m_winsparkle(
        "http://www.swish-sftp.org/autoupdate/appcast.xml", L"Swish",
        Utf8StringToWideString(release_version().as_string()),
        L"", "Software\\Swish\\Updates") {}

/**
 * The folder window is being created.
 *
 * The shell is notifying us of the folder view's window handle.
 */
bool CViewCallback::on_window_created(HWND hwnd_view)
{
    if (hwnd_view)
    {
        m_view = window<wchar_t>(window_handle::foster_handle(hwnd_view));
    }

    if (m_view)
    {
        m_winsparkle.show();
    }

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
    events = SHCNE_UPDATEDIR | SHCNE_UPDATEITEM | SHCNE_RENAMEITEM |
        SHCNE_RENAMEFOLDER | SHCNE_DELETE | SHCNE_MKDIR | SHCNE_RMDIR;
    pidl_monitor = m_folder.get(); // Owned by us
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


bool CViewCallback::on_merge_menu(QCMINFO& menu_info)
{
    m_menu_manager = auto_ptr<menu_command_manager>(
        new menu_command_manager(menu_info, m_view, m_folder));

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
    return m_menu_manager->invoke(command_id, selection());
}

#pragma warning(push)
#pragma warning(disable:4996) // std::copy ... may be unsafe

bool CViewCallback::on_get_help_text(
    UINT command_id, UINT buffer_size, LPTSTR buffer)
{
    wstring help_text;
    bool handled = m_menu_manager->help_text(
        command_id, help_text, selection());

    if (handled)
    {
        size_t copied = help_text.copy(buffer, buffer_size - 1);
        buffer[copied] = _T('\0');

        return true;
    }
    else
    {
        return false;
    }
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
        host_folder_task_pane_titles(
            (m_view) ? m_view->hwnd() : NULL, m_folder);

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
        host_folder_task_pane_tasks(
            (m_view) ? m_view->hwnd() : NULL, m_folder);

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

/**
 * Update the menus to match the current selection.
 */
void CViewCallback::update_menus()
{
    m_menu_manager->update_state(selection());
}

}} // namespace swish::host_folder
