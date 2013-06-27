/**
 * @file
 *
 * Tests for the running_session class.
 */

#include "test/common_boost/fixtures.hpp" // WinsockFixture
#include "test/common_boost/remote_test_config.hpp" // remote_test_config

#include "swish/provider/running_session.hpp"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <vector>

using test::remote_test_config;
using test::WinsockFixture;

using boost::make_shared;
using boost::shared_ptr;

using std::vector;

BOOST_FIXTURE_TEST_SUITE( CSession_tests, WinsockFixture )

/**
 * Test that connecting succeeds.
 */
BOOST_AUTO_TEST_CASE( connect )
{
    remote_test_config config;

    running_session session(config.GetHost(), config.GetPort());
}

BOOST_AUTO_TEST_CASE( multiple_connections )
{
    remote_test_config config;

    vector<shared_ptr<running_session>> sessions;
    for (int i = 0; i < 5; i++)
    {
        sessions.push_back(
            make_shared<running_session>(config.GetHost(), config.GetPort()));
    }
}

/** 
 * Test that trying to start SFTP channel before authenticating fails.
 */
BOOST_AUTO_TEST_CASE( start_sftp_too_early )
{
    remote_test_config config;

    running_session session(config.GetHost(), config.GetPort());
    BOOST_CHECK_THROW(session.StartSftp(), comet::com_error);
}

BOOST_AUTO_TEST_SUITE_END()
