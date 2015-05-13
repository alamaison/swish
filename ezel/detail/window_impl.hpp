/**
    @file

    HWND wrapper implementation.

    @if license

    Copyright (C) 2010, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef EZEL_DETAIL_WINDOW_IMPL_HPP
#define EZEL_DETAIL_WINDOW_IMPL_HPP
#pragma once

#include <ezel/detail/command_dispatch.hpp> // command_map
#include <ezel/detail/message_dispatch.hpp> // message_map
#include <ezel/detail/window_link.hpp> // window_link
#include <ezel/detail/window_proc.hpp> // window_proc_base, window_proc
#include <ezel/detail/window_proxy.hpp> // window_proxy

#include <washer/gui/messages.hpp> // message
#include <washer/gui/commands.hpp> // command
#include <washer/trace.hpp> // trace
#include <washer/window/window.hpp>

#include <boost/exception/diagnostic_information.hpp> // diagnostic_information
#include <boost/make_shared.hpp> // make_shared
#include <boost/noncopyable.hpp> // noncopyable
#include <boost/signal.hpp> // signal

#include <cassert> // assert
#include <string>

namespace ezel {
    
/**
 * We are EXPLICITLY bringing the washer::gui::message/command classes into
 * the ezel namespace.
 */
using washer::gui::message;
using washer::gui::command;

namespace detail {

void catch_form_creation(HWND hwnd, unsigned int msg, LPARAM lparam);
void catch_form_destruction(HWND hwnd, unsigned int msg);

LRESULT CALLBACK window_impl_proc(
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

class window_impl;

/**
 * Fetch C++ wrapper pointer embedded in WND data.
 */
inline window_impl* window_from_hwnd(HWND hwnd)
{
    return fetch_user_window_data<wchar_t, window_impl*>(hwnd);
}

/**
 * Interface for window wrappers used internally by window_impl.
 *
 * There are two implementations of this interface.  One wraps a real Win32
 * window which window_impl uses once it's been attached to an HWND.  The
 * other only simulates a window to holds the properties that a window_impl
 * in initialised with as well as to reflect any property changes made before
 * the real window is created.
 */
template<typename T>
class internal_window
{
public:
    virtual std::basic_string<T> text() const = 0;
    virtual void text(const std::basic_string<T>& new_text) = 0;

    virtual bool is_visible() = 0;
    virtual bool is_enabled() = 0;
    virtual void visible(bool state) = 0;
    virtual void enable(bool state) = 0;

    virtual short left() const = 0;
    virtual short top() const = 0;
    virtual short width() const = 0;
    virtual short height() const = 0;
};

/**
 * Fake window holding properties before real window is attached.
 *
 * The purpose of this class is to maintain any properties which are set on
 * a wrapper before it has been attached to a real Win32 window.  It
 * simulates the fields in the real window.
 */
template<typename T>
class fake_window : public internal_window<T>
{
public:
    fake_window(
        bool is_enabled, bool is_visible, const std::basic_string<T>& text,
        short left, short top, short width, short height)
        :
        m_enabled(is_enabled), m_visible(is_visible), m_text(text),
        m_left(left), m_top(top), m_width(width), m_height(height) {}

    std::basic_string<T> text() const { return m_text; }
    void text(const std::basic_string<T>& new_text) { m_text = new_text; }

    bool is_visible() { return m_visible; }
    bool is_enabled() { return m_enabled; }
    void visible(bool state) { m_visible = state; }
    void enable(bool state) { m_enabled = state; }

    short left() const { return m_left; }
    short top() const { return m_top; }
    short width() const { return m_width; }
    short height() const { return m_height; }

private:
    std::basic_string<T> m_text;

    bool m_enabled;
    bool m_visible;

    short m_left;
    short m_top;
    short m_width;
    short m_height;
};

/**
 * Wrapper around a real Win32 window.
 */
template<typename T>
class real_window : public internal_window<T>
{
public:
    real_window(HWND hwnd) :
        m_window(washer::window::window_handle::foster_handle(hwnd))
    {
        if (hwnd == NULL)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid window handle"));
    }

    /**
     * Window text.
     *
     * @note  We could allow the caller to specify that they require narrow or
     *        wide string irrespective of whether this window is narrow or
     *        wide.  However, the fake_window doesn't support this and
     *        this class must match its interface.
     */
    std::basic_string<T> text() const { return window().text<T>(); }

    /**
     * Window text.
     *
     * @note  We could allow the caller to pass this method a narrow or wide
     *        string irrespective of whether this window is narrow or wide.
     *        However, the fake_window doesn't support this and this
     *        class must match its interface.
     */
    void text(const std::basic_string<T>& new_text)
    { window().text(new_text); }

    bool is_visible() { return window().is_visible(); }
    bool is_enabled() { return window().is_enabled(); }

    void visible(bool state) { window().visible(state); }
    void enable(bool state) { window().enable(state); }

    short left() const { return window().position().left(); }
    short top() const { return window().position().top(); }
    short width() const { return window().position().width(); }
    short height() const { return window().position().height(); }

private:

    washer::window::window<T>& window() { return m_window; }
    const washer::window::window<T>& window() const { return m_window; }

    washer::window::window<T> m_window;
};

template<typename T>
void copy_fields(internal_window<T>& source, internal_window<T>& target)
{
    target.text(source.text());
    target.enable(source.is_enabled());
    target.visible(source.is_visible());
}

/**
 * Window handle (HWND) wrapper class.
 *
 * Only one instance must exist per HWND so this class in non-copyable.
 * Clients use it via facade classes that reference the single instances
 * via a shared pointer.
 *
 * The lifetime of the class has three phases:
 *
 *  - before it is connected to an HWND.
 *    The data in the fields of this class are the data that the Win32 window
 *    will be initialised with (via a dialog template) when the Window dialog
 *    manager calls CreateWindow.
 *
 *  - while connected to an HWND.
 *    Method of this class fetch their data directly from the Win32 object.
 *    Member fields are ignored.
 *
 *  - after detaching from an HWND (when the Win32 window is destroyed)
 *    The Win32 data is pulled in just before destruction and stored in the
 *    member fields.  Subsequent method calls use this data.
 */
class window_impl : private boost::noncopyable
{
public:

    typedef window_impl super; // end of dispatch chain

    typedef message_map<
        WM_CREATE, WM_DESTROY, WM_NCDESTROY, WM_SETTEXT, WM_SHOWWINDOW>
        messages;
    typedef command_map<-1> commands;

    virtual LRESULT handle_message(
        UINT message_id, WPARAM wparam, LPARAM lparam)
    {
        return dispatch_message(this, message_id, wparam, lparam);
    }

    virtual void handle_command(
        WORD command_id, WPARAM wparam, LPARAM lparam)
    {
        dispatch_command(this, command_id, wparam, lparam);
    }

    window_impl(
        const std::wstring& text, short left, short top, short width,
        short height)
        :
        m_window(
            boost::make_shared< fake_window<wchar_t> >(
                true, true, text, left, top, width, height)) {}

    virtual ~window_impl()
    {
        assert(!m_link.attached()); // why have we not detached?
    }

    bool is_active() const { return m_link.attached(); }

    virtual std::wstring window_class() const = 0;
    virtual DWORD style() const
    {
        return WS_VISIBLE | WS_TABSTOP;
    }

    short left() const { return window().left(); }
    short top() const { return window().top(); }
    short width() const { return window().width(); }
    short height() const { return window().height(); }

    std::wstring text() const { return window().text(); }
    void text(const std::wstring& new_text) { window().text(new_text); }

    void visible(bool state) { window().visible(state); }
    void enable(bool state) { window().enable(state); }

    /// @name Event handlers
    // @{

    /**
     * @name Lifetime events
     *
     * The main purpose of these handlers is to synchronise C++ wrapper
     * and the real Win32 window.  The wrapper's fields can be set before the
     * real window is created and callers need access to the fields after the
     * real window is destroyed.  Therefore we push the data out to the
     * window when it's created and pull it back just before it's destroyed.
     *
     * We do this with the @c WM_CREATE and @c WM_DESTROY messages (rather
     * than @c WM_NCCREATE and @c WM_NCDESTROY) as we can't be sure of the
     * fields' integrity outside of this 'safe zone'.  For example, when
     * common controls 6 is enabled setting an icon before @c WM_CREATE
     * fails to show the icon.
     */

    LRESULT on(message<WM_CREATE> m)
    {
        LRESULT res = default_message_handler(m);
        push();
        return res;
    }

    LRESULT on(message<WM_DESTROY> m)
    {
        pull();
        return default_message_handler(m);
    }

    /**
     * To prevent capturing creation of windows not directly part of our
     * dialog template, such as the system menu, we engage our CBT hook for
     * as short a period as possible.  Therefore we have to detach ourselves
     * in this function rather than from the CBT hook.
     *
     * @see ezel::detail::cbt_hook_function.
     */
    LRESULT on(message<WM_NCDESTROY> m)
    {
        LRESULT res = default_message_handler(m);
        detach();
        return res;
    }

    // @}

    LRESULT on(message<WM_SETTEXT> m)
    {
        m_on_text_change(m.text<wchar_t>());
        LRESULT res = default_message_handler(m);
        m_on_text_changed();
        return res;
    }

    LRESULT on(message<WM_SHOWWINDOW> m)
    {
        m_on_showing(m.state());
        LRESULT res = default_message_handler(m);
        m_on_show(m.state());
        return res;
    }

    // @}

    /**
     * Perform default processing for a message.
     *
     * This method must call the window procedure of the wrapped window.
     * Here it's done through a call to CallWindowProc but dialog windows
     * must do this differently so should override this method.
     */
    virtual LRESULT default_message_handler(
        UINT message_id, WPARAM wparam, LPARAM lparam)
    {
        return m_window_proc->do_default_handling(message_id, wparam, lparam);
    }

    template<UINT N>
    LRESULT default_message_handler(const message<N>& m)
    {
        return default_message_handler(N, m.wparam(), m.lparam());
    }

    /**
     * Default command handler.
     *
     * Command message that aren't handled elsewhere are dispatched to this
     * function.  By default, it does nothing.  Override if you want to
     * to handle unhandled command messages.
     */
    virtual void on(washer::gui::command_base unknown)
    {
        (void)unknown;
#ifdef _DEBUG
        window_impl* w = window_from_hwnd(unknown.control_hwnd());
        washer::trace(
            "Unhandled command (code 0x%x) from window with title '%s'")
            % unknown.command_code() % w->text();
#endif
    }

    /**
     * Establish a two-way link between this C++ wrapper object and the
     * Win32 window object.
     *
     * Also replace the Win32 window's message handling procedure (WNDPROC)
     * with our own so that we can intercept any messages it is sent. This
     * is otherwise known as subclassing.
     *
     * We don't push the wrapper fields out to the Win32 window yet as it's
     * much too early.  This is called by the CBT hook and at this point the
     * window hasn't even recieved an WM_NCCREATE message yet.
     *
     * @todo  What if we get a failure partway through?
     */
    void attach(HWND hwnd)
    {
        assert(!m_link.attached()); // an instance should only be attached once
        
        m_link = window_link<window_impl>(hwnd, this);
        m_window.attach(hwnd);

        install_window_procedure();
    }

    /// @name Event delegates
    // @{

    boost::signal<void (const wchar_t*)>& on_text_change()
    { return m_on_text_change; }

    boost::signal<void ()>& on_text_changed()
    { return m_on_text_changed; }

    boost::signal<void (bool)>& on_showing() { return m_on_showing; }
    boost::signal<void (bool)>& on_show() { return m_on_show; }

    // @}

protected:

    HWND hwnd() const { return m_link.hwnd(); }

    /**
     * Suck data from real Win32 window object into the wrapper class.
     *
     * This method exists so that properties of the window are still available
     * after the real window has been destroyed.
     *
     * Override this method when subclasses have other fields that need to
     * be sucked out of the window.  In most cases the overriding method must
     * call this method of the base class to synchronise all the fields.
     */
    virtual void pull()
    {
        m_window.pull();
    }

    /**
     * Update Win32 window object from fields in this wrapper class.
     *
     * Fields can be set in the wrapper before the Win32 window is created.
     * This window pushes those values out to the real window once it is
     * created.
     *
     * Override this method when subclasses have other fields that need to
     * be pushed out to the window.  In most cases the overriding method must
     * call this method of the base class to synchronise all the fields.
     *
     * @todo  Some of this pushing is redundant as the values are set in
     *        the dialogue template.  Not necessarily harmful but worth
     *        further thought.
     */
    virtual void push()
    {
        m_window.push();
    }

    boost::shared_ptr<window_proc_base>& window_procedure()
    { return m_window_proc; }

private:

    /**
     * Break the two-way link between this C++ wrapper object and the
     * Win32 window object.
     * 
     * The fields of the Win32 must have been pulled in by our window
     * proc when it recieved WM_DESTROY.  That message is the last point at
     * which we can be sure of the fields' integrity.
     *
     * @bug  If someone has subclassed us but not removed their hook by the
     *       time they pass us the the WM_NCDESTROY message (bad!) then we
     *       never remove our hooks as we're not at the bottom of the
     *       subclass chain.  The UpDown control seems to do this when
     *       it subclasses its buddy control.
     *
     * @todo  Investigate SetWindowSubclass/RemoveWindowSubclass and
     *        whether it might fix the unsubclassing bug.  Unfortunately,
     *        it may not work on earlier Windows versions.
     */
    void detach()
    {
        assert(m_link.attached()); // why are we detaching a detached wrapper?

        remove_window_procedure();

        m_window.detach();
        m_link = window_link<window_impl>();
    }

    /**
     * Replace the window's own Window proc with ours.
     */
    virtual void install_window_procedure()
    {
        window_procedure() =
            boost::make_shared<window_proc>(hwnd(), window_impl_proc);
    }

    /**
     * Remove our window proc and put back the one it came with.
     */
    virtual void remove_window_procedure()
    {
        window_procedure().reset();
    }

    internal_window<wchar_t>& window() { return *m_window; }
    const internal_window<wchar_t>& window() const { return *m_window; }

    window_link<window_impl> m_link;
    window_proxy<
        internal_window<wchar_t>, fake_window<wchar_t>, real_window<wchar_t> >
        m_window;
    boost::shared_ptr<window_proc_base> m_window_proc;

    /// @name Events
    // @{

    boost::signal<void (const wchar_t*)> m_on_text_change;
    boost::signal<void ()> m_on_text_changed;
    boost::signal<void (bool)> m_on_showing;
    boost::signal<void (bool)> m_on_show;

    // @}
};

/**
 * Custom window procedure for wrapping HWNDs and intercepting their
 * messages.
 */
inline LRESULT CALLBACK window_impl_proc(
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    try
    {
        window_impl* w = window_from_hwnd(hwnd);
        return w->handle_message(message, wparam, lparam);
    }
    catch (const std::exception& e)
    {
        // We should always be able to get our window.  If we were able to
        // replace the window proc with this one then we must have hooked
        // it correctly so why can't we find it now?

        washer::trace("window_impl_proc exception: %s")
            % boost::diagnostic_information(e);

        assert(!"Something went very wrong here - we couldn't get our window");

        return ::DefWindowProcW(hwnd, message, wparam, lparam);
    }
}

}} // namespace ezel::detail

#endif
