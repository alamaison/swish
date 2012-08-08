/**
    @file

    Fixture for tests that manipulate remote files via PIDLs.

    @if license

    Copyright (C) 2011, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "PidlFixture.hpp"

#include "swish/host_folder/host_pidl.hpp" // create_host_itemid
#include "swish/shell_folder/SftpDataObject.h" // CSftpDataObject
#include "swish/shell_folder/SftpDirectory.h" // CSftpDirectory
#include "swish/shell_folder/shell.hpp" // desktop_folder
#include "swish/utils.hpp" // Utf8StringToWideString

#include "test/common_boost/helpers.hpp"  // BOOST_REQUIRE_OK

#include <winapi/shell/pidl_array.hpp>  // PIDL array wrapper
#include <winapi/shell/shell.hpp> // desktop_folder

#include <boost/numeric/conversion/cast.hpp> // numeric_cast

#include <string>

using swish::host_folder::create_host_itemid;
using swish::utils::Utf8StringToWideString;

using namespace winapi::shell::pidl;
using winapi::shell::desktop_folder;

using comet::com_ptr;

using boost::filesystem::wpath;
using boost::numeric_cast;

using std::vector;

namespace test { // private

namespace {

    /**
     * Return the PIDL to the Swish HostFolder in Explorer.
     */
    apidl_t swish_pidl()
    {
        com_ptr<IShellFolder> desktop = desktop_folder();

        apidl_t pidl;
        HRESULT hr = desktop->ParseDisplayName(
            NULL, NULL, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\"
            L"::{B816A83A-5022-11DC-9153-0090F5284F85}", NULL, 
            reinterpret_cast<PIDLIST_RELATIVE*>(&pidl), NULL);
        BOOST_REQUIRE_OK(hr);

        return pidl;
    }
}

apidl_t PidlFixture::directory_pidl(const wpath& directory)
{
    return swish_pidl() + create_host_itemid(
        Utf8StringToWideString(GetHost()),
        Utf8StringToWideString(GetUser()),
        directory, GetPort());
}

apidl_t PidlFixture::sandbox_pidl()
{
    return directory_pidl(ToRemotePath(Sandbox()));
}

vector<cpidl_t> PidlFixture::pidls_in_sandbox()
{
    CSftpDirectory dir(sandbox_pidl(), Provider(), Consumer());
    com_ptr<IEnumIDList> pidl_enum = dir.GetEnum(
        SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN);

    vector<cpidl_t> pidls;
    cpidl_t pidl;
    while (pidl_enum->Next(1, pidl.out(), NULL) == S_OK)
    {
        pidls.push_back(pidl);
    }

    return pidls;
}

com_ptr<IDataObject> PidlFixture::data_object_from_sandbox()
{
    vector<cpidl_t> pidls = pidls_in_sandbox();
    pidl_array<cpidl_t> array(pidls.begin(), pidls.end());
    BOOST_REQUIRE_EQUAL(array.size(), 2U);

    com_ptr<IDataObject> data_object = new CSftpDataObject(
        numeric_cast<UINT>(array.size()), array.as_array(),
        sandbox_pidl().get(), Provider(), Consumer());
    BOOST_REQUIRE(data_object);
    return data_object;
}

} // test
