// Copyright 2016 Alexander Lamaison

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

#ifndef SWISH_TEST_FIXTURES_LOCAL_SANDBOX_FIXTURE_HPP
#define SWISH_TEST_FIXTURES_LOCAL_SANDBOX_FIXTURE_HPP

#include <boost/filesystem.hpp>

namespace test
{
namespace fixtures
{
/**
 * Fixture that creates and destroys a sandbox directory.
 */
class local_sandbox_fixture
{
public:
    local_sandbox_fixture();
    virtual ~local_sandbox_fixture();

    boost::filesystem::path local_sandbox();
    boost::filesystem::path new_file_in_local_sandbox();
    boost::filesystem::path
    new_file_in_local_sandbox(const boost::filesystem::path& name);
    boost::filesystem::path new_directory_in_local_sandbox();
    boost::filesystem::path
    new_directory_in_local_sandbox(const boost::filesystem::path& name);

private:
    boost::filesystem::path m_sandbox;
};
}
}
#endif
