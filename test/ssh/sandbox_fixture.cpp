/**
    @file

    Fixture creating a temporary sandbox directory.

    @if licence

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "sandbox_fixture.hpp"

#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp> // BOOST_REQUIRE etc.

#include <cstdio> // _tempnam
#include <string>
#include <vector>

using boost::system::system_error;
using boost::system::system_category;
using boost::filesystem::path;
using boost::shared_ptr;

using std::string;
using std::vector;

namespace test {
namespace ssh {

namespace { // private

	const string SANDBOX_NAME = "ssh-sandbox";

	/**
	 * Return the path to the sandbox directory.
	 */
	path sandbox_directory()
	{
		shared_ptr<char> name(
			_tempnam(NULL, SANDBOX_NAME.c_str()), free);
		BOOST_REQUIRE(name);
		
		return path(name.get());
	}
}

sandbox_fixture::sandbox_fixture() : m_sandbox(sandbox_directory())
{
	create_directory(m_sandbox);
}

sandbox_fixture::~sandbox_fixture()
{
	try
	{
		remove_all(m_sandbox);
	}
	catch (...) {}
}

path sandbox_fixture::sandbox()
{
	return m_sandbox;
}

/**
 * Create a new empty file in the fixture sandbox with a random name
 * and return the path.
 */
path sandbox_fixture::new_file_in_sandbox()
{
	vector<char> buffer(MAX_PATH);

	if (!GetTempFileNameA(
		sandbox().directory_string().c_str(), NULL, 0, &buffer[0]))
		throw system_error(::GetLastError(), system_category);
	
	path p = path(string(&buffer[0], buffer.size()));
	BOOST_CHECK(exists(p));
	BOOST_CHECK(is_regular_file(p));
	BOOST_CHECK(p.is_complete());
	return p;
}

}} // namespace test::ssh
