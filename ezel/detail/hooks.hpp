/**
    @file

    Window creation hooks

    @if license

    Copyright (C) 2010, 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef EZEL_HOOKS_HPP
#define EZEL_HOOKS_HPP
#pragma once

#include <ezel/detail/window_impl.hpp> // window_impl

#include <winapi/hook.hpp> // windows_hook

#include <iostream> // cerr

#include <Winuser.h> // CallNextHookEx

namespace ezel {
namespace detail {

namespace native {

    /// @name CREATESTRUCT
    // @{
    template<typename T> struct create_struct;

    template<> struct create_struct<char>
    { typedef CREATESTRUCTA type; };

    template<> struct create_struct<wchar_t>
    { typedef CREATESTRUCTW type; };
    // @}

    /// @name CBT_CREATEWND
    // @{
    template<typename T> struct cbt_createwnd;

    template<> struct cbt_createwnd<char>
    { typedef CBT_CREATEWNDA type; };

    template<> struct cbt_createwnd<wchar_t>
    { typedef CBT_CREATEWNDW type; };
    // @}

}

template<typename T>
inline void handle_create(
    HWND hwnd, HWND /*insert_after*/,
    typename native::create_struct<T>::type& create_info)
{
    UNALIGNED wchar_t* data =
        static_cast<UNALIGNED wchar_t*>(create_info.lpCreateParams);

    if (data)
    {
        assert(*data == sizeof(wchar_t) + sizeof(window_impl*));
        data++; // skip size

        window_impl* w = *reinterpret_cast<window_impl**>(data);
        w->attach(hwnd);
    }
}

template<typename T>
inline void handle_destroy(HWND hwnd)
{
    window_impl* this_window =
        fetch_user_window_data<T, window_impl*>(hwnd);
    this_window->detach();
}

/**
 * Hooking procedure called by Windows any time a GUI event happens.
 *
 * This function captures window creation and establishes a two-way link
 * between the Win32 window object and the C++ wrapper object.
 *
 * @todo  Pass *this* hook as first param for Windows 9x.
 */
template<typename T>
inline LRESULT CALLBACK cbt_hook_function(
    int code, WPARAM wparam, LPARAM lparam)
{
    try
    {
        if (code == HCBT_CREATEWND)
        {
            HWND hwnd = reinterpret_cast<HWND>(wparam);

            typedef typename native::cbt_createwnd<T>::type* cbt_param;

            cbt_param cbt_info = reinterpret_cast<cbt_param>(lparam);

            handle_create<T>(
                hwnd, cbt_info->hwndInsertAfter, *(cbt_info->lpcs));
        }
        else if (code == HCBT_DESTROYWND)
        {
            //handle_destroy<T>(reinterpret_cast<HWND>(wparam));
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what();
    }

    // TODO: pass *this* hook as first param for Windows 9x
    LRESULT rc = ::CallNextHookEx(NULL, code, wparam, lparam);
    return rc;
}

/**
 * Sets up and tears down window event hooks.
 *
 * Declare one static instance of this class in order to register window
 * creation.
 */
template<typename T>
class creation_hooks
{
public:
    creation_hooks()
        :
        m_cbt_hook(winapi::windows_hook(WH_CBT, &cbt_hook_function<T>)) {}

private:
    winapi::hhook m_cbt_hook;
};

}} // namespace ezel::detail

#endif
