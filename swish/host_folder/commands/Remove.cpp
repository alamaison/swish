/**
    @file

    Remove host command.

    @if license

    Copyright (C) 2010, 2011, 2012, 2013
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

#include "Remove.hpp"

#include "swish/host_folder/host_management.hpp" // RemoveConnectionFromRegistry
#include "swish/host_folder/host_pidl.hpp" // find_host_itemid, host_item_view
#include "swish/shell_folder/data_object/ShellDataObject.hpp" // PidlFormat

#include <comet/error.h> // com_error
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/locale.hpp> // translate
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <string>

using swish::nse::Command;
using swish::shell_folder::data_object::PidlFormat;
using swish::host_folder::find_host_itemid;
using swish::host_folder::host_itemid_view;
using swish::host_folder::host_management::RemoveConnectionFromRegistry;

using winapi::shell::pidl::apidl_t;

using comet::com_error;
using comet::com_ptr;
using comet::uuid_t;

using boost::locale::translate;

using std::wstring;

namespace swish {
namespace host_folder {
namespace commands {

namespace {
    const uuid_t REMOVE_COMMAND_ID("b816a881-5022-11dc-9153-0090f5284f85");

    /**
     * Cause Explorer to refresh any windows displaying the owning folder.
     *
     * Inform shell that something in our folder changed (we don't know
     * exactly what the new PIDL is until we reload from the registry, hence
     * UPDATEDIR).
     */
    void notify_shell(const apidl_t folder_pidl)
    {
        assert(folder_pidl);
        ::SHChangeNotify(
            SHCNE_UPDATEDIR, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT,
            folder_pidl.get(), NULL);
    }
}

Remove::Remove(HWND hwnd, const apidl_t& folder_pidl) :
    Command(
        translate(L"&Remove SFTP Connection"), REMOVE_COMMAND_ID,
        translate(L"Remove a SFTP connection created with Swish."),
        L"shell32.dll,-240", translate(L"&Remove SFTP Connection..."),
        translate(L"Remove Connection")),
    m_hwnd(hwnd), m_folder_pidl(folder_pidl) {}

BOOST_SCOPED_ENUM(Command::state) Remove::state(
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
        return state::enabled;
    case 0:
        return state::hidden;
    default:
        // This means multiple items are selected. We disable rather than
        // hide the buttons to let the user know the option exists but that
        // we don't support multi-host removal yet.

        // TODO: support multi-host removal
        return state::disabled;
    }
}

void Remove::operator()(
    const com_ptr<IDataObject>& data_object, const com_ptr<IBindCtx>&)
const
{
    PidlFormat format(data_object);
    // XXX: for the moment we only allow removing one item.
    //      is this what we want?
    if (format.pidl_count() != 1)
        BOOST_THROW_EXCEPTION(com_error(E_FAIL));

    apidl_t pidl_selected = format.file(0);
    wstring label = host_itemid_view(*find_host_itemid(pidl_selected)).label();
    assert(!label.empty());
    if (label.empty())
        BOOST_THROW_EXCEPTION(com_error(E_UNEXPECTED));

    RemoveConnectionFromRegistry(label);
    notify_shell(m_folder_pidl);
}

}}} // namespace swish::host_folder::commands
