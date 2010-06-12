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
#include "swish/provider/provider_error_adapter.hpp" // provider_error_adapter

#include "swish/atl.hpp"                    // Common ATL setup
#include <atlstr.h>                          // CString

#include <boost/filesystem/path.hpp> // wpath
#include <boost/shared_ptr.hpp> // shared_ptr

#include <string>

namespace swish {
namespace provider {

class provider
{
public:

	provider(const std::wstring& user, const std::wstring host, int port);
	~provider() throw();

	IEnumListing* get_listing(
		comet::com_ptr<ISftpConsumer> consumer,
		const boost::filesystem::wpath& directory);

	IStream* get_file(
		comet::com_ptr<ISftpConsumer> consumer,
		const boost::filesystem::wpath& file_path, bool writeable);

	VARIANT_BOOL rename(
		comet::com_ptr<ISftpConsumer> consumer,
		const boost::filesystem::wpath& from_path,
		const boost::filesystem::wpath& to_path);

	void delete_file(
		comet::com_ptr<ISftpConsumer> consumer,
		const boost::filesystem::wpath& path);

	void delete_directory(
		comet::com_ptr<ISftpConsumer> consumer,
		const boost::filesystem::wpath& path);

	void create_new_file(
		comet::com_ptr<ISftpConsumer> consumer,
		const boost::filesystem::wpath& path);

	void create_new_directory(
		comet::com_ptr<ISftpConsumer> consumer,
		const boost::filesystem::wpath& path);

private:
	boost::shared_ptr<CSession> m_session; ///< SSH/SFTP session

	/** @name Fields used for lazy connection. */
	// @{
	std::wstring m_user;
	std::wstring m_host;
	UINT m_port;
	// @}

	void _Connect(comet::com_ptr<ISftpConsumer> consumer);
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


class CProvider :
	public provider_interface, public provider_error_adapter
{
public:

	CProvider();
	virtual ~CProvider() throw();

	/** @name ISftpProvider implementation via provider_error_adapter */
	// @{
	virtual void initialize(BSTR user, BSTR host, UINT port);

	virtual IEnumListing* get_listing(
		ISftpConsumer* consumer, BSTR directory);

	virtual IStream* get_file(
		ISftpConsumer* consumer, BSTR file_path, BOOL writeable);

	virtual VARIANT_BOOL rename(
		ISftpConsumer* consumer, BSTR from_path, BSTR to_path);

	virtual void delete_file(ISftpConsumer* consumer, BSTR path);

	virtual void delete_directory(ISftpConsumer* consumer, BSTR path);

	virtual void create_new_file(ISftpConsumer* consumer, BSTR path);

	virtual void create_new_directory(ISftpConsumer* consumer, BSTR path);
	// @}

	virtual provider_interface& impl();

private:
	boost::shared_ptr<provider> m_provider;
	DWORD m_dwCookie;  ///< Running Object Table registration
};

}} // namespace swish::provider
