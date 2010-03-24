/**
    @file

    Tests for dynamic linking and loading functions.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <winapi/dynamic_link.hpp> // test subject

#include <boost/function.hpp> // function
#include <boost/system/system_error.hpp> // system_error
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(utils_tests)

/**
 * load_library<char> returns hinstance
 */
BOOST_AUTO_TEST_CASE( load_library )
{
	winapi::hmodule hinst = winapi::load_library("kernel32.dll");
	BOOST_CHECK(hinst);
}

/**
 * load_library<wchar_t> returns hinstance
 */
BOOST_AUTO_TEST_CASE( load_library_w )
{
	winapi::hmodule hinst = winapi::load_library(L"kernel32.dll");
	BOOST_CHECK(hinst);
}

/**
 * load_library on unknown DLL should throw.
 */
BOOST_AUTO_TEST_CASE( load_library_fail )
{
	BOOST_CHECK_THROW(
		winapi::load_library("idontexist.dll"),
		boost::system::system_error);
}

/**
 * load_library on unknown DLL should throw.
 */
BOOST_AUTO_TEST_CASE( load_library_fail_w )
{
	BOOST_CHECK_THROW(
		winapi::load_library(L"idontexist.dll"),
		boost::system::system_error);
}

/**
 * module_handle<char> returns hinstance
 */
BOOST_AUTO_TEST_CASE( module_handle )
{
	HMODULE hinst = winapi::module_handle("kernel32.dll");
	BOOST_CHECK(hinst);
}

/**
 * module_handle<wchar_t> returns hinstance
 */
BOOST_AUTO_TEST_CASE( module_handle_w )
{
	HMODULE hinst = winapi::module_handle(L"kernel32.dll");
	BOOST_CHECK(hinst);
}

/**
 * module_handle on unknown DLL should throw.
 */
BOOST_AUTO_TEST_CASE( module_handle_fail )
{
	BOOST_CHECK_THROW(
		winapi::module_handle("idontexist.dll"),
		boost::system::system_error);
}

/**
 * module_handle on unknown DLL should throw.
 */
BOOST_AUTO_TEST_CASE( module_handle_fail_w )
{
	BOOST_CHECK_THROW(
		winapi::module_handle(L"idontexist.dll"),
		boost::system::system_error);
}

/**
 * Current module handle must always succeed.
 */
BOOST_AUTO_TEST_CASE( current_module_handle )
{
	HMODULE hinst = winapi::module_handle();
	BOOST_CHECK(hinst);
}

/**
 * Call known function using dynamic binding.
 */
BOOST_AUTO_TEST_CASE( proc_address )
{
	boost::function<int (int)> func;
	func = winapi::proc_address<int (WINAPI *)(int)>(
		"user32.dll", "GetKeyboardType");
	BOOST_CHECK_EQUAL(func(0), ::GetKeyboardType(0));
}

/**
 * Call known function using dynamic binding.
 */
BOOST_AUTO_TEST_CASE( proc_address_w )
{
	boost::function<int (int)> func;
	func = winapi::proc_address<int (WINAPI *)(int)>(
		L"user32.dll", "GetKeyboardType");
	BOOST_CHECK_EQUAL(func(0), ::GetKeyboardType(0));
}

BOOST_AUTO_TEST_SUITE_END();
