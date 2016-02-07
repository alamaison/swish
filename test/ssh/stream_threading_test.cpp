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

#include <boost/bind/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread/future.hpp> // packaged_task
#include <boost/thread/thread.hpp>

#include <string>

using ssh::filesystem::ifstream;
using ssh::filesystem::fstream;
using ssh::filesystem::path;
using ssh::filesystem::sftp_filesystem;

using boost::bind;
using boost::packaged_task;
using boost::thread;

using test::ssh::sftp_fixture;

using std::string;

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

    packaged_task<string> p1(boost::bind(get_first_token, boost::ref(s1)));
    packaged_task<string> p2(boost::bind(get_first_token, boost::ref(s2)));

    thread(boost::ref(p1)).detach();
    thread(boost::ref(p2)).detach();

    BOOST_CHECK_EQUAL(p1.get_future().get(), "humpty");
    BOOST_CHECK_EQUAL(p2.get_future().get(), "on");
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

    // Using a long-running stream read operation to make sure the session
    // is still locked when we try to close the other file
    packaged_task<string> ps(bind(get_first_token, boost::ref(stream1)));
    thread(boost::ref(ps)).detach();

    thread(bind(&ifstream::close, &stream2)).detach();

    BOOST_CHECK_EQUAL(ps.get_future().get(), data);
}

BOOST_AUTO_TEST_SUITE_END();
