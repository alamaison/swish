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

#include "local_sandbox_fixture.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/uuid/uuid_generators.hpp> // random_generator
#include <boost/uuid/uuid_io.hpp>         // to_string

using boost::filesystem::path;
using boost::filesystem::ofstream;
using boost::filesystem::temp_directory_path;
using boost::uuids::random_generator;

namespace test
{
namespace fixtures
{

local_sandbox_fixture::local_sandbox_fixture()
    : m_sandbox(temp_directory_path() / to_string(random_generator()()))
{
    create_directory(m_sandbox);
}

local_sandbox_fixture::~local_sandbox_fixture()
{
    try
    {
        remove_all(m_sandbox);
    }
    catch (...)
    {
    }
}

path local_sandbox_fixture::local_sandbox()
{
    return m_sandbox;
}

path local_sandbox_fixture::new_file_in_local_sandbox(const path& name)
{
    path p = local_sandbox() / name;
    ofstream file(p);
    return p;
}

path local_sandbox_fixture::new_file_in_local_sandbox()
{
    random_generator generator;

    path filename = to_string(generator());
    return new_file_in_local_sandbox(filename);
}

path local_sandbox_fixture::new_directory_in_local_sandbox()
{
    random_generator generator;

    path directory_name = to_string(generator());
    path directory = local_sandbox() / directory_name;
    create_directory(directory);
    return directory;
}

path local_sandbox_fixture::new_directory_in_local_sandbox(const path& name)
{
    path p = local_sandbox() / name;
    create_directory(p);
    return p;
}
}
}
