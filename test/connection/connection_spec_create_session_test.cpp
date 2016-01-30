// Copyright 2013, 2016 Alexander Lamaison

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

#include "swish/connection/connection_spec.hpp" // Test subject
#include "swish/connection/authenticated_session.hpp"

#include "test/common_boost/ConsumerStub.hpp"
#include "test/common_boost/helpers.hpp"
#include "test/fixtures/openssh_fixture.hpp"

#include <comet/ptr.h> // com_ptr

#include <boost/test/unit_test.hpp>

#include <exception>
#include <map>

using swish::connection::authenticated_session;
using swish::connection::connection_spec;

using test::fixtures::openssh_fixture;
using test::CConsumerStub;

using comet::com_ptr;

using boost::test_tools::predicate_result;

using std::exception;
using std::map;

namespace
{

class fixture : private openssh_fixture
{
public:
    connection_spec get_connection()
    {
        return connection_spec(whost(), wuser(), port());
    }

    com_ptr<ISftpConsumer> consumer()
    {
        com_ptr<CConsumerStub> consumer =
            new CConsumerStub(private_key_path(), public_key_path());
        return consumer;
    }

    /**
     * Check that the given session responds sensibly to a request.
     */
    predicate_result alive(authenticated_session& session)
    {
        try
        {
            session.get_sftp_filesystem().directory_iterator("/");

            predicate_result res(true);
            res.message() << "Provider seems to be alive";
            return res;
        }
        catch (const exception& e)
        {
            predicate_result res(false);
            res.message() << "Provider seems to be dead: " << e.what();
            return res;
        }
    }
};
}

BOOST_FIXTURE_TEST_SUITE(connection_spec_session_create, fixture)

BOOST_AUTO_TEST_CASE(create)
{
    authenticated_session session(get_connection().create_session(consumer()));
    BOOST_CHECK(alive(session));
}

BOOST_AUTO_TEST_SUITE_END()
