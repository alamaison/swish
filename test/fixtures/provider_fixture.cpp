// Copyright 2009, 2010, 2011, 2013, 2016 Alexander Lamaison

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "provider_fixture.hpp"

#include "test/common_boost/MockConsumer.hpp"

#include "swish/connection/connection_spec.hpp"
#include "swish/connection/session_manager.hpp"
#include "swish/host_folder/host_pidl.hpp"     // create_host_itemid
#include "swish/provider/Provider.hpp"         // CProvider
#include "swish/shell_folder/SftpDataObject.h" // CSftpDataObject
#include "swish/shell_folder/SftpDirectory.h"  // CSftpDirectory

#include <comet/ptr.h> // com_ptr

#include <washer/shell/pidl_array.hpp> // PIDL array wrapper
#include <washer/shell/shell.hpp>      // desktop_folder

#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/shared_ptr.hpp>

#include <string>

using swish::connection::connection_spec;
using swish::connection::session_manager;
using swish::connection::session_reservation;
using swish::host_folder::create_host_itemid;
using swish::provider::CProvider;
using swish::provider::sftp_provider;

using comet::com_ptr;

using namespace washer::shell::pidl;
using washer::shell::desktop_folder;

using boost::numeric_cast;
using boost::shared_ptr;

using std::move;
using std::vector;

using ssh::filesystem::path;

namespace comet
{

template <>
struct comtype<IDataObject>
{
    static const IID& uuid()
    {
        return IID_IDataObject;
    }
    typedef ::IUnknown base;
};
}

namespace test
{
namespace fixtures
{
shared_ptr<sftp_provider> provider_fixture::Provider()
{
    session_reservation ticket(session_manager().reserve_session(
        connection_spec(whost(), wuser(), port()), Consumer(),
        "Running tests"));

    return shared_ptr<CProvider>(new CProvider(move(ticket)));
}

com_ptr<test::MockConsumer> provider_fixture::Consumer()
{
    com_ptr<test::MockConsumer> consumer = new test::MockConsumer();
    consumer->set_pubkey_behaviour(test::MockConsumer::CustomKeys);
    consumer->set_key_files(private_key_path().string(),
                            public_key_path().string());
    return consumer;
}

apidl_t provider_fixture::directory_pidl(const path& directory)
{
    return real_swish_pidl() +
           create_host_itemid(whost(), wuser(), directory, port());
}

apidl_t provider_fixture::sandbox_pidl()
{
    return directory_pidl(sandbox());
}

vector<cpidl_t> provider_fixture::pidls_in_sandbox()
{
    CSftpDirectory dir(sandbox_pidl(), Provider());
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

com_ptr<IDataObject> provider_fixture::data_object_from_sandbox()
{
    vector<cpidl_t> pidls = pidls_in_sandbox();
    pidl_array<cpidl_t> array(pidls.begin(), pidls.end());
    BOOST_REQUIRE_EQUAL(array.size(), 2U);

    com_ptr<IDataObject> data_object =
        new CSftpDataObject(numeric_cast<UINT>(array.size()), array.as_array(),
                            sandbox_pidl().get(), Provider());
    BOOST_REQUIRE(data_object);
    return data_object;
}
}
} // namespace test::fixture
