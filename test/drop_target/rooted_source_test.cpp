// Copyright 2012, 2016 Alexander Lamaison

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

#include "swish/drop_target/RootedSource.hpp" // Test subject

#include <test/common_boost/helpers.hpp> // wchar_t ostream
#include <test/fixtures/local_sandbox_fixture.hpp>

#include <washer/shell/shell.hpp>      // pidl_from_parsing_name
#include <washer/shell/shell_item.hpp> // pidl_shell_item
#include <washer/shell/pidl.hpp>       // apidl_t, cpidl_t

#include <boost/filesystem/path.hpp>    // path
#include <boost/filesystem/fstream.hpp> // ofstream
#include <boost/test/unit_test.hpp>

using swish::drop_target::RootedSource;

using test::fixtures::local_sandbox_fixture;

using washer::shell::pidl::apidl_t;
using washer::shell::pidl::cpidl_t;
using washer::shell::pidl::pidl_t;
using washer::shell::pidl_from_parsing_name;
using washer::shell::pidl_shell_item;

using boost::filesystem::wofstream;
using boost::filesystem::path;

namespace
{

class RootedSourceFixture : public local_sandbox_fixture
{
public:
    path test_root()
    {
        return local_sandbox();
    }

    path child_file()
    {
        return new_file_in_local_sandbox();
    }

    path child_directory()
    {
        path directory = local_sandbox() / L"testdir";
        create_directory(directory);
        return directory;
    }

    path grandchild_file()
    {
        path directory = local_sandbox() / L"testdir";
        create_directory(directory);

        path file = directory / L"testfile.txt";
        wofstream s(file);

        return file;
    }

    path greatgrandchild_file()
    {
        path directory1 = local_sandbox() / L"testdir1";
        create_directory(directory1);

        path directory2 = directory1 / L"testdir2";
        create_directory(directory2);

        path file = directory2 / L"testfile.txt";
        wofstream s(file);

        return file;
    }
};

inline bool operator==(const apidl_t& lhs, const apidl_t& rhs)
{
    return pidl_shell_item(lhs).parsing_name() ==
           pidl_shell_item(rhs).parsing_name();
}

apidl_t to_pidl(const path& path)
{
    return pidl_from_parsing_name(path.wstring());
}
}

BOOST_FIXTURE_TEST_SUITE(rooted_source_tests, RootedSourceFixture)

/**
 * Test the source where the root is the source itself (no branch).
 */
BOOST_AUTO_TEST_CASE(root)
{
    apidl_t root_pidl = to_pidl(test_root());
    RootedSource source(root_pidl, cpidl_t());

    BOOST_CHECK(source.pidl() == root_pidl);
    BOOST_CHECK(source.common_root() == root_pidl);
    BOOST_CHECK_EQUAL(source.relative_name(), L"");
}

/**
 * Test the source where the source is a file directly under the root.
 */
BOOST_AUTO_TEST_CASE(child)
{
    path file = child_file();
    apidl_t pidl = to_pidl(file);

    RootedSource source(pidl.parent(), pidl.last_item());

    BOOST_CHECK(source.pidl() == pidl);
    BOOST_CHECK(source.common_root() == pidl.parent());
    BOOST_CHECK_EQUAL(source.relative_name(), file.filename());
}

/**
 * Test the source where the source is a directory directly under the root.
 */
BOOST_AUTO_TEST_CASE(child_dir)
{
    path directory = child_directory();
    apidl_t pidl = to_pidl(directory);

    RootedSource source(pidl.parent(), pidl.last_item());

    BOOST_CHECK(source.pidl() == pidl);
    BOOST_CHECK(source.common_root() == pidl.parent());
    BOOST_CHECK_EQUAL(source.relative_name(), directory.filename());
}

/**
 * Test the source where the source is grandchild of the root.
 */
BOOST_AUTO_TEST_CASE(grandchild)
{
    path file = grandchild_file();
    apidl_t pidl = to_pidl(file);
    apidl_t root_pidl = pidl.parent().parent();
    pidl_t branch = pidl.parent().last_item() + pidl.last_item();

    RootedSource source(root_pidl, branch);

    BOOST_CHECK(source.pidl() == pidl);
    BOOST_CHECK(source.common_root() == root_pidl);

    path expected_relative_name = file.parent_path().filename();
    expected_relative_name /= file.filename();
    BOOST_CHECK_EQUAL(source.relative_name(), expected_relative_name.wstring());
}

/**
 * Test the source where the source is grandchild of the root.
 */
BOOST_AUTO_TEST_CASE(greatgrandchild)
{
    path file = greatgrandchild_file();
    apidl_t pidl = to_pidl(file);
    apidl_t root_pidl = pidl.parent().parent().parent();
    pidl_t branch = pidl.parent().parent().last_item() +
                    pidl.parent().last_item() + pidl.last_item();

    RootedSource source(root_pidl, branch);

    BOOST_CHECK(source.pidl() == pidl);
    BOOST_CHECK(source.common_root() == root_pidl);

    path expected_relative_name = file.parent_path().parent_path().filename();
    expected_relative_name /= file.parent_path().filename();
    expected_relative_name /= file.filename();
    BOOST_CHECK_EQUAL(source.relative_name(), expected_relative_name.wstring());
}

BOOST_AUTO_TEST_SUITE_END()
