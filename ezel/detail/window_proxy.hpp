/**
    @file

    Window implementation switcher.

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

#ifndef EZEL_DETAIL_WINDOW_PROXY_HPP
#define EZEL_DETAIL_WINDOW_PROXY_HPP
#pragma once

#include <boost/make_shared.hpp> // make_shared
#include <boost/noncopyable.hpp> // noncopyable
#include <boost/shared_ptr.hpp> // shared_ptr

#include <cassert> // assert

namespace ezel {
namespace detail {

/**
 * Switch between two window implementations.
 *
 * One implementation is a real wrapper round an HWND, the other just pretends.
 * This class serves up a window wrapper that works correctly whether or not
 * it is attached to an real Win32 window object.
 *
 * The purpose of this class is to manage the change from an artificial
 * window to an attached window and vice versa.  The idea is that an instance
 * of this class will responds correctly to property setting/getting
 * regardless of whether the window is real or fake.
 *
 * As the fake window type may have an arbitrary contructor signature, the
 * client must pass an instance to our constructor.  The real window type
 * we instantiate ourselves and must have the constructor:
 *    @code RealType(HWND hwnd) @endcode
 *
 * Field data is synchronised between the real and fake windows by calling
 * @c copy_fields.  For any type you use as the @c Interface template 
 * parameter you must implement @c copy_fields in the same namespace with this
 * signature:
 *    @code copy_fields(Interface& source, Interface& target) @endcode
 * It will be found by ADL.
 */
template<typename Interface, typename FakeType, typename RealType>
class window_proxy
{
public:
    window_proxy(boost::shared_ptr<FakeType> fake)
        : m_fake_window(fake), m_active_window(m_fake_window) {}

    Interface* operator->() { return m_active_window; }
    Interface& operator*() { return *m_active_window; }
    const Interface* operator->() const { return m_active_window; }
    const Interface& operator*() const { return *m_active_window; }

    /**
     * Switch from fake to real window.
     */
    void attach(HWND hwnd)
    {
        assert(!m_real_window); // why are we attaching twice?
        assert(m_active_window == m_fake_window); // fake window not active one

        m_real_window = boost::make_shared<RealType>(hwnd);
        m_active_window = m_real_window;
    }

    /**
     * Switch back to the fake window.
     */
    void detach()
    {
        assert(m_real_window); // why are not attached?
        assert(m_active_window == m_real_window); // real window not active one

        m_real_window.reset();
        m_active_window = m_fake_window;
    }

    /**
     * Suck data from real Win32 window object into the fake window.
     *
     * This method exists so that properties of the window are still available
     * after the real window has been destroyed.
     *
     * Call this method after receiving the WM_CREATE message so that
     * @c copy_fields() can rely on the integrity of the window fields.
     */
    void pull()
    {
        assert(m_real_window); // must not call this method unless attached

        copy_fields(real(), fake());
    }

    /**
     * Update Win32 window object from fields in the fake window.
     *
     * Fields can be set in the wrapper before the Win32 window is created.
     * This window pushes those values out to the real window once it is
     * created.
     *
     * Call this method before receiving the WM_DESTROY message so that
     * @c copy_fields() can rely on the integrity of the window fields.
     */
    void push()
    {
        assert(m_real_window); // must not call this method unless attached

        copy_fields(fake(), real());
    }

private:
    Interface& real() { return *m_real_window; }
    Interface& fake() { return *m_fake_window; }

    boost::shared_ptr<FakeType> m_fake_window;
    boost::shared_ptr<RealType> m_real_window;
    boost::shared_ptr<Interface> m_active_window;
};

}} // namespace ezel::detail

#endif