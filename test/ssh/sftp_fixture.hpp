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

#ifndef TEST_SSH_SFTP_FIXTURE_HPP
#define TEST_SSH_SFTP_FIXTURE_HPP

#include "session_fixture.hpp"

#include <ssh/filesystem.hpp>

#include <string>

namespace test
{
namespace ssh
{
class sftp_fixture : public session_fixture
{
public:
    sftp_fixture();

    ::ssh::filesystem::sftp_filesystem& filesystem();

    ::ssh::filesystem::sftp_file
    find_file_in_sandbox(const std::string& filename);

    ::ssh::filesystem::path new_file_in_sandbox();

    ::ssh::filesystem::path
    new_file_in_sandbox(const ::ssh::filesystem::path& filename);

    ::ssh::filesystem::path
    new_file_in_sandbox_containing_data(const std::string& data);

    ::ssh::filesystem::path
    new_file_in_sandbox_containing_data(const ::ssh::filesystem::path& name,
                                        const std::string& data);

    ::ssh::filesystem::path new_directory_in_sandbox();

    void create_symlink(const ::ssh::filesystem::path& link,
                        const ::ssh::filesystem::path& target);

private:
    ::ssh::filesystem::sftp_filesystem authenticate_and_create_sftp();

    ::ssh::filesystem::sftp_filesystem m_filesystem;
};
}
}

#endif
