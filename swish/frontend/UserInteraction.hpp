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

#ifndef SWISH_FRONTEND_USERINTERACTION_HPP
#define SWISH_FRONTEND_USERINTERACTION_HPP
#pragma once

#include "swish/provider/SftpProvider.h" // ISftpConsumer

#include <comet/server.h> // simple_object

namespace swish {
namespace frontend {

class CUserInteraction : public comet::simple_object<ISftpConsumer>
{
public:

    typedef ISftpConsumer interface_is;

    CUserInteraction(HWND hwnd);

    /**
     * User interaction callbacks.
     * @{
     */
    // ISftpConsumer Methods
    IFACEMETHODIMP OnPasswordRequest(
        __in BSTR bstrRequest, __out BSTR *pbstrPassword
    );
    IFACEMETHODIMP OnKeyboardInteractiveRequest(
        __in BSTR bstrName, __in BSTR bstrInstruction,
        __in SAFEARRAY *psaPrompts,
        __in SAFEARRAY *psaShowResponses,
        __deref_out SAFEARRAY **ppsaResponses
    );
    IFACEMETHODIMP OnPrivateKeyFileRequest(
        __out BSTR *pbstrPrivateKeyFile
    );
    IFACEMETHODIMP OnPublicKeyFileRequest(
        __out BSTR *pbstrPublicKeyFile
    );
    IFACEMETHODIMP OnConfirmOverwrite(
        __in BSTR bstrOldFile,
        __in BSTR bstrNewFile
    );
    IFACEMETHODIMP OnHostkeyMismatch(
        __in BSTR bstrHostName,
        __in BSTR bstrHostKey,
        __in BSTR bstrHostKeyType
    );
    IFACEMETHODIMP OnHostkeyUnknown(
        __in BSTR bstrHostName,
        __in BSTR bstrHostKey,
        __in BSTR bstrHostKeyType
    );
    /* @} */

private:
    HWND m_hwnd;      ///< Window to use as parent for user interaction.
};

}} // namespace swish::frontend

#endif
