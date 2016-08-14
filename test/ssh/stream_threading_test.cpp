// Copyright 2013, 2016 Alexander Lamaison

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

#include <ssh/stream.hpp> // test subject

#include <boost/test/unit_test.hpp>

#include <future>
#include <string>
#include <thread>

using ssh::filesystem::ifstream;
using ssh::filesystem::fstream;
using ssh::filesystem::path;
using ssh::filesystem::sftp_filesystem;

using test::ssh::sftp_fixture;

using std::packaged_task;
using std::string;
using std::thread;

namespace
{

// the large data must fill more than one stream buffer (currently set to
// 32768 (see DEFAULT_BUFFER_SIZE)

string large_data()
{
    string data;
    for (int i = 0; i < 32000; ++i)
    {
        data.push_back('a');
        data.push_back('m');
        data.push_back('z');
    }

    return data;
}
}

BOOST_FIXTURE_TEST_SUITE(stream_threading_test, sftp_fixture)

namespace
{

string get_first_token(ifstream& stream)
{
    string r;
    stream >> r;
    return r;
}
}

BOOST_AUTO_TEST_CASE(stream_read_on_different_threads)
{
    path target1 = new_file_in_sandbox_containing_data("humpty dumpty sat");
    path target2 = new_file_in_sandbox_containing_data("on the wall");

    sftp_filesystem& chan = filesystem();

    ifstream s1(chan, target1);
    ifstream s2(chan, target2);

    // TODO: Use async
    packaged_task<string()> p1([&s1]
                               {
                                   return get_first_token(s1);
                               });
    packaged_task<string()> p2([&s2]
                               {
                                   return get_first_token(s2);
                               });
    auto future1 = p1.get_future();
    auto future2 = p2.get_future();
    thread t1(std::move(p1));
    thread t2(std::move(p2));
    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL(future1.get(), "humpty");
    BOOST_CHECK_EQUAL(future2.get(), "on");
}

// There was a bug in our session locking that meant we locked the session
// when opening a file but didn't when closing it (because it happened in
// the shared_ptr destructor).  This test case triggers that bug by opening
// a file (locks and unlocks session), starting to read from a second file
// (locks) session and then closing the first file.  This will cause all
// sorts of bad behaviour of the closure doesn't lock the session so we can
// detect it if it regresses.
BOOST_AUTO_TEST_CASE(parallel_file_closing)
{
    string data = large_data();

    path read_me = new_file_in_sandbox_containing_data(data);
    path test_me = new_file_in_sandbox();

    ifstream stream1(filesystem(), read_me);
    ifstream stream2(filesystem(), test_me);

    // TODO: Use async

    // Using a long-running stream read operation to make sure the session
    // is still locked when we try to close the other file
    packaged_task<string()> ps([&stream1]
                               {
                                   return get_first_token(stream1);
                               });
    auto future = ps.get_future();
    thread t1(std::move(ps));

    thread t2([&stream2]
              {
                  stream2.close();
              });

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL(future.get(), data);
}

BOOST_AUTO_TEST_SUITE_END();
