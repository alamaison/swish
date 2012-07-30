/**
    @file

    Keyboard-interactive authentication via a callback.

    @if license

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.

    @endif
*/

#include "KeyboardInteractive.hpp"

#include "swish/interfaces/SftpProvider.h" // ISftpConsumer

#include <libssh2.h>
#include <libssh2_sftp.h>

using ATL::CComBSTR;
using ATL::CComSafeArray;

using comet::com_error;

CKeyboardInteractive::CKeyboardInteractive(ISftpConsumer *pConsumer) throw() :
    m_spConsumer(pConsumer), m_hr(S_OK)
{}

void CKeyboardInteractive::SetErrorState(HRESULT hr) throw()
{
    m_hr = hr;
}

HRESULT CKeyboardInteractive::GetErrorState() throw()
{
    return m_hr;
}

/**
 * Callback for libssh2_userauth_keyboard_interactive() library function.
 *
 * This static function extracts the 'this' instance to use from the
 * session abstract data which must be set using libssh2_session_abstract()
 * prior to calling.
 */
/* static */ void CKeyboardInteractive::OnKeyboardInteractive(
    const char *name, int name_len,
    const char *instruction, int instruction_len, 
    int num_prompts, const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts, 
    LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
    void **abstract) throw()
{
    if (!num_prompts && !name_len && !instruction_len)
        return;

    // Retrieve pointer to our object instance from callback payload
    ATLASSERT(*abstract);
    CKeyboardInteractive *pThis =
        static_cast<CKeyboardInteractive *>(*abstract);

    try
    {
        // Pack prompt information into SAFEARRAYs
        CComSafeArray<BSTR> saPrompts =
            _PackPromptSafearray(num_prompts, prompts);
        CComSafeArray<VARIANT_BOOL> saShowPrompts =
            _PackEchoSafearray(num_prompts, prompts);

        // Send arrays to the front-end in a keyboard-interactive request
        CComSafeArray<BSTR> saResponses = pThis->_SendRequest(
            CComBSTR(name_len, name), CComBSTR(instruction_len, instruction),
            saPrompts, saShowPrompts);

        // Pack responses into libssh2 data-structure
        _ProcessResponses(saResponses, num_prompts, responses);
    }
    catch (com_error& e)
    {
        pThis->SetErrorState(e.hr());
        return;
    }
    catch (...)
    {
        pThis->SetErrorState(E_UNEXPECTED);
        return;
    }
}

/**
 * Sends arrays to the front-end in a keyboard-interactive request.
 *
 * @returns the responses given to each prompt by the front-end (i.e the user).
 */
CKeyboardInteractive::ResponseArray CKeyboardInteractive::_SendRequest(
    const BSTR bstrName, const BSTR bstrInstruction,
    PromptArray& saPrompts, EchoArray& saShowPrompts) throw (...)
{
    HRESULT hr = E_FAIL;

    SAFEARRAY *psaResponses = NULL;
    hr = m_spConsumer->OnKeyboardInteractiveRequest(
        bstrName, bstrInstruction, saPrompts, saShowPrompts, &psaResponses);
    if (FAILED(hr))
        AtlThrow(hr);
    ATLENSURE(psaResponses);

    return psaResponses;
}

/**
 * Packs prompt text into a SAFEARRAY.
 */
/* static */ CKeyboardInteractive::PromptArray CKeyboardInteractive::_PackPromptSafearray(
    int cPrompts, const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts) throw()
{
    CComSafeArray<BSTR> saPrompts(cPrompts);
    for (int i = 0; i < cPrompts; i++)
    {
        ATLVERIFY(SUCCEEDED( saPrompts.SetAt(
            i, CComBSTR(prompts[i].length, prompts[i].text).Detach(), false) ));
    }

    return saPrompts;
}

/**
 * Packs prompt echo information into a SAFEARRAY.
 */
/* static */ CKeyboardInteractive::EchoArray CKeyboardInteractive::_PackEchoSafearray(
    int cPrompts, const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts) throw()
{
    CComSafeArray<VARIANT_BOOL> saShowPrompts(cPrompts);
    for (int i = 0; i < cPrompts; i++)
    {
        saShowPrompts[i] = (prompts[i].echo) ? VARIANT_TRUE : VARIANT_FALSE;
    }

    return saShowPrompts;
}

/**
 * Packs the SAFEARRAY of responses into the data-structure provided by libssh2.
 */
/* static */ void CKeyboardInteractive::_ProcessResponses(
    ResponseArray& saResponses,
    int cPrompts, LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses) throw()
{
    ATLASSERT( saResponses.GetCount() == (ULONG)cPrompts );
    for (int i = 0; i < cPrompts; i++)
    {
        // Calculate necessary length of UTF-8 response buffer
        int cbLength = ::WideCharToMultiByte(
            CP_UTF8, 0, saResponses[i], ::SysStringLen(saResponses[i]),
            NULL, 0, NULL, NULL);

        // Allocate a sufficiently large buffer using the libssh2 allocator
        // (malloc) and convert string into the buffer
        responses[i].text = static_cast<char *>(malloc(cbLength));
        responses[i].length = cbLength;
        ::WideCharToMultiByte(
            CP_UTF8, 0, saResponses[i], ::SysStringLen(saResponses[i]),
            responses[i].text, cbLength, NULL, NULL);
    }
}
