/**
    @file

    Exercise host management registry manipulation.

    @if license

    Copyright (C) 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <swish/host_folder/host_management.hpp> // test subject

#include <test/common_boost/helpers.hpp> // wide-string output

#include <boost/test/unit_test.hpp>

#include <comet/regkey.h>

#include <string>

using swish::host_folder::host_management::AddConnectionToRegistry;
using swish::host_folder::host_management::RemoveConnectionFromRegistry;

using comet::regkey;

using std::exception;
using std::wstring;

namespace {

	const wstring TEST_CONNECTION_NAME = L"T";

	struct cleanup_fixture
	{
		~cleanup_fixture()
		{
			regkey connections =
				regkey(HKEY_CURRENT_USER).open(L"Software\\Swish\\Connections");
			connections.delete_subkey_nothrow(TEST_CONNECTION_NAME);
		}
	};

	regkey test_connection_key()
	{
		return regkey(HKEY_CURRENT_USER).open(
			L"Software\\Swish\\Connections\\" + TEST_CONNECTION_NAME);
	}
}

BOOST_FIXTURE_TEST_SUITE(host_management_tests, cleanup_fixture)

BOOST_AUTO_TEST_CASE( add_minimal )
{
	AddConnectionToRegistry(TEST_CONNECTION_NAME, L"h", 1U, L"u", L"/");

	regkey new_connection = test_connection_key();

	BOOST_CHECK_EQUAL(new_connection[L"Host"].str(), L"h");
	BOOST_CHECK_EQUAL(new_connection[L"User"].str(), L"u");
	BOOST_CHECK_EQUAL(new_connection[L"Port"].dword(), 1U);
	BOOST_CHECK_EQUAL(new_connection[L"Path"].str(), L"/");
}

BOOST_AUTO_TEST_CASE( add )
{
	wstring hostname = 
		L"a.nice.really.beautiful.long.loooooooooooooooooooooooooooooo"
		L"ooooooong.host.name.example";
	wstring username = 
		L"dsflkm dfsdoifmo opim[i\"moimoimoimoim[ipom]0k3\"9k42p3m4l23 4k 23;"
		L"krjn1;oi[9j[c09j38j4kj2 3k4 ;2o3iun4[029j3[9mre4;cj ;l3i45r c£";
	wstring path = 
		L"/krjn1;oi[9j[c09j38j4kj2 3k4 ;2o3iun4[029j3[9mre4;cj ;l3i45r c£"
		L"dsflkm dfsdoifmo opim[i\"moimoimoimoim[ipom]0k3\"9k42p3m4l23 4k 23;";

	AddConnectionToRegistry(
		TEST_CONNECTION_NAME, hostname, 65535U, username, path);

	regkey new_connection = test_connection_key();

	BOOST_CHECK_EQUAL(new_connection[L"Host"].str(), hostname);
	BOOST_CHECK_EQUAL(new_connection[L"User"].str(), username);
	BOOST_CHECK_EQUAL(new_connection[L"Port"].dword(), 65535U);
	BOOST_CHECK_EQUAL(new_connection[L"Path"].str(), path);
}

BOOST_AUTO_TEST_CASE( remove )
{
	AddConnectionToRegistry(TEST_CONNECTION_NAME, L"h", 1U, L"u", L"/");
	RemoveConnectionFromRegistry(L"T");

	BOOST_CHECK_THROW(test_connection_key(), exception);
}

BOOST_AUTO_TEST_SUITE_END()
