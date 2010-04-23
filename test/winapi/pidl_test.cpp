/**
    @file

    Unit tests for classes derived from basic_pidl.

    @if licence

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <winapi/shell/pidl.hpp>  // test subject

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>
#include <boost/shared_ptr.hpp>  // shared_ptr
#include <boost/numeric/conversion/cast.hpp>  // numeric_cast

#include <ShlObj.h>  // ILClone etc.

#include <string>
#include <cstring>  // memset, memcpy


using namespace winapi::shell::pidl;

using boost::shared_ptr;
using boost::test_tools::predicate_result;
using boost::numeric_cast;

namespace {

	/**
	 * @name Convenience types for testing.
	 */
	// @{

	typedef ITEMIDLIST_RELATIVE IDRELATIVE;
	typedef ITEMIDLIST_ABSOLUTE IDABSOLUTE;
	typedef ITEMID_CHILD IDCHILD;

	template<typename T>
	struct heap_pidl
	{
		typedef basic_pidl<T, newdelete_alloc<T> > type;
	};

	typedef heap_pidl<IDRELATIVE>::type hpidl_t;
	typedef heap_pidl<IDABSOLUTE>::type ahpidl_t;
	typedef heap_pidl<IDCHILD>::type chpidl_t;

	typedef boost::mpl::list<IDRELATIVE, IDABSOLUTE, IDCHILD> pidl_types;
	typedef boost::mpl::list<IDRELATIVE, IDCHILD> relative_pidl_types;
	typedef boost::mpl::list<IDRELATIVE, IDABSOLUTE> adult_pidl_types;

	// @}

	/**
	 * @name Getters
	 *
	 * Getters so that test helper functions can operate on wrapped and raw
	 * PIDLs alike.
	 */
	// @{

	template<typename T, typename A>
	const T* get(const basic_pidl<T, A>& pidl) { return pidl.get(); }
	
	template<typename T>
	const T* get(const T* raw_pidl) { return raw_pidl; }

	// @}

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
	heap_pidl<T>::type pidl;
    BOOST_CHECK(!pidl.get());
	BOOST_CHECK(!pidl);
	BOOST_REQUIRE(pidl.empty());
}

/**
 * Initialising with NULL should set wrapped PIDL to NULL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( create_null, T, pidl_types )
{
	heap_pidl<T>::type pidl(NULL);
    BOOST_CHECK(!pidl.get());
	BOOST_CHECK(!pidl);
	BOOST_REQUIRE(pidl.empty());
}

/**
 * Initialising with valid PIDL should set wrapped PIDL to a non-null value.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( create_non_null, T, pidl_types )
{
	heap_pidl<T>::type pidl(fake_pidl<T>());
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
	heap_pidl<T>::type pidl(reinterpret_cast<const T*>(&empty));
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
	shared_ptr<IDRELATIVE> combined_pidl(
		raw_pidl::combine<newdelete_alloc<IDRELATIVE> >(pidl1, pidl2));

	shared_ptr<IDRELATIVE> expected(
		reinterpret_cast<PIDLIST_RELATIVE>(::ILCombine(
			reinterpret_cast<PCUIDLIST_ABSOLUTE>(pidl1), pidl2)),
		::ILFree);

	BOOST_REQUIRE(binary_equal_pidls(combined_pidl.get(), expected.get()));
}

/**
 * Join PIDLs to an absolute PIDL (raw).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( combine_abs, T, relative_pidl_types )
{
	const IDABSOLUTE* pidl1 = fake_pidl<IDABSOLUTE>();
	const T* pidl2 = fake_pidl<T>();
	do_combine_test(pidl1, pidl2);
}

/**
 * Join PIDLs to a relative PIDL (raw).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( combine_rel, T, relative_pidl_types )
{
	const IDRELATIVE* pidl1 = fake_pidl<IDRELATIVE>();
	const T* pidl2 = fake_pidl<T>();
	do_combine_test(pidl1, pidl2);
}

/**
 * Join PIDLs to a child PIDL (raw).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( combine_child, T, relative_pidl_types )
{
	const IDCHILD* pidl1 = fake_pidl<IDCHILD>();
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
	heap_pidl<T>::type pidl(fake_pidl<T>());

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
	heap_pidl<T>::type pidl(empty_pidl);

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
	heap_pidl<T>::type pidl;
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
	heap_pidl<T>::type pidl(fake_pidl<T>());
	heap_pidl<T>::type pidl_copy(pidl);

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
	heap_pidl<T>::type pidl(fake_pidl<T>());
	heap_pidl<T>::type pidl_copy;
	pidl_copy = pidl;

	BOOST_REQUIRE(binary_equal_pidls(pidl.get(), pidl_copy.get()));

	BOOST_REQUIRE_NE(pidl.get(), pidl_copy.get());
}

/**
 * Copy basic_pidl to a raw PIDL.
 * The data in both pidls must be the same but their addresses should
 * be different (i.e. a copy).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( copy_to, T, pidl_types )
{
	heap_pidl<T>::type pidl(fake_pidl<T>());
	T* raw;

	pidl.copy_to(raw);

	shared_ptr<T> scope(raw);

	BOOST_REQUIRE(binary_equal_pidls(pidl.get(), raw));

	BOOST_REQUIRE_NE(pidl.get(), raw);
}

/**
 * Attach a basic_pidl to a raw PIDL.
 * The wrapped PIDL should have the same address as the orginal.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( attach, T, pidl_types )
{
	heap_pidl<T>::type pidl;
	
	T* raw = raw_pidl::clone<newdelete_alloc<T>>(fake_pidl<T>());
	pidl.attach(raw);

	BOOST_REQUIRE_EQUAL(pidl.get(), raw);
}

template<typename T, typename U>
void do_join_test(const T& pidl1, const U& pidl2)
{
	typedef typename T::join_type join_type;
	typedef typename T::join_pidl join_pidl;

	shared_ptr<join_type> expected(
		reinterpret_cast<join_type*>(::ILCombine(
			reinterpret_cast<PCUIDLIST_ABSOLUTE>(get(pidl1)), get(pidl2))),
		::ILFree);

	join_pidl joined_pidl = pidl1 + pidl2;

	BOOST_REQUIRE(binary_equal_pidls(joined_pidl.get(), expected.get()));
	BOOST_REQUIRE_NE(
		reinterpret_cast<const void*>(get(pidl1)), 
		reinterpret_cast<const void*>(joined_pidl.get()));
	BOOST_REQUIRE_NE(
		reinterpret_cast<const void*>(get(pidl2)), 
		reinterpret_cast<const void*>(joined_pidl.get()));
}

/**
 * Join different types of PIDL to a relative PIDL.
 * The result of joining two basic_pidls should be a newly allocated pidl
 * with the data from both.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_rel, T, relative_pidl_types )
{
	hpidl_t pidl1(fake_pidl<IDRELATIVE>());
	heap_pidl<T>::type pidl2(fake_pidl<T>());

	do_join_test(pidl1, pidl2);
}

/**
 * Join different types of PIDL to a child PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_child, T, relative_pidl_types )
{
	chpidl_t pidl1(fake_pidl<IDCHILD>());
	heap_pidl<T>::type pidl2(fake_pidl<T>());

	do_join_test(pidl1, pidl2);
}

/**
 * Join different types of PIDL to an absolute PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_abs, T, relative_pidl_types )
{
	typename heap_pidl<IDABSOLUTE>::type pidl1(fake_pidl<IDABSOLUTE>());
	typename heap_pidl<T>::type pidl2(fake_pidl<T>());

	do_join_test(pidl1, pidl2);
}

/**
 * Join different types of PIDL to a NULL PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_null_pidl, T, relative_pidl_types )
{
	heap_pidl<T>::type pidl1(NULL);
	heap_pidl<T>::type pidl2(fake_pidl<T>());

	do_join_test(pidl1, pidl2);
}

/**
 * Join a NULL PIDL to different types of PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_pidl_null, T, relative_pidl_types )
{
	heap_pidl<T>::type pidl1(fake_pidl<T>());
	heap_pidl<T>::type pidl2(NULL);

	do_join_test(pidl1, pidl2);
}

/**
 * Join different types of PIDL to an empty PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_empty_pidl, T, relative_pidl_types )
{
	SHITEMID empty = {0, {0}};
	heap_pidl<T>::type pidl1 = reinterpret_cast<const T*>(&empty);
	heap_pidl<T>::type pidl2(fake_pidl<T>());

	do_join_test(pidl1, pidl2);
}

/**
 * Join an empty PIDL to different types of PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_pidl_empty, T, relative_pidl_types )
{
	heap_pidl<T>::type pidl1(fake_pidl<T>());
	SHITEMID empty = {0, {0}};
	heap_pidl<T>::type pidl2 = reinterpret_cast<const T*>(&empty);

	do_join_test(pidl1, pidl2);
}

/**
 * Join different types of raw PIDL to a relative basic_pidl.
 * The result should be a newly allocated pidl with the data from both.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( join_raw, T, relative_pidl_types )
{
	pidl_t pidl1(fake_pidl<IDRELATIVE>());

	do_join_test(pidl1, fake_pidl<T>());
}

template<typename T, typename U, typename Alloc>
void do_append_test(basic_pidl<T, Alloc>& pidl1, const U& pidl2)
{
	shared_ptr<T> expected(
		reinterpret_cast<T*>(::ILCombine(
			reinterpret_cast<PCUIDLIST_ABSOLUTE>(get(pidl1)), get(pidl2))),
		::ILFree);

	pidl1 += pidl2;

	BOOST_REQUIRE(binary_equal_pidls(get(pidl1), expected.get()));
}

/**
 * Append different types of PIDL to a relative PIDL.
 * The left-hand PIDL should end up holding newly-allocated memory
 * with the data from both PIDLs.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_rel, T, relative_pidl_types )
{
	hpidl_t pidl1(fake_pidl<IDRELATIVE>());
	heap_pidl<T>::type pidl2(fake_pidl<T>());

	do_append_test(pidl1, pidl2);
}

/**
 * Append different types of PIDL to a child PIDL.
 * THIS TEST SHOULD FAIL TO COMPILE IF enable_if IS WORKING.
 */
/*
BOOST_AUTO_TEST_CASE_TEMPLATE( append_child, T, relative_pidl_types )
{
	chpidl_t pidl1(fake_pidl<IDCHILD>());
	heap_pidl<T>::type pidl2(fake_pidl<T>());

	do_append_test(pidl1, pidl2);
}
*/

/**
 * Append different types of PIDL to an absolute PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_abs, T, relative_pidl_types )
{
	ahpidl_t pidl1(fake_pidl<IDABSOLUTE>());
	heap_pidl<T>::type pidl2(fake_pidl<T>());

	do_append_test(pidl1, pidl2);
}

/**
 * Append different types of PIDL to a NULL PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_null_pidl, T, relative_pidl_types )
{
	hpidl_t pidl1(NULL);
	heap_pidl<T>::type pidl2(fake_pidl<T>());

	do_append_test(pidl1, pidl2);
}

/**
 * Append a NULL PIDL to different types of PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_pidl_null, T, relative_pidl_types )
{
	hpidl_t pidl1(fake_pidl<T>());
	heap_pidl<T>::type pidl2(NULL);

	do_append_test(pidl1, pidl2);
}

/**
 * Append different types of PIDL to an empty PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_empty_pidl, T, relative_pidl_types )
{
	SHITEMID empty = {0, {0}};
	hpidl_t pidl1 = reinterpret_cast<const T*>(&empty);
	heap_pidl<T>::type pidl2(fake_pidl<T>());

	do_append_test(pidl1, pidl2);
}

/**
 * Append an empty PIDL to different types of PIDL.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_pidl_empty, T, relative_pidl_types )
{
	hpidl_t pidl1(fake_pidl<T>());
	SHITEMID empty = {0, {0}};
	heap_pidl<T>::type pidl2 = reinterpret_cast<const T*>(&empty);

	do_append_test(pidl1, pidl2);
}

/**
 * Append a raw PIDL to a relative basic_pidl.
 * The basic_pidl should end up holding newly-allocated memory
 * with the data from both PIDLs.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( append_raw, T, relative_pidl_types )
{
	hpidl_t pidl1(fake_pidl<IDRELATIVE>());

	do_append_test(pidl1, fake_pidl<T>());
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

#pragma region basic_pidl tests related to pidl types
BOOST_FIXTURE_TEST_SUITE(basic_pidl_type_tests, PidlFixture)

/**
 * Test type violation detection.
 *
 * The test creates a raw non-child PIDL (one with more than one segment) that
 * masquerades as a child.  The type_checking in the child pidl constructor
 * should detect that the incoming raw pidl is a fraud.
 */
BOOST_AUTO_TEST_CASE( type_check_catch_violation )
{
	shared_ptr<IDCHILD> invalid_child(
		reinterpret_cast<IDCHILD*>(
			::ILCombine(fake_pidl<IDABSOLUTE>(), fake_pidl<IDRELATIVE>())),
		::ILFree);

	BOOST_REQUIRE_THROW(chpidl_t pidl(invalid_child.get()), std::invalid_argument);
}

/**
 * Test type violation detection for non-child PIDL types don't give false
 * positives.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( type_check_no_false_pos, T, adult_pidl_types )
{
	shared_ptr<T> double_pidl(
		reinterpret_cast<T*>(
			::ILCombine(fake_pidl<IDABSOLUTE>(), fake_pidl<IDRELATIVE>())),
		::ILFree);

	heap_pidl<T>::type pidl(double_pidl.get());
}

/**
 * Casting wrappers should work as it would for the underlying raw PIDLs.
 */
BOOST_AUTO_TEST_CASE( cast_wrapped_to_wrapped  )
{
	hpidl_t rpidl;
	ahpidl_t apidl;
	chpidl_t cpidl;

	// Upcast
	rpidl = apidl;
	rpidl = cpidl;

	// Implicit downcasts - should give a compile-time ERROR
	// apidl = rpidl;
	// cpidl = rpidl;

	// Implicit crosscasts - should give a compile-time ERROR
	// apidl = cpidl;
	// cpidl = apidl;

	// Explicit downcasts with static_cast - should give a compile-time ERROR
	// apidl = static_cast<ahpidl_t>(rpidl);
	// cpidl = static_cast<chpidl_t>(rpidl);

	// Explicit downcasts with pidl_cast
	apidl = pidl_cast<ahpidl_t>(rpidl);
	cpidl = pidl_cast<chpidl_t>(rpidl);

	// Explicit crosscasts - should give a compile-time ERROR
	// apidl = static_cast<ahpidl_t>(cpidl);
	// cpidl = static_cast<chpidl_t>(apidl);
	// apidl = pidl_cast<ahpidl_t>(cpidl);
	// cpidl = pidl_cast<chpidl_t>(apidl);
}

/**
 * Casting wrappers should work as it would for the underlying raw PIDLs.
 */
BOOST_AUTO_TEST_CASE( cast_raw_to_wrapped  )
{
	heap_pidl<IDRELATIVE>::type rpidl;
	heap_pidl<IDABSOLUTE>::type apidl;
	heap_pidl<IDCHILD>::type cpidl;

	IDRELATIVE* raw_rpidl = NULL;
	IDABSOLUTE* raw_apidl = NULL;
	IDCHILD* raw_cpidl = NULL;

	// Upcast
	rpidl = raw_apidl;
	rpidl = raw_cpidl;

	// Implicit downcasts - should give a compile-time ERROR
	// apidl = raw_rpidl;
	// cpidl = raw_rpidl;

	// Implicit crosscasts - should give a compile-time ERROR
	// apidl = raw_cpidl;
	// cpidl = raw_apidl;

	// Explicit downcasts with static_cast - should give a compile-time ERROR
	// apidl = static_cast<heap_pidl<IDABSOLUTE>::type >(raw_rpidl);
	// cpidl = static_cast<heap_pidl<IDCHILD>::type >(raw_rpidl);

	// Explicit downcasts with pidl_cast
	apidl = pidl_cast<heap_pidl<IDABSOLUTE>::type >(raw_rpidl);
	cpidl = pidl_cast<heap_pidl<IDCHILD>::type >(raw_rpidl);

	// Explicit crosscasts - should give a compile-time ERROR
	// apidl = static_cast<heap_pidl<IDABSOLUTE>::type>(raw_cpidl);
	// cpidl = static_cast<heap_pidl<IDCHILD>::type >(raw_apidl);
	// apidl = pidl_cast<heap_pidl<IDABSOLUTE>::type >(raw_cpidl);
	// cpidl = pidl_cast<heap_pidl<IDCHILD>::type >(raw_apidl);
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion