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

#include "Add.hpp"

#include "swish/forms/add_host.hpp" // add_host
#include "swish/host_folder/host_management.hpp" // AddConnectionToRegistry
#include "swish/shell_folder/shell.hpp" // window_for_ole_site

#include <comet/error.h> // com_error
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/locale.hpp> // translate
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <string>
#include <utility> // make_pair

#include <shlobj.h> // SHChangeNotify

using swish::forms::add_host;
using swish::forms::host_info;
using swish::nse::Command;
using swish::host_folder::host_management::AddConnectionToRegistry;
using swish::host_folder::host_management::ConnectionExists;
using swish::shell_folder::window_for_ole_site;

using washer::shell::pidl::apidl_t;
using washer::window::window;

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
    const uuid_t ADD_COMMAND_ID(L"b816a880-5022-11dc-9153-0090f5284f85");

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

Add::Add(const apidl_t& folder_pidl) :
    Command(
        translate(L"&Add SFTP Connection"), ADD_COMMAND_ID,
        translate(L"Create a new SFTP connection with Swish."),
        L"shell32.dll,-258", translate(L"&Add SFTP Connection..."),
        translate(L"Add Connection")),
    m_folder_pidl(folder_pidl) {}

BOOST_SCOPED_ENUM(Command::state) Add::state(
    const comet::com_ptr<IDataObject>& /*data_object*/, bool /*ok_to_be_slow*/)
const
{
    return state::enabled;
}

/** Display dialog to get connection info from user. */
void Add::operator()(
    const com_ptr<IDataObject>&, const com_ptr<IUnknown>& ole_site,
    const com_ptr<IBindCtx>&)
const
{
    optional<window<wchar_t>> view_window = window_for_ole_site(
        ole_site);
    if (view_window)
    {
        host_info info = add_host(view_window->hwnd());

        if (ConnectionExists(info.name))
            BOOST_THROW_EXCEPTION(com_error(E_FAIL));

        AddConnectionToRegistry(
            info.name, info.host, info.port, info.user, info.path);

        notify_shell(m_folder_pidl);
    }
}

}}} // namespace swish::host_folder::commands
