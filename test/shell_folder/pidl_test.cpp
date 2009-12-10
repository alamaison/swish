/**
    @file

    Unit tests for classes derived from basic_pidl.

    @if licence

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

#include "swish/shell_folder/pidl.hpp"  // test subject

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>
#include <boost/shared_ptr.hpp>  // shared_ptr
#include <boost/numeric/conversion/cast.hpp>  // numeric_cast

#include <ShlObj.h>  // ILClone etc.

#include <string>
#include <cstring>  // memset, memcpy

typedef boost::mpl::list<
	ITEMIDLIST_RELATIVE, ITEMIDLIST_ABSOLUTE, ITEMID_CHILD> pidl_types;
typedef boost::mpl::list<
	ITEMIDLIST_RELATIVE, ITEMID_CHILD> relative_pidl_types;

using namespace swish::shell_folder::pidl;
using boost::shared_ptr;
using boost::test_tools::predicate_result;
using boost::numeric_cast;

namespace {

	const std::string data = "Lorem ipsum dolor sit amet.";

	class PidlFixture
	{
	public:

		/**
		 * Allocate a PIDL large enough to hold the test data as well as the
		 * size field prefix (USHORT) and the null-terminator suffix (USHORT).
		 */
		PidlFixture() : 
			m_pidl(
				static_cast<ITEMIDLIST*>(::CoTaskMemAlloc(fake_pidl_size())),
				::CoTaskMemFree)
		{
			BOOST_REQUIRE(m_pidl);

			std::memset(m_pidl.get(), 0, fake_pidl_size());
			m_pidl.get()->mkid.cb = numeric_cast<USHORT>(
				fake_pidl_size() - sizeof(USHORT));

			std::memcpy(m_pidl.get()->mkid.abID, data.c_str(), data.size());
		}

		/**
		 * Return pointer to chunk of memory intialised with data as a PIDL.
		 */
		template<typename T>
		const T* fake_pidl() const
		{
			return static_cast<const T*>(m_pidl.get());
		}

		/**
		 * Size of the chunk of memory used as a fake PIDL.
		 */
		size_t fake_pidl_size() const
		{
			return data.size() + 2 * sizeof(USHORT);
		}

	private:
		shared_ptr<ITEMIDLIST> m_pidl;
	};
	

	/**
	 * Compare two PIDLs as a sequence of bytes.  Display mismatch as though
	 * the were streams of characters.
	 */
	predicate_result binary_equal_pidls(
		PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
	{
		const char* lhs = reinterpret_cast<const char*>(pidl1);
		const char* rhs = reinterpret_cast<const char*>(pidl2);
		return boost::test_tools::tt_detail::equal_coll_impl(
			lhs, lhs + ::ILGetSize(pidl1),
			rhs, rhs + ::ILGetSize(pidl2));
	}
}

#pragma region basic_pidl creation tests
BOOST_FIXTURE_TEST_SUITE(basic_pidl_creation_tests, PidlFixture)

/**
 * Default constructor should result in wrapped PIDL being NULL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( create, T, pidl_types )
{
	basic_pidl<T> pidl;
    BOOST_CHECK(!pidl.get());
	BOOST_CHECK(!pidl);
	BOOST_REQUIRE(pidl.empty());
}

/**
 * Initialising with NULL should set wrapped PIDL to NULL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( create_null, T, pidl_types )
{
	basic_pidl<T> pidl(NULL);
    BOOST_CHECK(!pidl.get());
	BOOST_CHECK(!pidl);
	BOOST_REQUIRE(pidl.empty());
}

/**
 * Initialising with valid PIDL should set wrapped PIDL to a non-null value.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( create_non_null, T, pidl_types )
{
	basic_pidl<T> pidl(fake_pidl<T>());
    BOOST_CHECK(pidl.get());
	BOOST_CHECK(!!pidl);
	BOOST_REQUIRE(!pidl.empty());
}

/**
 * Initialising with empty PIDL should set wrapped PIDL to a non-null value
 * but empty() should return true..
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( create_empty, T, pidl_types )
{
	SHITEMID empty = {0, {0}};
	basic_pidl<T> pidl(reinterpret_cast<const T*>(&empty));
    BOOST_CHECK(pidl.get());
	BOOST_CHECK(!!pidl);
	BOOST_REQUIRE(pidl.empty());
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

#pragma region raw PIDL function tests
BOOST_FIXTURE_TEST_SUITE(raw_pidl_func_tests, PidlFixture)

/**
 * Measure the size of a PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( size_raw, T, pidl_types )
{
	const T* pidl = fake_pidl<T>();
	BOOST_REQUIRE_EQUAL(raw_pidl::size(pidl), ::ILGetSize(pidl));
}

/**
 * Measure the size of NULL PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( size_raw_null, T, pidl_types )
{
	const T* pidl = NULL;
	BOOST_REQUIRE_EQUAL(raw_pidl::size(pidl), ::ILGetSize(pidl));
}

/**
 * Measure the size of the empty PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( size_raw_empty, T, pidl_types )
{
	SHITEMID empty = {0, {0}};
	const T* pidl = reinterpret_cast<const T*>(&empty);
	BOOST_REQUIRE_EQUAL(raw_pidl::size(pidl), ::ILGetSize(pidl));
}

template<typename T, typename U>
inline void do_combine_test(const T* pidl1, const U* pidl2)
{
	shared_ptr<ITEMIDLIST_RELATIVE> combined_pidl(
		raw_pidl::combine<newdelete_alloc<ITEMIDLIST_RELATIVE> >(pidl1, pidl2));
	shared_ptr<ITEMIDLIST_RELATIVE> expected_combined_pidl(
		reinterpret_cast<PIDLIST_RELATIVE>(::ILCombine(
			reinterpret_cast<PCUIDLIST_ABSOLUTE>(pidl1), pidl2)),
		::ILFree);

	BOOST_REQUIRE(binary_equal_pidls(
		combined_pidl.get(), expected_combined_pidl.get()));
}

/**
 * Join PIDLs to an absolute PIDL (raw).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( combine_abs, T, relative_pidl_types )
{
	const ITEMIDLIST_ABSOLUTE* pidl1 = fake_pidl<ITEMIDLIST_ABSOLUTE>();
	const T* pidl2 = fake_pidl<T>();
	do_combine_test(pidl1, pidl2);
}

/**
 * Join PIDLs to a relative PIDL (raw).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( combine_rel, T, relative_pidl_types )
{
	const ITEMIDLIST_RELATIVE* pidl1 = fake_pidl<ITEMIDLIST_RELATIVE>();
	const T* pidl2 = fake_pidl<T>();
	do_combine_test(pidl1, pidl2);
}

/**
 * Join PIDLs to a child PIDL (raw).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( combine_child, T, relative_pidl_types )
{
	const ITEMID_CHILD* pidl1 = fake_pidl<ITEMID_CHILD>();
	const T* pidl2 = fake_pidl<T>();
	do_combine_test(pidl1, pidl2);
}

/**
 * Join PIDL to NULL (raw).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( combine_null_pidl, T, relative_pidl_types )
{
	const T* pidl1 = NULL;
	const T* pidl2 = fake_pidl<T>();
	do_combine_test(pidl1, pidl2);
}

/**
 * Join NULL to PIDL (raw).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( combine_pidl_null, T, relative_pidl_types )
{
	const T* pidl1 = fake_pidl<T>();
	const T* pidl2 = NULL;
	do_combine_test(pidl1, pidl2);
}

/**
 * Join PIDL to empty PIDL (raw).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( combine_empty_pidl, T, relative_pidl_types )
{
	SHITEMID empty = {0, {0}};
	const T* pidl1 = reinterpret_cast<const T*>(&empty);
	const T* pidl2 = fake_pidl<T>();
	do_combine_test(pidl1, pidl2);
}

/**
 * Join empty PIDL to PIDL (raw).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( combine_pidl_empty, T, relative_pidl_types )
{
	const T* pidl1 = fake_pidl<T>();
	SHITEMID empty = {0, {0}};
	const T* pidl2 = reinterpret_cast<const T*>(&empty);
	do_combine_test(pidl1, pidl2);
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

#pragma region basic_pidl tests
BOOST_FIXTURE_TEST_SUITE(basic_pidl_tests, PidlFixture)

/**
 * Initialise a basic_pidl from a PIDL.
 * The data in both pidls must be the same but the PIDL's addresses should
 * be different (i.e. a copy).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( initialise, T, pidl_types )
{
	basic_pidl<T> pidl(fake_pidl<T>());

	BOOST_REQUIRE(binary_equal_pidls(pidl.get(), fake_pidl<T>()));

	BOOST_REQUIRE_NE(pidl.get(), fake_pidl<T>());
}

/**
 * Initialise a basic_pidl from the empty PIDL.
 * The data in both pidls must be the same but the PIDL's addresses should
 * be different (i.e. a copy).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( initialise_empty, T, pidl_types )
{
	SHITEMID empty = {0, {0}};
	const T* empty_pidl = reinterpret_cast<const T*>(&empty);
	basic_pidl<T> pidl(empty_pidl);

	BOOST_REQUIRE(binary_equal_pidls(pidl.get(), empty_pidl));

	BOOST_REQUIRE_NE(pidl.get(), empty_pidl);
}

/**
 * Assign a PIDL to a basic_pidl.
 * The data in both pidls must be the same but the PIDL's addresses should
 * be different (i.e. a copy).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( assign, T, pidl_types )
{
	basic_pidl<T> pidl;
	pidl = fake_pidl<T>();

	BOOST_REQUIRE(binary_equal_pidls(pidl.get(), fake_pidl<T>()));

	BOOST_REQUIRE_NE(pidl.get(), fake_pidl<T>());
}

/**
 * Copy-construct.
 * The data in both basic_pidls must be the same but their addresses should
 * be different (i.e. a copy).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( copy_construct, T, pidl_types )
{
	basic_pidl<T> pidl(fake_pidl<T>());
	basic_pidl<T> pidl_copy(pidl);

	BOOST_REQUIRE(binary_equal_pidls(pidl.get(), pidl_copy.get()));

	BOOST_REQUIRE_NE(pidl.get(), pidl_copy.get());
}

/**
 * Copy-assign.
 * The data in both basic_pidls must be the same but their addresses should
 * be different (i.e. a copy).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( copy_assign, T, pidl_types )
{
	basic_pidl<T> pidl(fake_pidl<T>());
	basic_pidl<T> pidl_copy;
	pidl_copy = pidl;

	BOOST_REQUIRE(binary_equal_pidls(pidl.get(), pidl_copy.get()));

	BOOST_REQUIRE_NE(pidl.get(), pidl_copy.get());
}

/**
 * Attach a basic_pidl to a raw PIDL.
 * The wrapped PIDL should have the same address as the orginal.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( attach, T, pidl_types )
{
	basic_pidl<T> pidl;
	
	T* raw = raw_pidl::clone<newdelete_alloc<T>>(fake_pidl<T>());
	pidl.attach(raw);

	BOOST_REQUIRE_EQUAL(pidl.get(), raw);
}

template<typename T, typename U>
void do_join_test(const basic_pidl<T>& pidl1, const basic_pidl<U>& pidl2)
{
	typedef typename basic_pidl<T>::join_type join_type;
	typedef typename basic_pidl<T>::join_pidl join_pidl;

	shared_ptr<join_type> expected_joined_pidl(
		reinterpret_cast<join_type*>(::ILCombine(
			reinterpret_cast<PCUIDLIST_ABSOLUTE>(pidl1.get()), pidl2.get())),
		::ILFree);

	join_pidl joined_pidl = pidl1 + pidl2;

	BOOST_REQUIRE(binary_equal_pidls(
		joined_pidl.get(), expected_joined_pidl.get()));
	BOOST_REQUIRE_NE(
		reinterpret_cast<const void*>(pidl1.get()), 
		reinterpret_cast<const void*>(joined_pidl.get()));
	BOOST_REQUIRE_NE(
		reinterpret_cast<const void*>(pidl2.get()), 
		reinterpret_cast<const void*>(joined_pidl.get()));
}

/**
 * Join different types of PIDL to a relative PIDL.
 * The result of joining two basic_pidls should be a newly allocated pidl
 * with the data from both.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_rel, T, relative_pidl_types )
{
	basic_pidl<ITEMIDLIST_RELATIVE> pidl1(fake_pidl<ITEMIDLIST_RELATIVE>());
	basic_pidl<T> pidl2(fake_pidl<T>());

	do_join_test(pidl1, pidl2);
}

/**
 * Join different types of PIDL to a child PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_child, T, relative_pidl_types )
{
	basic_pidl<ITEMID_CHILD> pidl1(fake_pidl<ITEMID_CHILD>());
	basic_pidl<T> pidl2(fake_pidl<T>());

	do_join_test(pidl1, pidl2);
}

/**
 * Join different types of PIDL to an absolute PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_abs, T, relative_pidl_types )
{
	basic_pidl<ITEMIDLIST_ABSOLUTE> pidl1(fake_pidl<ITEMIDLIST_ABSOLUTE>());
	basic_pidl<T> pidl2(fake_pidl<T>());

	do_join_test(pidl1, pidl2);
}

/**
 * Join different types of PIDL to a NULL PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_null_pidl, T, relative_pidl_types )
{
	basic_pidl<T> pidl1(NULL);
	basic_pidl<T> pidl2(fake_pidl<T>());

	do_join_test(pidl1, pidl2);
}

/**
 * Join a NULL PIDL to different types of PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_pidl_null, T, relative_pidl_types )
{
	basic_pidl<T> pidl1(fake_pidl<T>());
	basic_pidl<T> pidl2(NULL);

	do_join_test(pidl1, pidl2);
}

/**
 * Join different types of PIDL to an empty PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_empty_pidl, T, relative_pidl_types )
{
	SHITEMID empty = {0, {0}};
	basic_pidl<T> pidl1 = reinterpret_cast<const T*>(&empty);
	basic_pidl<T> pidl2(fake_pidl<T>());

	do_join_test(pidl1, pidl2);
}

/**
 * Join an empty PIDL to different types of PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_pidl_empty, T, relative_pidl_types )
{
	basic_pidl<T> pidl1(fake_pidl<T>());
	SHITEMID empty = {0, {0}};
	basic_pidl<T> pidl2 = reinterpret_cast<const T*>(&empty);

	do_join_test(pidl1, pidl2);
}


template<typename T, typename U>
void do_append_test(basic_pidl<T> pidl1, const basic_pidl<U>& pidl2)
{
	typedef typename basic_pidl<T>::join_type join_type;
	typedef typename basic_pidl<T>::join_pidl join_pidl;

	shared_ptr<T> expected_appended_pidl(
		reinterpret_cast<T*>(::ILCombine(
			reinterpret_cast<PCUIDLIST_ABSOLUTE>(pidl1.get()), pidl2.get())),
		::ILFree);

	pidl1 += pidl2;

	BOOST_REQUIRE(binary_equal_pidls(
		pidl1.get(), expected_appended_pidl.get()));
}

/**
 * Append different types of PIDL to a relative PIDL.
 * The left-hand PIDL should end up holding newly-allocated memory
 * with the data from both PIDLs.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_rel, T, relative_pidl_types )
{
	basic_pidl<ITEMIDLIST_RELATIVE> pidl1(fake_pidl<ITEMIDLIST_RELATIVE>());
	basic_pidl<T> pidl2(fake_pidl<T>());

	do_append_test(pidl1, pidl2);
}

/**
 * Append different types of PIDL to a child PIDL.
 * THIS TEST SHOULD FAIL TO COMPILE IF enable_if IS WORKING.
 */
/*
BOOST_AUTO_TEST_CASE_TEMPLATE( append_child, T, relative_pidl_types )
{
	basic_pidl<ITEMID_CHILD> pidl1(fake_pidl<ITEMID_CHILD>());
	basic_pidl<T> pidl2(fake_pidl<T>());

	do_append_test(pidl1, pidl2);
}
*/

/**
 * Append different types of PIDL to an absolute PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_abs, T, relative_pidl_types )
{
	basic_pidl<ITEMIDLIST_ABSOLUTE> pidl1(fake_pidl<ITEMIDLIST_ABSOLUTE>());
	basic_pidl<T> pidl2(fake_pidl<T>());

	do_append_test(pidl1, pidl2);
}

/**
 * Append different types of PIDL to a NULL PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_null_pidl, T, relative_pidl_types )
{
	basic_pidl<ITEMIDLIST_RELATIVE> pidl1(NULL);
	basic_pidl<T> pidl2(fake_pidl<T>());

	do_append_test(pidl1, pidl2);
}

/**
 * Append a NULL PIDL to different types of PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_pidl_null, T, relative_pidl_types )
{
	basic_pidl<ITEMIDLIST_RELATIVE> pidl1(fake_pidl<T>());
	basic_pidl<T> pidl2(NULL);

	do_append_test(pidl1, pidl2);
}

/**
 * Append different types of PIDL to an empty PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_empty_pidl, T, relative_pidl_types )
{
	SHITEMID empty = {0, {0}};
	basic_pidl<ITEMIDLIST_RELATIVE> pidl1 = 
		reinterpret_cast<const T*>(&empty);
	basic_pidl<T> pidl2(fake_pidl<T>());

	do_append_test(pidl1, pidl2);
}

/**
 * Append an empty PIDL to different types of PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_pidl_empty, T, relative_pidl_types )
{
	basic_pidl<ITEMIDLIST_RELATIVE> pidl1(fake_pidl<T>());
	SHITEMID empty = {0, {0}};
	basic_pidl<T> pidl2 = reinterpret_cast<const T*>(&empty);

	do_append_test(pidl1, pidl2);
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion
