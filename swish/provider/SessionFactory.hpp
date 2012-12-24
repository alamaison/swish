/**
    @file

    Factory producing connected, authenticated CSession objects.

    @if license

    Copyright (C) 2008, 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#pragma once

#include "Session.hpp"

#include "swish/provider/sftp_provider.hpp" // ISftpConsumer

#include <winerror.h>  // HRESULT

#include <memory>      // auto_ptr
#include <string>

class CSessionFactory
{
public:
    static std::auto_ptr<CSession> CreateSftpSession(
        const wchar_t* pwszHost, unsigned int uPort, const wchar_t* pwszUser,
        __in ISftpConsumer* pConsumer);

private:
    static void _VerifyHostKey(
        PCWSTR pwszHost, CSession& session, __in ISftpConsumer *pConsumer);

    static void _AuthenticateUser(
        PCWSTR pwszUser, CSession& session, 
        __in ISftpConsumer* pConsumer);

    static HRESULT _PasswordAuthentication(
        const std::string& utf8_username, CSession& session, 
        __in ISftpConsumer* pConsumer);

    static HRESULT _KeyboardInteractiveAuthentication(
        const std::string& utf8_username, CSession& session, 
        __in ISftpConsumer* pConsumer);

    static HRESULT _PublicKeyAuthentication(
        const std::string& utf8_username, CSession& session, 
        __in ISftpConsumer* pConsumer);
};
