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

#include "sftp_fixture.hpp"

#include <ssh/filesystem.hpp>
#include <ssh/stream.hpp>

#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/uuid/uuid_generators.hpp> // random_generator
#include <boost/uuid/uuid_io.hpp>         // to_string

#include <string>

using ssh::filesystem::directory_iterator;
using ssh::filesystem::ofstream;
using ssh::filesystem::path;
using ssh::filesystem::sftp_file;
using ssh::filesystem::sftp_filesystem;
using ssh::session;

using boost::bind;
using boost::uuids::random_generator;

using test::ssh::session_fixture;

using std::string;

namespace
{

bool filename_matches(const string& filename, const sftp_file& remote_file)
{
    return filename == remote_file.path().filename();
}
}

namespace test
{
namespace ssh
{

sftp_fixture::sftp_fixture() : m_filesystem(authenticate_and_create_sftp())
{
}

sftp_filesystem& sftp_fixture::filesystem()
{
    return m_filesystem;
}

path sftp_fixture::sandbox() const
{
    return "sandbox";
}

path sftp_fixture::absolute_sandbox() const
{
    return "/home/swish/sandbox";
}

sftp_file sftp_fixture::find_file_in_sandbox(const string& filename)
{
    directory_iterator it = filesystem().directory_iterator(sandbox());
    directory_iterator pos = find_if(it, filesystem().directory_iterator(),
                                     bind(filename_matches, filename, _1));
    BOOST_REQUIRE(pos != filesystem().directory_iterator());

    return *pos;
}

path sftp_fixture::new_file_in_sandbox()
{
    random_generator generator;

    path filename = to_string(generator());
    return new_file_in_sandbox(filename);
}

path sftp_fixture::new_file_in_sandbox(const path& filename)
{
    path file = sandbox() / filename;
    ofstream(filesystem(), file).close();
    return file;
}

path sftp_fixture::new_file_in_sandbox_containing_data(const string& data)
{
    path p = new_file_in_sandbox();
    ofstream s(filesystem(), p);

    s.write(data.data(), data.size());

    return p;
}

path sftp_fixture::new_file_in_sandbox_containing_data(const path& name,
                                                       const string& data)
{
    path p = new_file_in_sandbox(name);
    ofstream s(filesystem(), p);

    s.write(data.data(), data.size());

    return p;
}

path sftp_fixture::new_directory_in_sandbox()
{
    random_generator generator;

    path directory_name = to_string(generator());
    path directory = sandbox() / directory_name;
    create_directory(filesystem(), directory);
    return directory;
}

void sftp_fixture::create_symlink(const path& link, const path& target)
{
    // Passing arguments in the wrong order to work around OpenSSH bug
    ::ssh::filesystem::create_symlink(filesystem(), target, link);
}

sftp_filesystem sftp_fixture::authenticate_and_create_sftp()
{
    session& s = test_session();
    s.authenticate_by_key_files(user(), public_key_path(), private_key_path(),
                                "");
    return s.connect_to_filesystem();
}
}
}
