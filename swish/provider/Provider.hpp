/**
    @file

    libssh2-based SFTP provider component.

    @if licence

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

#include "SessionFactory.hpp"                // CSession
#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer

#include "swish/atl.hpp"                    // Common ATL setup
#include <atlstr.h>                          // CString

#include <boost/shared_ptr.hpp> // shared_ptr

#include <string>

namespace swish {
namespace provider {

class ATL_NO_VTABLE CProvider : public ISftpProvider
{
public:
	typedef ISftpProvider interface_is;

	CProvider();
	~CProvider() throw();

	/** @name ISftpProvider methods */
	// @{
	IFACEMETHODIMP Initialize(
		__in BSTR bstrUser, __in BSTR bstrHost, UINT uPort );
	IFACEMETHODIMP GetListing(
		__in ISftpConsumer *pConsumer,
		__in BSTR bstrDirectory, __out IEnumListing **ppEnum );
	IFACEMETHODIMP GetFile(
		__in ISftpConsumer *pConsumer,
		__in BSTR bstrFilePath, __in BOOL fWriteable,
		__out IStream **ppStream );
	IFACEMETHODIMP Rename(
		__in ISftpConsumer *pConsumer,
		__in BSTR bstrFromPath, __in BSTR bstrToPath,
		__deref_out VARIANT_BOOL *pfWasTargetOverwritten );
	IFACEMETHODIMP Delete(
		__in ISftpConsumer *pConsumer,
		__in BSTR bstrPath );
	IFACEMETHODIMP DeleteDirectory(
		__in ISftpConsumer *pConsumer,
		__in BSTR bstrPath );
	IFACEMETHODIMP CreateNewFile(
		__in ISftpConsumer *pConsumer,
		__in BSTR bstrPath );
	IFACEMETHODIMP CreateNewDirectory(
		__in ISftpConsumer *pConsumer,
		__in BSTR bstrPath );
	// @}

private:
	BOOL m_fInitialized;           ///< Flag if Initialize() has been called
	boost::shared_ptr<CSession> m_session; ///< SSH/SFTP session
	ATL::CString m_strUser;        ///< Holds username for remote connection
	ATL::CString m_strHost;        ///< Hold name of remote host
	UINT m_uPort;                  ///< Holds remote port to connect to
	DWORD m_dwCookie;              ///< Running Object Table registration

	HRESULT _Connect(__in ISftpConsumer *pConsumer);
	void _Disconnect();

	ATL::CString _GetLastErrorMessage();
	ATL::CString _GetSftpErrorMessage( ULONG uError );

	HRESULT _RenameSimple(const std::string& from, const std::string& to);
	HRESULT _RenameRetryWithOverwrite(
		__in ISftpConsumer *pConsumer, __in ULONG uPreviousError,
		const std::string& from, const std::string& to,
		__out ATL::CString& strError);
	HRESULT _RenameAtomicOverwrite(
		const std::string& from, const std::string& to,
		__out ATL::CString& strError);
	HRESULT _RenameNonAtomicOverwrite(
		const std::string& from, const std::string& to,
		ATL::CString& strError);

	HRESULT _Delete(
		__in_z const char *szPath, __out ATL::CString& strError );
	HRESULT _DeleteDirectory(
		__in_z const char *szPath, __out ATL::CString& strError );
	HRESULT _DeleteRecursive(
		__in_z const char *szPath, __out ATL::CString& strError );
};

}} // namespace swish::provider
