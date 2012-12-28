/**
    @file

    Component to handle user-interaction between the user and an SftpProvider.

    @if license

    Copyright (C) 2008, 2009, 2010, 2011
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

#include "swish/atl.hpp"  // Common ATL setup
#include "swish/debug.hpp"
#include "swish/forms/password.hpp" // password_prompt
#include "swish/frontend/bind_best_taskdialog.hpp" // best_taskdialog
#include "swish/shell_folder/KbdInteractiveDialog.h" // Keyboard-interactive auth dialog box

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE
#include <winapi/gui/task_dialog.hpp> // task_dialog
#include <winapi/gui/message_box.hpp> // message_box

#include <comet/bstr.h> // bstr_t
#include <comet/error.h> // com_error

#include <boost/bind.hpp> // bind
#include <boost/format.hpp> // format
#include <boost/locale.hpp> // translate

#include <atlsafe.h> // CComSafeArray
#include <atlcore.h> // _AtlBaseModule

#include <cassert> // assert
#include <sstream> // wstringstream
#include <string>

using ATL::CComBSTR;
using ATL::CString;
using ATL::CComSafeArray;

using swish::forms::password_prompt;
using swish::frontend::best_taskdialog;

using namespace winapi::gui;

using comet::bstr_t;
using comet::com_error;

using boost::locale::translate;
using boost::wformat;

using std::wstringstream;
using std::string;
using std::wstring;

namespace swish {
namespace frontend {

CUserInteraction::CUserInteraction(HWND hwnd) : m_hwnd(hwnd) {}

/**
 * Displays UI dialog to get password from user and returns it.
 *
 * @param [in]  bstrRequest    The prompt to display to the user.
 * @param [out] pbstrPassword  The reply from the user - the password.
 *
 * @return E_ABORT if the user chooses Cancel, E_FAIL if user interaction is
 *         forbidden and S_OK otherwise.
 */
HRESULT CUserInteraction::OnPasswordRequest(
    BSTR bstrRequest, BSTR *pbstrPassword
)
{
    if (m_hwnd == NULL)
        return E_FAIL;

    try
    {
        wstring password;
        if (!password_prompt(m_hwnd, bstrRequest, password))
            BOOST_THROW_EXCEPTION(com_error(E_ABORT));

        *pbstrPassword = bstr_t::detach(bstr_t(password));
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();
        
    return S_OK;
}

HRESULT CUserInteraction::OnKeyboardInteractiveRequest(
    BSTR bstrName, BSTR bstrInstruction, SAFEARRAY *psaPrompts, 
    SAFEARRAY *psaShowResponses, SAFEARRAY **ppsaResponses
)
{
    if (m_hwnd == NULL)
        return E_FAIL;

    CComSafeArray<BSTR> saPrompts(psaPrompts);
    CComSafeArray<VARIANT_BOOL> saShowPrompts(psaShowResponses);

    // Prompt array and echo mask array should correspond
    ATLASSERT(saPrompts.GetLowerBound() == saShowPrompts.GetLowerBound());
    ATLASSERT(saPrompts.GetUpperBound() == saShowPrompts.GetUpperBound());

    PromptList vecPrompts;
    EchoList vecEcho;
    for (int i = saPrompts.GetLowerBound(); i <= saPrompts.GetUpperBound(); i++)
    {
        vecPrompts.push_back(CString(saPrompts[i]));
        vecEcho.push_back((saShowPrompts[i] == VARIANT_TRUE) ? true : false);
    }

    // Show dialogue and fetch responses when user clicks OK
    CKbdInteractiveDialog dlg(bstrName, bstrInstruction, vecPrompts, vecEcho);
    if (dlg.DoModal(m_hwnd) == IDCANCEL)
        return E_ABORT;
    ResponseList vecResponses = dlg.GetResponses();

    // Create response array. Indices must correspond to prompts array!
    CComSafeArray<BSTR> saResponses(
        saPrompts.GetCount(), saPrompts.GetLowerBound());
    ATLASSERT(vecResponses.size() == saPrompts.GetCount());
    ATLASSERT(saPrompts.GetLowerBound() == saResponses.GetLowerBound());
    ATLASSERT(saPrompts.GetUpperBound() == saResponses.GetUpperBound());

    // SAFEARRAY may start at any index but vector will always start at 0.
    // We need to keep an offset value to map between them
    int nIndexOffset = saPrompts.GetLowerBound();

    // Fill responses SAFEARRAY
    for (int i = saPrompts.GetLowerBound(); i <= saPrompts.GetUpperBound(); i++)
    {
        ATLVERIFY(SUCCEEDED( saResponses.SetAt(
            i, CComBSTR(vecResponses[i-nIndexOffset]).Detach(), false) ));
    }

    *ppsaResponses = saResponses.Detach();

    return S_OK;
}

/**
 * Return the path of the file containing the private key.
 */
HRESULT CUserInteraction::OnPrivateKeyFileRequest(
    BSTR *pbstrPrivateKeyFile)
{
    ATLENSURE_RETURN_HR(pbstrPrivateKeyFile, E_POINTER);
    *pbstrPrivateKeyFile = NULL;

    return E_NOTIMPL;
}

/**
 * Return the path of the file containing the public key.
 */
HRESULT CUserInteraction::OnPublicKeyFileRequest(
    BSTR *pbstrPublicKeyFile)
{
    ATLENSURE_RETURN_HR(pbstrPublicKeyFile, E_POINTER);
    *pbstrPublicKeyFile = NULL;

    return E_NOTIMPL;
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
        translate("The folder already contains a file named '%1%'"))
        % old_file;
    message << L"\n\n";
    message << wformat(
        translate(
            "Would you like to replace the existing file\n\n\t%1%\n\n"
            "with this one?\n\n\t%2%")) % old_file % new_file;

    message_box::button_type::type button_clicked = message_box::message_box(
        hwnd, message.str(), translate("File already exists"),
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

    wstring title = translate("Mismatched host-key");
    wstring instruction = translate("WARNING: the SSH host-key has changed!");

    wstringstream message;
    
    message << wformat(
        translate(
            "The SSH host-key sent by '%1%' to identify itself doesn't match "
            "the known key for this server.  This could mean a third-party "
            "is pretending to be the computer you're trying to connect to "
            "or the system administrator may have just changed the key."))
        % host;
    message << L"\n\n";
    message << translate(
        "It is important to check this is the right key fingerprint:");
    message << wformat(L"\n\n        %1%    %2%") % key_type % key;

    task_dialog::task_dialog<HRESULT, best_taskdialog> td(
        hwnd, instruction, message.str(), title,
        winapi::gui::task_dialog::icon_type::warning, true,
        boost::bind(return_hr, E_ABORT));
    td.add_button(
        translate(
            "I trust this key: &update and connect\n"
            "You won't have to verify this key again unless it changes"),
        boost::bind(return_hr, S_OK));
    td.add_button(
        translate(
            "I trust this key: &just connect\n"
            "You will be warned about this key again next time you "
            "connect"),
        boost::bind(return_hr, S_FALSE));
    td.add_button(
        translate(
            "&Cancel\n"
            "Choose this option unless you are sure the key is correct"),
        boost::bind(return_hr, E_ABORT), true);
    return td.show();
}

HRESULT on_hostkey_unknown(
    const wstring& host, const wstring& key, const wstring& key_type,
    HWND hwnd)
{
    if (!hwnd)
        BOOST_THROW_EXCEPTION(com_error("User interation forbidden", E_FAIL));

    wstring title = translate("Unknown host-key");

    wstringstream message;
    
    message << wformat(
        translate(
            "The server '%1%' has identified itself with an SSH host-key "
            "whose fingerprint is:")) % host;
    message << wformat(L"\n\n        %1%    %2%\n\n") % key_type % key;
    message << translate(
        "If you are not expecting this key, a third-party may be pretending "
        "to be the computer you're trying to connect to.");

    wstring instruction = translate("Verify unknown SSH host-key");
    task_dialog::task_dialog<HRESULT, best_taskdialog> td(
        hwnd, instruction, message.str(), title,
        winapi::gui::task_dialog::icon_type::information, true,
        boost::bind(return_hr, E_ABORT));
    td.add_button(
        translate(
            "I trust this key: &store and connect\n"
            "You won't have to verify this key again unless it changes"),
        boost::bind(return_hr, S_OK));
    td.add_button(
        translate(
            "I trust this key: &just connect\n"
            "You will be asked to verify the key again next time you "
            "connect"),
        boost::bind(return_hr, S_FALSE));
    td.add_button(
        translate(
            "&Cancel\n"
            "Choose this option unless you are sure the key is correct"),
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
