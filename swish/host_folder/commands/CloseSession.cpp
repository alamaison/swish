/**
    @file

    Ending running sessions.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "CloseSession.hpp"

#include "swish/connection/session_pool.hpp"
#include "swish/shell_folder/data_object/ShellDataObject.hpp" // PidlFormat
#include "swish/remote_folder/pidl_connection.hpp" // connection_from_pidl

#include <comet/error.h> // com_error
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/locale.hpp> // translate
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert

using swish::connection::session_pool;
using swish::nse::Command;
using swish::shell_folder::data_object::PidlFormat;
using swish::remote_folder::connection_from_pidl;

using winapi::shell::pidl::apidl_t;

using comet::com_error;
using comet::com_ptr;
using comet::uuid_t;

using boost::locale::translate;

namespace swish {
namespace host_folder {
namespace commands {

namespace {
    const uuid_t CLOSE_SESSION_COMMAND_ID("b816a886-5022-11dc-9153-0090f5284f85");

    /**
     * Cause Explorer to refresh the UI view of the given item.
     */
    void notify_shell(const apidl_t item)
    {
        ::SHChangeNotify(
            SHCNE_UPDATEITEM, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT,
            item.get(), NULL);
    }
}

CloseSession::CloseSession(HWND hwnd, const apidl_t& folder_pidl) :
    Command(
        translate("&Close SFTP connection"), CLOSE_SESSION_COMMAND_ID,
        translate("Close the authenticated connection to the server."),
        L"shell32.dll,-11", translate("&Close SFTP Connection..."),
        translate("Close Connection")),
    m_hwnd(hwnd), m_folder_pidl(folder_pidl) {}

BOOST_SCOPED_ENUM(Command::state) CloseSession::state(
    const comet::com_ptr<IDataObject>& data_object, bool /*ok_to_be_slow*/)
const
{
    if (!data_object)
    {
        // Selection unknown.
        return state::hidden;
    }

    PidlFormat format(data_object);
    switch (format.pidl_count())
    {
    case 1:
        if (session_pool().has_session(connection_from_pidl(format.file(0))))
            return state::enabled;
        else
            return state::hidden;
    case 0:
        return state::hidden;
    default:
        // This means multiple items are selected. We disable rather than
        // hide the buttons to let the user know the option exists but that
        // we don't support multi-host session closure.
        return state::disabled;
    }
}

void CloseSession::operator()(
    const com_ptr<IDataObject>& data_object, const com_ptr<IBindCtx>&)
const
{
    PidlFormat format(data_object);
    if (format.pidl_count() != 1)
        BOOST_THROW_EXCEPTION(com_error(E_FAIL));

    apidl_t pidl_selected = format.file(0);

    session_pool().remove_session(connection_from_pidl(pidl_selected));

    notify_shell(pidl_selected);
}

}}} // namespace swish::host_folder::commands
