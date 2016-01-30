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
#include "test/fixtures/openssh_fixture.hpp"

#include "swish/provider/sftp_provider.hpp"

#include <comet/ptr.h> // com_ptr

#include <boost/filesystem/path.hpp> // path
#include <boost/shared_ptr.hpp>

#include <string>

namespace swish
{
namespace connection
{

class authenticated_session;
}
}

namespace test
{
namespace fixtures
{

/** Fixture for tests that need a backend data provider. */
class provider_fixture : virtual public openssh_fixture
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
};
}
} // namespace test::fixture

#endif
