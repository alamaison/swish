/**
    @file

    Unit tests for the locked HGLOBAL wrapper.

    @if license

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

    @endif
*/

#include "swish/atl.hpp"

#include "swish/shell_folder/data_object/GlobalLocker.hpp"  // Test subject

#include "test/common_boost/fixtures.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>  // shared_ptr
#include <boost/system/system_error.hpp>  // system_error

#include <string>

using boost::shared_ptr;
using boost::system::system_error;
using std::string;

namespace { // private

    typedef swish::shell_folder::data_object::GlobalLocker<char> 
        GlobalStringLock;

    /**
     * Put the test string into global memory and return a smart pointer to it.
     */
    shared_ptr<void> global_test_data(const string& data)
    {
        shared_ptr<void> global(
            ::GlobalAlloc(GMEM_MOVEABLE, data.size()+1), ::GlobalFree);
        char* buf = static_cast<char*>(::GlobalLock(global.get()));

        ::CopyMemory(buf, data.c_str(), data.size());
        buf[data.size()] = '\0';

        ::GlobalUnlock(buf);
        return global;
    }
}

BOOST_AUTO_TEST_SUITE( global_lock_test )

/**
 * Get locked data and check that it isn't unexpectedly different.
 */
BOOST_AUTO_TEST_CASE( lock )
{
    shared_ptr<void> global = global_test_data("lorem ipsum");

    GlobalStringLock lock(global.get());
    char* data = lock.get();

    BOOST_REQUIRE_EQUAL(data, "lorem ipsum");
}

/**
 * Create on an invalid HGLOBAL.
 * This should fail and throw an exception.
 */
BOOST_AUTO_TEST_CASE( lock_fail )
{
    BOOST_REQUIRE_THROW(GlobalStringLock lock(NULL), system_error);
}

/**
 * Copy a GlobalLock using the copy contructor.
 * The pointers returned from get() should be identical by *address*.
 */
BOOST_AUTO_TEST_CASE( lock_copy )
{
    shared_ptr<void> global = global_test_data("lorem ipsum");

    GlobalStringLock lock(global.get());
    void* data1 = lock.get();

    GlobalStringLock lock_copy(lock);
    void* data2 = lock_copy.get();

    BOOST_REQUIRE_EQUAL(data1, data2); // Compare address, not strings
}

/**
 * Copy a GlobalLock using copy assignment.
 * The pointers returned from get() should be identical by *address*.
 */
BOOST_AUTO_TEST_CASE( lock_copy_assign )
{
    // Create first lock on global data
    shared_ptr<void> global1 = global_test_data("lorem ipsum");
    GlobalStringLock lock1(global1.get());

    // Create a lock on *other* global data
    shared_ptr<void> global2 = global_test_data("dolor sit amet");
    GlobalStringLock lock2(global2.get());

    // Assign second lock to first which should point both locks to second data
    lock1 = lock2;

    char* data1 = lock1.get();
    char* data2 = lock2.get();

    // Compare addresses and make sure it points to the *second* string
    BOOST_REQUIRE_EQUAL(static_cast<void*>(data1), static_cast<void*>(data2));
    BOOST_REQUIRE_EQUAL(data1, "dolor sit amet");
}

BOOST_AUTO_TEST_SUITE_END()
