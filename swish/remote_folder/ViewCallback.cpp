/**
    @file

    Handler for remote folder's interaction with Explorer Shell Folder View.

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

#include "swish/frontend/UserInteraction.hpp" // CUserInteraction
#include "swish/remote_folder/commands/commands.hpp" // NewFolder
#include "swish/remote_folder/pidl_connection.hpp" // provider_from_pidl

#include <winapi/error.hpp> // last_error

#include <boost/bind.hpp> // bind
#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert

using swish::frontend::CUserInteraction;
using swish::nse::IEnumUICommand;
using swish::nse::IUIElement;
using swish::remote_folder::provider_from_pidl;
using swish::remote_folder::commands::remote_folder_task_pane_tasks;
using swish::remote_folder::commands::remote_folder_task_pane_titles;

using comet::com_ptr;

using winapi::last_error;
using winapi::shell::pidl::apidl_t;

using boost::bind;
using boost::enable_error_info;
using boost::errinfo_api_function;

using std::pair;

namespace swish {
namespace remote_folder {

namespace {

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
    m_folder_pidl(folder_pidl), m_hwnd_view(NULL) {}

/**
 * The folder window is being created.
 *
 * The shell is notifying us of the folder view's window handle.
 */
bool CViewCallback::on_window_created(HWND hwnd_view)
{
    m_hwnd_view = hwnd_view;
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
    events = SHCNE_CREATE | SHCNE_DELETE | SHCNE_MKDIR | SHCNE_RMDIR |
        SHCNE_UPDATEITEM | SHCNE_UPDATEDIR | SHCNE_RENAMEITEM |
        SHCNE_RENAMEFOLDER;
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
        remote_folder_task_pane_titles(m_hwnd_view, m_folder_pidl);

    content_out.pExtraTasksExpando = tasks.first.detach();
    content_out.pFolderTasksExpando = tasks.second.detach();
    return true;
}

namespace {
    com_ptr<ISftpConsumer> consumer(HWND hwnd)
    {
        return new CUserInteraction(hwnd);
    }
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
        remote_folder_task_pane_tasks(
            m_hwnd_view, m_folder_pidl, ole_site(),
            bind(provider_from_pidl, m_folder_pidl, _1),
            bind(&consumer, m_hwnd_view));

    tasks_out.pEnumExtraTasks = commands.first.detach();
    tasks_out.pEnumFolderTasks = commands.second.detach();
    return true;
}

}} // namespace swish::remote_folder
