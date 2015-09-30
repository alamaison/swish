/* Copyright (C) 2010, 2011, 2012, 2013, 2015
   Alexander Lamaison <swish@lammy.co.uk>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation, either version 3 of the License, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Remove.hpp"

#include "swish/host_folder/host_management.hpp" // RemoveConnectionFromRegistry
#include "swish/host_folder/host_pidl.hpp" // find_host_itemid, host_item_view
#include "swish/shell/parent_and_item.hpp"
#include "swish/shell/shell_item_array.hpp"

#include <comet/error.h> // com_error
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/locale.hpp> // translate
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <string>

using swish::nse::Command;
using swish::nse::command_site;
using swish::host_folder::find_host_itemid;
using swish::host_folder::host_itemid_view;
using swish::host_folder::host_management::RemoveConnectionFromRegistry;

using washer::shell::pidl::apidl_t;

using comet::com_error;
using comet::com_ptr;
using comet::uuid_t;

using boost::locale::translate;
using boost::optional;

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
    void notify_shell(const apidl_t& folder_pidl)
    {
        ::SHChangeNotify(
            SHCNE_UPDATEDIR, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT,
            folder_pidl.get(), NULL);
    }
}

Remove::Remove(const apidl_t& folder_pidl) :
    Command(
        translate(L"&Remove SFTP Connection"), REMOVE_COMMAND_ID,
        translate(L"Remove a SFTP connection created with Swish."),
        L"shell32.dll,-240", translate(L"&Remove SFTP Connection..."),
        translate(L"Remove Connection")),
    m_folder_pidl(folder_pidl) {}

BOOST_SCOPED_ENUM(Command::state) Remove::state(
    com_ptr<IShellItemArray> selection, bool /*ok_to_be_slow*/)
const
{
    if (!selection)
    {
        // Selection unknown.
        return state::hidden;
    }

    switch (selection->size())
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
    com_ptr<IShellItemArray> selection, const command_site& site, com_ptr<IBindCtx>)
const
{
    // XXX: for the moment we only allow removing one item.
    //      is this what we want?
    if (selection->size() != 1)
        BOOST_THROW_EXCEPTION(com_error(E_FAIL));

    com_ptr<IShellItem> item = selection->at(0);
    com_ptr<IParentAndItem> folder_and_pidls = try_cast(item);
    apidl_t selected_item = folder_and_pidls->absolute_item_pidl();

    wstring label = host_itemid_view(*find_host_itemid(selected_item)).label();
    assert(!label.empty());
    if (label.empty())
        BOOST_THROW_EXCEPTION(com_error(E_UNEXPECTED));

    RemoveConnectionFromRegistry(label);
    notify_shell(m_folder_pidl);
}

}}} // namespace swish::host_folder::commands
