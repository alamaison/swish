/**
    @file

    Unit tests for classes derived from basic_pidl.

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

#include "swish/shell_folder/data_object/FileGroupDescriptor.hpp"  // subject

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>  // shared_ptr

#include <cstring>  // memset, memcpy

using namespace swish::shell_folder::data_object;
using boost::shared_ptr;
using boost::filesystem::wpath;

namespace {

    class FgdFixture
    {
    public:

        static const size_t TEST_ALLOC_SIZE = 
            sizeof(FILEGROUPDESCRIPTOR) + sizeof(FILEDESCRIPTOR);

        /**
         * Allocate a fake FILEGROUPDESCRIPTOR with space for two
         * FILEDESCRIPTORS.
         */
        FgdFixture() : 
            m_hglobal(
                ::GlobalAlloc(GMEM_MOVEABLE, TEST_ALLOC_SIZE), ::GlobalFree)
        {
            BOOST_REQUIRE(m_hglobal.get());

            FILEGROUPDESCRIPTOR* fgd = 
                static_cast<FILEGROUPDESCRIPTOR*>(
                    ::GlobalLock(m_hglobal.get()));
            BOOST_REQUIRE(fgd);

            fgd->cItems = 2;

            FILEDESCRIPTOR fd1;
            std::memset(&fd1, 0, sizeof(fd1));
            wcscpy_s(fd1.cFileName, L"test/item/path");

            FILEDESCRIPTOR fd2;
            std::memset(&fd2, 0, sizeof(fd2));
            wcscpy_s(fd2.cFileName, L"test\\item\\bob");

            fgd->fgd[0] = fd1;
            fgd->fgd[1] = fd2;

            BOOST_REQUIRE(::GlobalUnlock(fgd));
        }

        /**
         * Return pointer to chunk of memory intialised with fake
         * FILEGROUPDESCRIPTOR.
         */
        HGLOBAL get() const
        {
            return m_hglobal.get();
        }

    private:
        shared_ptr<void> m_hglobal; // HGLOBAL
    };
    

}

#pragma region FileGroupDescriptor tests
BOOST_FIXTURE_TEST_SUITE(file_group_descriptor_tests, FgdFixture)

/**
 * Constructor doesn't throw.
 */
BOOST_AUTO_TEST_CASE( create )
{
    FileGroupDescriptor fgd(get());
}

/**
 * Counting contained descriptors gives the expected value of 2.
 */
BOOST_AUTO_TEST_CASE( size )
{
    FileGroupDescriptor fgd(get());
    size_t size = fgd.size();
    BOOST_REQUIRE_EQUAL(size, 2U);
}

/**
 * Accessing descriptors renders the expected data.
 */
BOOST_AUTO_TEST_CASE( access )
{
    FileGroupDescriptor fgd(get());
    BOOST_REQUIRE(fgd[0].path() == wpath(L"test/item/path"));
    BOOST_REQUIRE(fgd[1].path() == wpath(L"test\\item\\bob"));
    BOOST_REQUIRE(fgd[0].path() == wpath(L"test/item/path"));
}

/**
 * Accessing an out-of-bounds descriptor throws an exception.
 */
BOOST_AUTO_TEST_CASE( bounds_error )
{
    FileGroupDescriptor fgd(get());
    BOOST_REQUIRE_THROW(fgd[2], std::out_of_range);
}

/**
 * The lifetime of a descriptor outlives that of its parent group.
 */
BOOST_AUTO_TEST_CASE( descriptor_lifetime )
{
    FileGroupDescriptor fgd(get());
    Descriptor d = fgd[0];

    {
        FileGroupDescriptor scoped_fgd(get());
        d = scoped_fgd[1];
    }

    BOOST_REQUIRE(d.path() == wpath(L"test\\item\\bob"));
}

/**
 * Changing a descriptor outside the FileGroupDescriptor should leave the
 * copy in the FGD unchanged.  This checks that descriptors point at copies
 * not references to the original memory.
 */
BOOST_AUTO_TEST_CASE( descriptor_independence )
{
    FileGroupDescriptor fgd(get());
    Descriptor d = fgd[1];
    d.path(L"replaced/path");

    BOOST_REQUIRE(d.path() == L"replaced/path");
    BOOST_REQUIRE(fgd[1].path() == wpath(L"test\\item\\bob"));
}

/**
 * Changing a descriptor in the FileGroupDescriptor directly should change
 * the value returned in subsequent accesses.  This checks that the FGD
 * [] accessor returns the descriptors by reference.
 */
BOOST_AUTO_TEST_CASE( descriptor_access_byref )
{
    FileGroupDescriptor fgd(get());
    fgd[1].path(L"replaced/path");
    Descriptor d = fgd[1];

    BOOST_REQUIRE(d.path() == L"replaced/path");
    BOOST_REQUIRE(fgd[1].path() == L"replaced/path");
}

/**
 * A copy of an FGD should give the expected data from its accessors.
 * This checks that the copied FGD has access to sensible data but does
 * *not* check that it points to the same copy of the data as the original.
 */
BOOST_AUTO_TEST_CASE( copy_construct )
{
    FileGroupDescriptor fgd_orig(get());
    FileGroupDescriptor fgd = fgd_orig;
    BOOST_REQUIRE(fgd[0].path() == wpath(L"test/item/path"));
    BOOST_REQUIRE(fgd[1].path() == wpath(L"test\\item\\bob"));
    BOOST_REQUIRE_EQUAL(fgd.size(), 2U);
}

/**
 * A copy of an FGD should point to the same memory as the original.
 * Therefore, changes to one should affect the other.
 */
BOOST_AUTO_TEST_CASE( copies_are_linked )
{
    FileGroupDescriptor fgd_orig(get());
    FileGroupDescriptor fgd = fgd_orig;
    
    fgd[1].path(L"replaced/path");

    BOOST_REQUIRE(fgd_orig[1].path() == L"replaced/path");
}

/**
 * Descriptor field are initialised to zero.
 */
BOOST_AUTO_TEST_CASE( descriptor_zero_init )
{
    Descriptor d;
    const FILEDESCRIPTOR fd = d.get();

    FILEDESCRIPTOR fd_zero;
    std::memset(&fd_zero, 0, sizeof(fd_zero));

    BOOST_REQUIRE(!std::memcmp(&fd, &fd_zero, sizeof(fd)));
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion
