/**
    @file

    HWND/wrapper linking.

    @if license

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

#ifndef EZEL_DETAIL_WINDOW_LINK_HPP
#define EZEL_DETAIL_WINDOW_LINK_HPP
#pragma once

#include <ezel/detail/hwnd_linking.hpp> // store_user_window_data

#include <washer/trace.hpp> // trace

#include <boost/exception/diagnostic_information.hpp> // diagnostic_information
#include <boost/noncopyable.hpp> // noncopyable
#include <boost/shared_ptr.hpp> // shared_ptr, make_shared

namespace ezel {
namespace detail {

/**
 * Link between a real Win32 window handle and a pointer to a window wrapper.
 *
 * The purpose of this class is to establish, maintain and then destroy a
 * two-way link between an HWND and a C++ wrapper instance.  The link
 * is broken when the instance is destroyed.  To explicitly break the link,
 * assign a broken link to the existing link:
 *     @code   link = window_link()   @endcode
 *
 * Clients can query the status of the linked by calling attached().
 *
 * Stores the instance pointer in the HWND's user data field.
 * @todo  Store the pointer somewhere else to prevent issues including other
 *        uses accidentally overwriting our pointer.
 */
template<typename T>
class window_link_helper : private boost::noncopyable
{
public:
    typedef T* pointer_type;

    /// Link HWND to wrapper.
    window_link_helper(HWND hwnd, pointer_type wrapper) : m_hwnd(hwnd)
    {
        store_user_window_data<wchar_t>(hwnd, wrapper);
    }

    /// Create a broken link.
    window_link_helper() : m_hwnd(NULL) {}

    /// Break link.
    ~window_link_helper()
    {
        try
        {
            if (attached())
                store_user_window_data<wchar_t>(m_hwnd, pointer_type());
        }
        catch (const std::exception& e)
        {
            washer::trace("Unlinking window threw exception: %s")
                % boost::diagnostic_information(e);
        }
    }

    HWND hwnd() const { return m_hwnd; }
    bool attached() const { return m_hwnd != NULL; }

private:
    HWND m_hwnd;
};

/**
 * Copyable link between a real Win32 window handle and a pointer to a window
 * wrapper.
 *
 * The purpose of this class make the link implementation in window_link_helper
 * break the link only when the last copy goes out-of-scope.
 *
 * The methods are the same as window_link_helper.
 */
template<typename T>
class window_link
{
public:
    typedef T* pointer_type;

    /// Link HWND to wrapper.
    window_link(HWND hwnd, pointer_type wrapper)
        : m_link(boost::make_shared<window_link_helper<T> >(hwnd, wrapper)) {}

    /// Create a broken link.
    window_link() : m_link(boost::make_shared<window_link_helper<T> >()) {}

    HWND hwnd() const { return m_link->hwnd(); }
    bool attached() const { return m_link->attached(); }

private:
    boost::shared_ptr<window_link_helper<T> > m_link;
};

}} // namespace ezel::detail

#endif
