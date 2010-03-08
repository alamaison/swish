/**
    @file

    Tests for host_key object.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.

    @endif
*/

#include "openssh_fixture.hpp" // openssh_fixture

#include <ssh/host_key.hpp> // test subject
#include <ssh/session.hpp> // session

#include <boost/asio/ip/tcp.hpp> // Boost sockets
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/lexical_cast.hpp> // lexical_cast
#include <boost/system/system_error.hpp> // system_error
#include <boost/test/unit_test.hpp>

#include <sstream> // ostringstream
#include <string>

#include <libssh2.h>

using ssh::host_key::host_key;
using ssh::session::session;

using test::ssh::openssh_fixture;

using boost::asio::error::host_not_found;
using boost::asio::ip::tcp;
using boost::lexical_cast;
using boost::system::error_code;
using boost::system::system_error;

using std::ostringstream;
using std::pair;
using std::string;

class session_fixture : public openssh_fixture
{
public:
	session_fixture() : m_io(0), m_socket(m_io) {}

	boost::asio::ip::tcp::socket& open_socket(
		const std::string host_name, int port)
	{
		tcp::resolver resolver(m_io);
		tcp::endpoint endpoint;
		typedef tcp::resolver::query Lookup;
		Lookup query(
			endpoint.protocol(), host_name, lexical_cast<string>(port), 
			Lookup::all_matching | Lookup::numeric_service);

		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		tcp::resolver::iterator end;

		error_code error = host_not_found;
		while (error && endpoint_iterator != end)
		{
			m_socket.close();
			m_socket.connect(*endpoint_iterator++, error);
		}
		if (error)
			BOOST_THROW_EXCEPTION(system_error(error));

		return m_socket;
	}

	session test_session()
	{
		boost::asio::ip::tcp::socket& sock = open_socket(host(), port());
		return session(sock.native());
	}

private:
	boost::asio::io_service m_io; ///< Boost IO system
	boost::asio::ip::tcp::socket m_socket;
};

const string EXPECTED_HOSTKEY =
	"AAAAB3NzaC1yc2EAAAABIwAAAQEArrr/JuJmaZligyfS8vcNur+mWR2ddDQtVdhHzdKU"
	"UoR6/Om6cvxpe61H1YZO1xCpLUBXmkki4HoNtYOpPB2W4V+8U4BDeVBD5crypEOE1+7B"
	"Am99fnEDxYIOZq2/jTP0yQmzCpWYS3COyFmkOL7sfX1wQMeW5zQT2WKcxC6FSWbhDqrB"
	"eNEGi687hJJoJ7YXgY/IdiYW5NcOuqRSWljjGS3dAJsHHWk4nJbhjEDXbPaeduMAwQU9"
	"i6ELfP3r+q6wdu0P4jWaoo3De1aYxnToV/ldXykpipON4NPamsb6Ph2qlJQKypq7J4iQ"
	"gkIIbCU1A31+4ExvcIVoxLQw/aTSbw==";

BOOST_FIXTURE_TEST_SUITE(host_key_tests, session_fixture)

namespace {

	std::string base64_decode(
		boost::shared_ptr<LIBSSH2_SESSION> session, const std::string input)
	{
		char* data;
		unsigned int data_len;
		int rc = libssh2_base64_decode(
			session.get(), &data, &data_len, input.data(), input.size());
		if (rc)
			BOOST_THROW_EXCEPTION(
				ssh::exception::last_error(session) <<
				boost::errinfo_api_function("libssh2_base64_decode"));

		string out(data, data_len);
		free(data);
		return out;
	}
}

/**
 * Server hostkey corresponds to test key when connected.
 */
BOOST_AUTO_TEST_CASE( hostkey )
{
	session s = test_session();
	host_key key = s.hostkey();

	string expected = base64_decode(s.get(), EXPECTED_HOSTKEY);
	BOOST_CHECK_EQUAL(key.key(), expected);
	BOOST_CHECK_EQUAL(key.algorithm(), ssh::host_key::ssh_rsa);
	BOOST_CHECK_EQUAL(key.algorithm_name(), "ssh-rsa");
	BOOST_CHECK(!key.is_base64());
}

/**
 * Hashed (MD5) hostkey should print as:
 *    0C 0E D1 A5 BB 10 27 5F 76 92 4C E1 87 CE 5C 5E
 * in hex.
 */
BOOST_AUTO_TEST_CASE( hostkey_md5 )
{
	host_key key = test_session().hostkey();

	string hex_hash(ssh::host_key::hexify(key.md5_hash(), " ", true));

	BOOST_CHECK_EQUAL(
		hex_hash, "0C 0E D1 A5 BB 10 27 5F 76 92 4C E1 87 CE 5C 5E");
}

BOOST_AUTO_TEST_SUITE_END();
