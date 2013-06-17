/**
    @file

    Helper functions for Boost.Test,

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

#pragma once

#include <swish/utils.hpp>

#include <comet/error.h>

#include <boost/system/error_code.hpp>
#include <boost/test/test_tools.hpp> // predicate_result
#include <boost/filesystem.hpp>

#include <string>
#include <ostream>

namespace std {

    inline std::ostream& operator<<(
        std::ostream& out, const std::wstring& wide_in)
    {
        out << swish::utils::WideStringToUtf8String(wide_in);
        return out;
    }

    inline std::ostream& operator<<(
        std::ostream& out, const wchar_t* wide_in)
    {
        out << std::wstring(wide_in);
        return out;
    }

    inline std::ostream& operator<<(
        std::ostream& out, const boost::filesystem::wpath& path)
    {
        out << path.string();
        return out;
    }
}

namespace test {
namespace detail {

inline boost::test_tools::predicate_result s_ok(HRESULT hr)
{
    if (hr == S_OK)
    {
        boost::test_tools::predicate_result res(true);
        res.message() << "COM status code was S_OK";
        return res;
    }
    else
    {
        boost::test_tools::predicate_result res(false);
        res.message() << "COM status code was not S_OK: ";
        res.message() << comet::com_error(hr).s_str();
        return res;
    }
}

template<typename Itf>
inline boost::test_tools::predicate_result s_ok_error_info(
    comet::com_ptr<Itf> failure_source, HRESULT hr)
{
    if (hr == S_OK)
    {
        boost::test_tools::predicate_result res(true);
        res.message() << "COM status code was S_OK";
        return res;
    }
    else
    {
        boost::test_tools::predicate_result res(false);
        res.message() << "COM status code was not S_OK: ";
        res.message() <<
            comet::com_error_from_interface(failure_source, hr).s_str();
        return res;
    }
}

}
}

/**
 * COM HRESULT-specific assertions
 * @{
 */
#define BOOST_REQUIRE_OK(hr) BOOST_REQUIRE(test::detail::s_ok( (hr) ))
#define BOOST_CHECK_OK(hr) BOOST_CHECK(test::detail::s_ok( (hr) ))
#define BOOST_WARN_OK(hr) BOOST_WARN(test::detail::s_ok( (hr) ))

#define BOOST_REQUIRE_INTERFACE_OK(failure_source, hr) \
    BOOST_REQUIRE(test::detail::s_ok_error_info( (failure_source), (hr) ))
#define BOOST_CHECK_INTERFACE_OK(failure_source, hr) \
    BOOST_CHECK(test::detail::s_ok_error_info( (failure_source), (hr) ))
#define BOOST_WARN_INTERFACE_OK(failure_source, hr) \
    BOOST_WARN(test::detail::s_ok_error_info( (failure_source), (hr) ))
/* @} */
