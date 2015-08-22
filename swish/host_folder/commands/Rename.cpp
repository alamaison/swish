/* Copyright (C) 2015  Alexander Lamaison <swish@lammy.co.uk>

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

#include "Rename.hpp"

#include "swish/shell_folder/data_object/ShellDataObject.hpp" // PidlFormat
#include "swish/shell/shell.hpp" // put_view_item_into_rename_mode

#include <washer/shell/services.hpp> // shell_browser, shell_view

#include <comet/error.h> // com_error
#include <comet/ptr.h> // com_ptr
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/locale.hpp> // translate
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <string>

using swish::nse::Command;
using swish::nse::command_site;
using swish::shell_folder::data_object::PidlFormat;
using swish::shell::put_view_item_into_rename_mode;

using washer::shell::pidl::apidl_t;
using washer::shell::shell_browser;
using washer::shell::shell_view;

using comet::com_error;
using comet::com_ptr;
using comet::uuid_t;

using boost::locale::translate;

using std::wstring;

namespace swish {
namespace host_folder {
namespace commands {

namespace {
    const uuid_t RENAME_COMMAND_ID("b816a883-5022-11dc-9153-0090f5284f85");
}

Rename::Rename() :
    Command(
        translate(L"&Rename SFTP Connection"), RENAME_COMMAND_ID,
        translate(L"Rename an SFTP connection created with Swish."),
        L"shell32.dll,133", translate(L"&Rename SFTP Connection..."),
        translate(L"Rename Connection"))
    {}

BOOST_SCOPED_ENUM(Command::state) Rename::state(
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
        // we don't support multi-host renaming.
        return state::disabled;
    }
}

// This command just puts the item into rename (edit) mode.  When the user
// finishes typing the new name, the shell takes care of performing the rest of
// the renaming process by calling SetNameOf() on the HostFolder
void Rename::operator()(
    const com_ptr<IDataObject>& selection, const command_site& site,
    const com_ptr<IBindCtx>&)
const
{
    PidlFormat format(selection);
    if (format.pidl_count() != 1)
        BOOST_THROW_EXCEPTION(com_error(E_FAIL));

    com_ptr<IShellView> view = shell_view(shell_browser(site.ole_site()));

    apidl_t pidl_selected = format.file(0);

    put_view_item_into_rename_mode(view, pidl_selected.last_item());
}

}}} // namespace swish::host_folder::commands
