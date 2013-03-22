/**
    @file

    Reporting exceptions to the user.

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

#include "announce_error.hpp"

#include "swish/frontend/bind_best_taskdialog.hpp" // best_taskdialog

#include <winapi/gui/task_dialog.hpp> // task_dialog

#include <comet/error.h> // com_error

#include <boost/exception/diagnostic_information.hpp>
#include <boost/locale.hpp> // translate

#include <cassert> // assert
#include <exception>
#include <sstream> // wstringstream

using swish::frontend::best_taskdialog;

using namespace winapi::gui::task_dialog;

using comet::com_error;

using boost::locale::translate;
using boost::diagnostic_information;

using std::exception;
using std::wstring;
using std::wstringstream;

namespace {

    wstring hexify_hr(HRESULT hr)
    {
        wstringstream stream;
        stream << std::hex << std::showbase << hr;
        return stream.str();
    }

#define SWISH_ANNOUNCE_ERROR_HRESULT_CASE(hr) case (hr): return L#hr;

    wstring hresult_code(HRESULT hr)
    {
        switch (hr)
        {
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(S_OK);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(S_FALSE);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(E_UNEXPECTED);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(E_NOTIMPL);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(E_OUTOFMEMORY);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(E_INVALIDARG);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(E_NOINTERFACE);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(E_POINTER);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(E_HANDLE);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(E_ABORT);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(E_FAIL);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(E_ACCESSDENIED);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(E_PENDING);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_CANTSAVE);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_INCOMPLETE);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_FILENOTFOUND);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_ACCESSDENIED);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_UNIMPLEMENTEDFUNCTION);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_INVALIDHANDLE);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_FILEALREADYEXISTS);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_DISKISWRITEPROTECTED);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_MEDIUMFULL);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_LOCKVIOLATION);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_INVALIDPARAMETER);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_INVALIDFUNCTION);
            SWISH_ANNOUNCE_ERROR_HRESULT_CASE(STG_E_INSUFFICIENTMEMORY);
        default:
            return hexify_hr(hr);
        }
    }

#undef SWISH_ANNOUNCE_ERROR_HRESULT_CASE

    /**
     * @todo  Convert narrow to wide strings properly.
     */
    wstring format_exception(const exception& error)
    {
        wstringstream details;

        details << error.what();
        details << "\n\nBug report information: "
            << diagnostic_information(error).c_str();

        return details.str();
    }

    wstring format_exception(const com_error& error)
    {
        wstringstream details;

        details << error.w_str();
        details << L"\n\nHRESULT: " << hresult_code(error.hr());
        details << "\n\nBug report information: "
            << diagnostic_information(error).c_str();

        return details.str();
    }
}

namespace swish {
namespace frontend {

void announce_error(
    HWND hwnd, const wstring& problem, const wstring& suggested_resolution,
    const wstring& details)
{
    task_dialog<void, best_taskdialog> td(
        hwnd, problem, suggested_resolution, L"Swish", icon_type::error, true);
    td.extended_text(
        details, expansion_position::below,
        initial_expansion_state::default,
        translate("Show &details (which may not be in your language)"),
        translate("Hide &details"));
    td.show();
}

void rethrow_and_announce(
    HWND hwnd, const wstring& title, const wstring& suggested_resolution,
    bool force_ui)
{
    // Only try and announce if we have an owner window
    if (!force_ui && hwnd == NULL)
        throw; 

    // Each call to announce_error below is guarded with a try/catch.
    // I've tested these catch handler and they works the way I
    // expected: they swallows the newly thrown exception allowing the
    // 'throw' statement at the bottom to rethrow the original exception.
    // XXX: I can't find whether this is guaranteed by the C++
    // standard.  Implementing this behaviour must mean maintaining
    // some form of thrown exception stack.
    try
    {
        throw;
    }
    catch (const com_error& error)
    {
        try
        {
            if (error.hr() != E_ABORT)
                announce_error(
                    hwnd, title, suggested_resolution, format_exception(error));
        }
        catch (...) { assert(!"Exception announcer threw new exception"); }

        throw;
    }
    catch (const exception& error)
    {
        try
        {
            announce_error(
                hwnd, title, suggested_resolution, format_exception(error));
        }
        catch (...) { assert(!"Exception announcer threw new exception"); }

        throw;
    }
#ifdef DEBUG
    catch (...)
    {
        try
        {
            announce_error(
                hwnd, title, suggested_resolution,
                L"Woooooo there soldier! Completely unrecognised type of "
                L"exception.");
        }
        catch (...) { assert(!"Exception announcer threw new exception"); }

        throw;
    }
#endif
}

}} // namespace swish::frontend
