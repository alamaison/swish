/**
    @file

    Add host command.

    @if license

    Copyright (C) 2010, 2011, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "Add.hpp"

#include "swish/forms/add_host.hpp" // add_host
#include "swish/host_folder/host_management.hpp" // AddConnectionToRegistry

#include <comet/error.h> // com_error
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/locale.hpp> // translate
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <string>
#include <utility> // make_pair

using swish::forms::add_host;
using swish::forms::host_info;
using swish::nse::Command;
using swish::host_folder::host_management::AddConnectionToRegistry;
using swish::host_folder::host_management::ConnectionExists;

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

Add::Add(HWND hwnd, const apidl_t& folder_pidl) :
    Command(
        translate("&Add SFTP Connection"), ADD_COMMAND_ID,
        translate("Create a new SFTP connection with Swish."),
        L"shell32.dll,-258", translate("&Add SFTP Connection..."),
        translate("Add Connection")),
    m_hwnd(hwnd), m_folder_pidl(folder_pidl) {}

bool Add::disabled(
    const comet::com_ptr<IDataObject>& /*data_object*/, bool /*ok_to_be_slow*/)
const
{ return false; }

bool Add::hidden(
    const comet::com_ptr<IDataObject>& /*data_object*/, bool /*ok_to_be_slow*/)
const
{ return false; }

/** Display dialog to get connection info from user. */
void Add::operator()(const com_ptr<IDataObject>&, const com_ptr<IBindCtx>&)
const
{
    host_info info = add_host(m_hwnd);

    if (ConnectionExists(info.name))
        BOOST_THROW_EXCEPTION(com_error(E_FAIL));

    AddConnectionToRegistry(
        info.name, info.host, info.port, info.user, info.path);

    notify_shell(m_folder_pidl);
}

}}} // namespace swish::host_folder::commands
