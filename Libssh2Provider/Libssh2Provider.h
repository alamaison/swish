/*  Declaration of the libssh2-based SFTP provider component.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
*/

#ifndef LIBSSH2PROVIDER_H
#define LIBSSH2PROVIDER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma once
#include "stdafx.h"
#include "resource.h"       // main symbols

#include <list>
using std::list;

// CLibssh2Provider
[
	coclass,
	default(ISftpProvider),
	threading(apartment),
	vi_progid("Libssh2Provider.Libssh2Provider"),
	progid("Libssh2Provider.Libssh2Provider.1"),
	version(1.0),
	uuid("b816a847-5022-11dc-9153-0090f5284f85"),
	helpstring("Libssh2Provider Class")
]
class ATL_NO_VTABLE CLibssh2Provider :
	public ISftpProvider
{
public:

	/** @name ATL Constructors */
	// @{
	CLibssh2Provider();
	DECLARE_PROTECT_FINAL_CONSTRUCT();
	HRESULT FinalConstruct();
	void FinalRelease();
	// @}

	/** @name ISftpProvider methods */
	// @{
	IFACEMETHODIMP Initialize(
		__in ISftpConsumer *pConsumer,
		__in BSTR bstrUser, __in BSTR bstrHost, UINT uPort );
	IFACEMETHODIMP GetListing(
		__in BSTR bstrDirectory, __out IEnumListing **ppEnum );
	IFACEMETHODIMP Rename(
		__in BSTR bstrFromFilename, __in BSTR bstrToFilename );
	// @}

private:
	ISftpConsumer *m_pConsumer;    ///< Callback to consuming object
	BOOL m_fInitialized;           ///< Flag if Initialize() has been called
	LIBSSH2_SESSION *m_pSession;   ///< SSH session
	LIBSSH2_SFTP *m_pSftpSession;  ///< SFTP subsystem session
	SOCKET m_socket;               ///< TCP/IP socket to the remote host
	list<Listing> m_lstFiles;
	CString m_strUser;             ///< Holds username for remote connection
	CString m_strHost;             ///< Hold name of remote host
	UINT m_uPort;                  ///< Holds remote port to connect to

	HRESULT _Connect();
	HRESULT _Disconnect();
	HRESULT _OpenSocketToHost();
	HRESULT _VerifyHostKey();
	HRESULT _AuthenticateUser();
	HRESULT _PasswordAuthentication( PCSTR szUsername );
	HRESULT _KeyboardInteractiveAuthentication( PCSTR szUsername );
	HRESULT _PublicKeyAuthentication( PCSTR szUsername );
	Listing _FillListingEntry(
		PCSTR pszFilename, LIBSSH2_SFTP_ATTRIBUTES& attrs );
	CString _GetSftpErrorMessage(ULONG uError);
};

#endif // LIBSSH2PROVIDER_H