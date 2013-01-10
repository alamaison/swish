/**
    @file

    Unit tests for version info.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    If you modify this Program, or any covered work, by linking or
    combining it with the OpenSSL project's OpenSSL library (or a
    modified version of that library), containing parts covered by the
    terms of the OpenSSL or SSLeay licenses, the licensors of this
    Program grant you additional permission to convey the resulting work.

    @endif
*/

#include <boost/test/unit_test.hpp>

#include <swish/version/version.hpp>

#include <string>

using std::string;

BOOST_AUTO_TEST_SUITE( version_test )

/**
 * Sensible snapshot version result.
 */
BOOST_AUTO_TEST_CASE( snapshot )
{
    string version = swish::snapshot_version();
    
    BOOST_WARN_MESSAGE(
        !version.empty(), "Legal, but unfortunate, snapshot description");
}

BOOST_AUTO_TEST_CASE( release_numeric )
{
    swish::structured_version version = swish::release_version();
    
    BOOST_CHECK_LT(version.major(), 50);
    BOOST_CHECK_GE(version.major(), 0);

    BOOST_CHECK_LT(version.minor(), 500);
    BOOST_CHECK_GE(version.minor(), 0);

    BOOST_CHECK_LT(version.bugfix(), 500);
    BOOST_CHECK_GE(version.bugfix(), 0);
}

/**
 * Sensible release version string result.
 */
BOOST_AUTO_TEST_CASE( release_string )
{
    string version = swish::release_version().as_string();
    
    BOOST_CHECK_MESSAGE(
        !version.empty(), "Release version not allowed to be empty");
}

BOOST_AUTO_TEST_CASE( timestamp )
{
    BOOST_CHECK_MESSAGE(
        !swish::build_date().empty(), "Build date not allowed to be empty");
    BOOST_CHECK_MESSAGE(
        !swish::build_time().empty(), "Build time not allowed to be empty");
}

BOOST_AUTO_TEST_SUITE_END()
