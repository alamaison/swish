/**
    @file

    Explorer shell view windows callback handler.

    @if license

    Copyright (C) 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "view_callback.hpp"

#include <washer/com/catch.hpp> // WASHER_COM_CATCH_AUTO_INTERFACE

#include <comet/error.h> // com_error

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert

using comet::com_error;

namespace {

    /// @name  Undocumented messages
    // @{
    const UINT SFVM_SELECTIONCHANGED = 8;
    const UINT SFVM_GET_WEBVIEW_CONTENT = 83;
    const UINT SFVM_GET_WEBVIEW_TASKS = 84;
    // @}

}

namespace swish {
namespace nse {

CViewCallback::~CViewCallback() {}

STDMETHODIMP CViewCallback::MessageSFVCB(
        UINT message, WPARAM wparam, LPARAM lparam)
{
    try
    {
        bool handled = false;
        switch (message)
        {
        case SFVM_WINDOWCREATED:
            handled = on_window_created(reinterpret_cast<HWND>(wparam));
            break;

        case SFVM_GETNOTIFY:
            if (wparam == 0 || lparam == 0)
                BOOST_THROW_EXCEPTION(com_error(E_POINTER));
            handled = on_get_notify(
                *reinterpret_cast<PCIDLIST_ABSOLUTE*>(wparam),
                *reinterpret_cast<LONG*>(lparam));
            break;

        case SFVM_FSNOTIFY:
            handled = on_fs_notify(
                reinterpret_cast<PCIDLIST_ABSOLUTE>(wparam), lparam);
            break;

        case SFVM_MERGEMENU:
            if (lparam == 0)
                BOOST_THROW_EXCEPTION(com_error(E_POINTER));
            handled = on_merge_menu(*reinterpret_cast<QCMINFO*>(lparam));
            break;
          
        case SFVM_SELECTIONCHANGED:
            // wparam's meaning is unknown
            if (lparam == 0)
                BOOST_THROW_EXCEPTION(com_error(E_POINTER));
            handled = on_selection_changed(
                *reinterpret_cast<SFV_SELECTINFO*>(lparam));
            break;

        case SFVM_INITMENUPOPUP:
            handled = on_init_menu_popup(
                LOWORD(wparam), HIWORD(wparam),
                reinterpret_cast<HMENU>(lparam));
            break;

        case SFVM_INVOKECOMMAND:
            handled = on_invoke_command(wparam);
            break;

        case SFVM_GETHELPTEXT:
            handled = on_get_help_text(
                LOWORD(wparam), HIWORD(wparam), 
                reinterpret_cast<LPTSTR>(lparam));
            break;

        case SFVM_GET_WEBVIEW_CONTENT:
            if (lparam == 0)
                BOOST_THROW_EXCEPTION(com_error(E_POINTER));
            handled = on_get_webview_content(
                *reinterpret_cast<SFV_WEBVIEW_CONTENT_DATA*>(lparam));
            break;

        case SFVM_GET_WEBVIEW_TASKS:
            if (lparam == 0)
                BOOST_THROW_EXCEPTION(com_error(E_POINTER));
            handled = on_get_webview_tasks(
                *reinterpret_cast<SFV_WEBVIEW_TASKSECTION_DATA*>(lparam));
            break;

        default:
            handled = on_unknown_sfvm(message, wparam, lparam);
            break;
        }

        if (handled)
        {
            return S_OK;
        }
        else
        {
            // special treatment for FSNOTIFY because it uses S_FALSE to
            // suppress default processing.
            if (message == SFVM_FSNOTIFY)
                BOOST_THROW_EXCEPTION(com_error(S_FALSE));
            else
                BOOST_THROW_EXCEPTION(com_error(E_NOTIMPL));
        }
    }
    WASHER_COM_CATCH_AUTO_INTERFACE();

    assert(false && "Unreachable");
    return E_UNEXPECTED;
}

bool CViewCallback::on_unknown_sfvm(
    UINT /*message*/, WPARAM /*wparam*/, LPARAM /*lparam*/)
{ return false; }

bool CViewCallback::on_window_created(HWND /*hwnd_view*/)
{ return false; }

bool CViewCallback::on_get_notify(
    PCIDLIST_ABSOLUTE& /*pidl_monitor*/, LONG& /*events*/)
{ return false; }

bool CViewCallback::on_fs_notify(
    PCIDLIST_ABSOLUTE /*pidl*/, LONG /*event*/)
{ return false; }

bool CViewCallback::on_merge_menu(QCMINFO& /*menu_info*/)
{ return false; }

bool CViewCallback::on_selection_changed(
    SFV_SELECTINFO& /*selection_info*/)
{ return false; }

bool CViewCallback::on_init_menu_popup(
    UINT /*first_command_id*/, int /*menu_index*/, HMENU /*menu*/)
{ return false; }

bool CViewCallback::on_invoke_command(UINT /*command_id*/)
{ return false; }

bool CViewCallback::on_get_help_text(
    UINT /*command_id*/, UINT /*buffer_size*/, LPTSTR /*buffer*/)
{ return false; }

bool CViewCallback::on_get_webview_content(
    SFV_WEBVIEW_CONTENT_DATA& /*content_out*/) { return false; }

bool CViewCallback::on_get_webview_tasks(
    SFV_WEBVIEW_TASKSECTION_DATA& /*tasks_out*/) { return false; }

}} // namespace swish::nse
