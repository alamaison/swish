/**
    @file

    Component to handle user-interaction between the user and an SftpProvider.

    @if license

    Copyright (C) 2008, 2009, 2010, 2011, 2013
    Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "UserInteraction.hpp"

#include "swish/debug.hpp"
#include "swish/forms/password.hpp" // password_prompt
#include "swish/frontend/bind_best_taskdialog.hpp" // best_taskdialog
#include "swish/shell_folder/KbdInteractiveDialog.h" // Keyboard-interactive auth dialog box

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE
#include <winapi/gui/task_dialog.hpp> // task_dialog_builder
#include <winapi/gui/message_box.hpp> // message_box

#include <comet/error.h> // com_error

#include <boost/bind.hpp> // bind
#include <boost/format.hpp> // format
#include <boost/locale.hpp> // translate

#include <cassert> // assert
#include <sstream> // wstringstream
#include <string>

using swish::forms::password_prompt;
using swish::frontend::best_taskdialog;

using namespace winapi::gui;

using comet::com_error;

using boost::filesystem::path;
using boost::locale::translate;
using boost::optional;
using boost::wformat;

using std::pair;
using std::string;
using std::vector;
using std::wstring;
using std::wstringstream;

namespace swish {
namespace frontend {

CUserInteraction::CUserInteraction(HWND hwnd) : m_hwnd(hwnd) {}

/**
 * Displays UI dialog to get password from user and returns it.
 *
 * If no window given, abort authentication.
 */
optional<wstring> CUserInteraction::prompt_for_password()
{
    if (m_hwnd)
    {
        wstring password;
        if (password_prompt(
                m_hwnd, translate(L"Prompt on password dialog", L"Password:"),
                password))
        {
            return password;
        }
    }

    return optional<wstring>();
}


optional<pair<path, path>> CUserInteraction::key_files()
{
    // Swish doesn't use this way of pub-key auth - it uses Pageant via
    // the agent interface.  This method is only implemented by unit
    // test helpers.
    return optional<pair<path, path>>();
}

optional<vector<string>> CUserInteraction::challenge_response(
    const string& title, const string& instructions,
    const vector<pair<string, bool>>& prompts)
{
    if (!m_hwnd)
        BOOST_THROW_EXCEPTION(com_error("User interation forbidden", E_FAIL));

    // We don't show the dialog if there is nothing to tell the user.
    // Kb-int authentication usually seems to end with such an empty
    // interaction for some reason.
    if (title.empty() && instructions.empty() && prompts.empty())
    {
        // Not optional<vector<string>> because that means abort.
        return vector<string>();
    }

    // Show dialogue and fetch responses when user clicks OK
    CKbdInteractiveDialog dlg(title, instructions, prompts);
    if (dlg.DoModal(m_hwnd) == IDCANCEL)
        return optional<vector<string>>();

    return dlg.GetResponses();
}

namespace {

HRESULT on_confirm_overwrite(
    const wstring& old_file, const wstring& new_file, HWND hwnd)
{
    assert(hwnd);
    if (!hwnd)
        BOOST_THROW_EXCEPTION(com_error("User interation forbidden", E_FAIL));

    wstringstream message;
    
    message << wformat(
        translate(L"The folder already contains a file named '%1%'"))
        % old_file;
    message << L"\n\n";
    message << wformat(
        translate(
            L"Would you like to replace the existing file\n\n\t%1%\n\n"
            L"with this one?\n\n\t%2%")) % old_file % new_file;

    message_box::button_type::type button_clicked = message_box::message_box(
        hwnd, message.str(), translate(L"File already exists"),
        message_box::box_type::yes_no, message_box::icon_type::question, 2);

    switch (button_clicked)
    {
    case message_box::button_type::yes:
        return S_OK;
    case message_box::button_type::no:
    default:
        return E_ABORT;
    }
}

}

HRESULT CUserInteraction::OnConfirmOverwrite(
    BSTR bstrOldFile, BSTR bstrNewFile )
{
    try
    {
        return on_confirm_overwrite(bstrOldFile, bstrNewFile, m_hwnd);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();
}


namespace {

/** Click handler callback. */
HRESULT return_hr(HRESULT hr) { return hr; }

HRESULT on_hostkey_mismatch(
    const wstring& host, const wstring& key, const wstring& key_type,
    HWND hwnd)
{
    if (!hwnd)
        BOOST_THROW_EXCEPTION(com_error("User interation forbidden", E_FAIL));

    wstring title = translate(L"Mismatched host-key");
    wstring instruction = translate(L"WARNING: the SSH host-key has changed!");

    wstringstream message;
    
    message << wformat(
        translate(
            L"The SSH host-key sent by '%1%' to identify itself doesn't match "
            L"the known key for this server.  This could mean a third-party "
            L"is pretending to be the computer you're trying to connect to "
            L"or the system administrator may have just changed the key."))
        % host;
    message << L"\n\n";
    message << translate(
        L"It is important to check this is the right key fingerprint:");
    message << wformat(L"\n\n        %1%    %2%") % key_type % key;

    task_dialog::task_dialog_builder<HRESULT, best_taskdialog> td(
        hwnd, instruction, message.str(), title,
        winapi::gui::task_dialog::icon_type::warning, true,
        boost::bind(return_hr, E_ABORT));
    td.add_button(
        translate(
            L"I trust this key: &update and connect\n"
            L"You won't have to verify this key again unless it changes"),
        boost::bind(return_hr, S_OK));
    td.add_button(
        translate(
            L"I trust this key: &just connect\n"
            L"You will be warned about this key again next time you "
            L"connect"),
        boost::bind(return_hr, S_FALSE));
    td.add_button(
        translate(
            L"&Cancel\n"
            L"Choose this option unless you are sure the key is correct"),
        boost::bind(return_hr, E_ABORT), true);
    return td.show();
}

HRESULT on_hostkey_unknown(
    const wstring& host, const wstring& key, const wstring& key_type,
    HWND hwnd)
{
    if (!hwnd)
        BOOST_THROW_EXCEPTION(com_error("User interation forbidden", E_FAIL));

    wstring title = translate(L"Unknown host-key");

    wstringstream message;
    
    message << wformat(
        translate(
            L"The server '%1%' has identified itself with an SSH host-key "
            L"whose fingerprint is:")) % host;
    message << wformat(L"\n\n        %1%    %2%\n\n") % key_type % key;
    message << translate(
        L"If you are not expecting this key, a third-party may be pretending "
        L"to be the computer you're trying to connect to.");

    wstring instruction = translate(L"Verify unknown SSH host-key");
    task_dialog::task_dialog_builder<HRESULT, best_taskdialog> td(
        hwnd, instruction, message.str(), title,
        winapi::gui::task_dialog::icon_type::information, true,
        boost::bind(return_hr, E_ABORT));
    td.add_button(
        translate(
            L"I trust this key: &store and connect\n"
            L"You won't have to verify this key again unless it changes"),
        boost::bind(return_hr, S_OK));
    td.add_button(
        translate(
            L"I trust this key: &just connect\n"
            L"You will be asked to verify the key again next time you "
            L"connect"),
        boost::bind(return_hr, S_FALSE));
    td.add_button(
        translate(
            L"&Cancel\n"
            L"Choose this option unless you are sure the key is correct"),
        boost::bind(return_hr, E_ABORT), true);
    return td.show();
}
}

HRESULT CUserInteraction::OnHostkeyMismatch(
    BSTR bstrHostName, BSTR bstrHostKey, BSTR bstrHostKeyType)
{
    try
    {
        return on_hostkey_mismatch(
            bstrHostName, bstrHostKey, bstrHostKeyType, m_hwnd);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();
}

HRESULT CUserInteraction::OnHostkeyUnknown(
    BSTR bstrHostName, BSTR bstrHostKey, BSTR bstrHostKeyType)
{
    try
    {
        return on_hostkey_unknown(
            bstrHostName, bstrHostKey, bstrHostKeyType, m_hwnd);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();
}

}} // namespace swish::frontend
