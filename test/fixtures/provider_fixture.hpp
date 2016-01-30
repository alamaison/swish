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

#ifndef SWISH_TEST_FIXTURES_PROVIDER_FIXTURE_HPP
#define SWISH_TEST_FIXTURES_PROVIDER_FIXTURE_HPP

#include "test/common_boost/MockConsumer.hpp"
#include "test/common_boost/SwishPidlFixture.hpp"
#include "test/fixtures/sftp_fixture.hpp"

#include "swish/provider/sftp_provider.hpp"

#include <ssh/filesystem/path.hpp>

#include <washer/shell/pidl.hpp> // apidl_t, cpidl_t

#include <comet/ptr.h> // com_ptr

#include <boost/filesystem/path.hpp> // path
#include <boost/shared_ptr.hpp>

#include <string>

#include <vector>

#include <ObjIdl.h> // IDataObject

namespace test
{
namespace fixtures
{

/** Fixture for tests that need a backend data provider. */
class provider_fixture : virtual public sftp_fixture,
                         public test::SwishPidlFixture // for fake_swish_pidl
{
public:
    /**
     * Get an sftp_provider connected to the fixture SSH server.
     */
    boost::shared_ptr<swish::provider::sftp_provider> Provider();

    /**
     * Get a dummy consumer to use in calls to provider.
     */
    comet::com_ptr<test::MockConsumer> Consumer();

    /**
     * Return an absolute PIDL to a remote directory.
     *
     * We cheat by returning a PIDL to a HostFolder item with the
     * shortcut path set to the remote directory.
     */
    washer::shell::pidl::apidl_t
    directory_pidl(const ssh::filesystem::path& directory);

    /**
     * Return an absolute PIDL to the sandbox on the remote end.
     *
     * This is, of course, the local sandbox but the PIDL points to
     * it via Swish rather than via the local filesystem.
     */
    washer::shell::pidl::apidl_t sandbox_pidl();

    /**
     * Return pidls for all the items in the sandbox directory.
     */
    std::vector<washer::shell::pidl::cpidl_t> pidls_in_sandbox();

    /**
     * Make a DataObject to all the items in the sandbox, via the SFTP
     * connection.
     */
    comet::com_ptr<IDataObject> data_object_from_sandbox();
};
}
} // namespace test::fixture

#endif
