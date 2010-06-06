/**
    @file

    Tests for the SFTP directory listing helper functions.

    @if licence

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/provider/listing/listing.hpp"
#include "swish/interfaces/SftpProvider.h" // Listing

#include "test/common_boost/helpers.hpp"

#include <boost/test/unit_test.hpp>

#include <string>

using comet::bstr_t;

using std::string;
using std::wstring;

using namespace swish::provider;

namespace {
	const string longentry = 
		"-rw-r--r--    1 swish    wheel         767 Dec  8  2005 .cshrc";
}

BOOST_AUTO_TEST_SUITE(listing_tests)

/**
 * Test for parse_user_from_long_entry().
 */
BOOST_AUTO_TEST_CASE( parse_user_test )
{
	bstr_t bstr = listing::parse_user_from_long_entry(longentry);
	wstring user(L"swish");
	BOOST_REQUIRE_EQUAL(bstr, user);
}

/**
 * Test for parse_group_from_long_entry().
 */
BOOST_AUTO_TEST_CASE( parse_group_test )
{
	bstr_t bstr = listing::parse_group_from_long_entry(longentry);
	BOOST_REQUIRE_EQUAL(bstr, L"wheel");
}

/**
 * Test for fill_listing_entry().
 *
 * @todo Test dates.
 * @todo Test max filesize.
 * @todo Test tricky filename forms.
 */
BOOST_AUTO_TEST_CASE( create_listing_test )
{
	// Set up properties for test.  These are intentionally different from
	// those in the long entry to check that *only* the user and group
	// names are parsed from it.
	string filename(".cshrc test");
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	::ZeroMemory(&attrs, sizeof(attrs));
	attrs.flags = LIBSSH2_SFTP_ATTR_UIDGID | LIBSSH2_SFTP_ATTR_SIZE | 
		LIBSSH2_SFTP_ATTR_PERMISSIONS;
	attrs.uid = 1000;
	attrs.gid = 1001;
	attrs.filesize = 348;
	attrs.permissions = 0677;

	Listing lt = listing::fill_listing_entry(filename, longentry, attrs);

	// Check fields that should be set
	BOOST_CHECK_EQUAL(lt.bstrFilename, L".cshrc test");
	BOOST_CHECK_EQUAL(lt.uUid, static_cast<ULONG>(1000));
	BOOST_CHECK_EQUAL(lt.uGid, static_cast<ULONG>(1001));
	BOOST_CHECK_EQUAL(lt.bstrOwner, L"swish");
	BOOST_CHECK_EQUAL(lt.bstrGroup, L"wheel");
	BOOST_CHECK_EQUAL(lt.uSize, static_cast<ULONG>(348));
	BOOST_CHECK_EQUAL(lt.uPermissions, static_cast<ULONG>(0677));

	// Check fields that should not be set
	BOOST_CHECK_EQUAL(lt.cHardLinks, static_cast<ULONG>(0));
	BOOST_CHECK_EQUAL(lt.dateModified, static_cast<ULONG>(0));

	// Clean up memory
	::SysFreeString(lt.bstrFilename);
	::SysFreeString(lt.bstrGroup);
	::SysFreeString(lt.bstrOwner);
}

BOOST_AUTO_TEST_SUITE_END()
