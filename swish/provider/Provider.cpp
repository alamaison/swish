/**
    @file

    libssh2-based SFTP provider component.

    @if license

    Copyright (C) 2008, 2009, 2010, 2011, 2012
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

#include "Provider.hpp"

#include "KeyboardInteractive.hpp"
#include "SessionFactory.hpp" // CSession
#include "SftpStream.hpp"
#include "listing/listing.hpp"   // SFTP directory listing helper functions

#include "swish/port_conversion.hpp" // port_to_wstring
#include "swish/remotelimits.h"
#include "swish/utils.hpp" // running_object_table, WideStringToUtf8String
#include "swish/trace.hpp" // trace

#include <comet/bstr.h> // bstr_t
#include <comet/datetime.h> // datetime_t
#include <comet/error.h> // com_error
#include <comet/ptr.h> // com_ptr
#include <comet/enum.h> // stl_enumeration
#include <comet/server.h> // simple_object for STL holder with AddRef lifetime

#include <ssh/sftp.hpp> // directory_iterator

#include <boost/filesystem/path.hpp> // wpath
#include <boost/iterator/filter_iterator.hpp> // make_filter_iterator
#include <boost/make_shared.hpp> // make_shared<provider>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/system/system_error.hpp> // system_error, system_category

#include <exception>
#include <string>
#include <vector> // to hold listing

using swish::utils::com::running_object_table;
using swish::utils::WideStringToUtf8String;
using swish::utils::Utf8StringToWideString;
using swish::tracing::trace;

using comet::bstr_t;
using comet::com_error;
using comet::com_ptr;
using comet::datetime_t;
using comet::stl_enumeration;

using boost::filesystem::wpath;
using boost::make_filter_iterator;
using boost::make_shared;
using boost::system::system_error;
using boost::system::system_category;
using boost::shared_ptr;

using ssh::sftp::attributes;
using ssh::sftp::canonical_path;
using ssh::sftp::directory_iterator;
using ssh::sftp::file_attributes;
using ssh::sftp::sftp_channel;
using ssh::sftp::sftp_file;
using ssh::sftp::unsupported_attribute_error;

using std::exception;
using std::string;
using std::wstring;
using std::vector;

namespace swish {
namespace provider {

class provider
{
public:

	provider(const wstring& user, const wstring& host, int port);
	~provider() throw();

	IEnumListing* get_listing(
		com_ptr<ISftpConsumer> consumer, const wpath& directory);

	IStream* get_file(
		com_ptr<ISftpConsumer> consumer, const wpath& file_path,
		bool writeable);

	VARIANT_BOOL rename(
		com_ptr<ISftpConsumer> consumer, const wpath& from_path,
		const wpath& to_path);

	void delete_file(com_ptr<ISftpConsumer> consumer, const wpath& path);

	void delete_directory(com_ptr<ISftpConsumer> consumer, const wpath& path);

	void create_new_file(com_ptr<ISftpConsumer> consumer, const wpath& path);

	void create_new_directory(
		com_ptr<ISftpConsumer> consumer, const wpath& path);

	BSTR resolve_link(com_ptr<ISftpConsumer> consumer, const wpath& path);

	virtual Listing stat(ISftpConsumer* consumer, BSTR path, BOOL follow_links);

private:
	boost::shared_ptr<CSession> m_session; ///< SSH/SFTP session

	/** @name Fields used for lazy connection. */
	// @{
	wstring m_user;
	wstring m_host;
	UINT m_port;
	// @}

	void _Connect(com_ptr<ISftpConsumer> consumer);
	void _Disconnect();

	wstring _GetLastErrorMessage();
	wstring _GetSftpErrorMessage( ULONG uError );

	HRESULT _RenameSimple(const string& from, const string& to);
	HRESULT _RenameRetryWithOverwrite(
		__in ISftpConsumer *pConsumer, __in ULONG uPreviousError,
		const string& from, const string& to,
		wstring& error_out);
	HRESULT _RenameAtomicOverwrite(
		const string& from, const string& to,
		wstring& error_out);
	HRESULT _RenameNonAtomicOverwrite(
		const string& from, const string& to,
		wstring& error_out);

	HRESULT _Delete(
		__in_z const char *szPath, wstring& error_out);
	HRESULT _DeleteDirectory(
		__in_z const char *szPath, wstring& error_out);
	HRESULT _DeleteRecursive(
		__in_z const char *szPath, wstring& error_out);
};


namespace {

	/**
	 * Create an item moniker for the session with the given parameters.
	 *
	 * e.g. !user@host:port
	 */
	com_ptr<IMoniker> create_item_moniker(
		const wstring& user, const wstring& host, int port)
	{
		wstring moniker_name = 
			user + L'@' + host + L':' + port_to_wstring(port);

		com_ptr<IMoniker> moniker;
		HRESULT hr = ::CreateItemMoniker(
			OLESTR("!"), moniker_name.c_str(), moniker.out());
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(hr));

		return moniker;
	}

	/**
	 * Register a session with the Running Object Table.
	 *
	 *
	 * The connection is registered with a moniker of the form 
	 * !sftp://user@host:port.  By default, the ROT will hold a strong 
	 * reference to this instance preventing it from being released unless 
	 * explicitly revoked.
	 *
	 * @note  The connection doesn't actually have to be connected to the 
	 * remote server to be registered.  This object instance, connection or 
	 * otherwise, is the connection as far as clients are concerned.
	 *
	 * @returns  A cookie identifying the registration.
	 */
	DWORD register_in_rot(
		const com_ptr<ISftpProvider>& provider, const wstring& user,
		const wstring& host, int port,
		DWORD flags=ROTFLAGS_REGISTRATIONKEEPSALIVE)
	{
		com_ptr<IMoniker> moniker = create_item_moniker(user, host, port);
		com_ptr<IRunningObjectTable> rot = running_object_table();
		com_ptr<IUnknown> unknown = provider;

		DWORD cookie = 0;
		HRESULT hr = rot->Register(flags, unknown.in(), moniker.in(), &cookie);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(hr));

		return cookie;
	}

	/**
	 * Remove this connection instance from the Running Object Table.
	 *
	 * The target instance will be registered with a moniker of the form 
	 * !sftp://user@host:port.  The ROT will release its reference to
	 * this instance and, if that was the last one, this object will be freed.
	 */
	void revoke_from_rot(DWORD cookie)
	{
		com_ptr<IRunningObjectTable> rot = running_object_table();

		HRESULT hr = rot->Revoke(cookie);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(hr));
	}
}


CProvider::CProvider() : m_dwCookie(0) {}

/**
 * Remove from ROT.
 */
CProvider::~CProvider() throw()
{
	try
	{
		if (m_dwCookie)
			revoke_from_rot(m_dwCookie);
	}
	catch (const std::exception& e)
	{
		trace("EXCEPTION THROWN IN DESTRUCTOR: %s") % e.what();
	}
}

provider_interface& CProvider::impl() {	return *this; }

void CProvider::initialize(BSTR user, BSTR host, UINT port)
{
	if (::SysStringLen(user) == 0 || ::SysStringLen(host) == 0)
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));
	if (port < MIN_PORT || port > MAX_PORT)
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

	m_provider = make_shared<provider>(user, host, port);
	m_dwCookie = register_in_rot(this, user, host, port);
}

IEnumListing* CProvider::get_listing(ISftpConsumer* consumer, BSTR directory)
{ return m_provider->get_listing(consumer, directory); }

IStream* CProvider::get_file(
	ISftpConsumer* consumer, BSTR file_path, BOOL writeable)
{ return m_provider->get_file(consumer, file_path, writeable == TRUE); }

VARIANT_BOOL CProvider::rename(
	ISftpConsumer* consumer, BSTR from_path, BSTR to_path)
{ return m_provider->rename(consumer, from_path, to_path); }

void CProvider::delete_file(ISftpConsumer* consumer, BSTR path)
{ m_provider->delete_file(consumer, path); }

void CProvider::delete_directory(ISftpConsumer* consumer, BSTR path)
{ m_provider->delete_directory(consumer, path); }

void CProvider::create_new_file(ISftpConsumer* consumer, BSTR path)
{ m_provider->create_new_file(consumer, path); }

void CProvider::create_new_directory(ISftpConsumer* consumer, BSTR path)
{ m_provider->create_new_directory(consumer, path); }

BSTR CProvider::resolve_link(ISftpConsumer* consumer, BSTR path)
{ return m_provider->resolve_link(consumer, path); }

Listing CProvider::stat(ISftpConsumer* consumer, BSTR path, BOOL follow_links)
{
	return m_provider->stat(consumer, path, follow_links);
}

/**
 * Create libssh2-based data provider.
 */
provider::provider(const wstring& user, const wstring& host, int port)
	: m_user(user), m_host(host), m_port(port)
{
	assert(!m_user.empty());
	assert(!m_host.empty());
	assert(m_port >= MIN_PORT);
	assert(m_port <= MAX_PORT);
}

/**
 * Free libssh2 and remove from ROT.
 */
provider::~provider() throw()
{
	try
	{
		_Disconnect(); // Destroy session before shutting down Winsock
	}
	catch (const std::exception& e)
	{
		trace("EXCEPTION THROWN IN DESTRUCTOR: %s") % e.what();
	}
}

/**
 * Sets up the SFTP session, prompting user for input if necessary.
 *
 * The remote server must have its identity verified which may require user
 * confirmation and the user must authenticate with the remote server
 * which might be done silently (i.e. with a public-key) or may require
 * user input.
 *
 * If the session has already been created, this function does nothing.
 */
void provider::_Connect(com_ptr<ISftpConsumer> consumer)
{
	if (!m_session || m_session->IsDead())
	{
		m_session = CSessionFactory::CreateSftpSession(
			m_host.c_str(), m_port, m_user.c_str(), consumer.get());
	}
}

void provider::_Disconnect()
{
	m_session.reset();
}

namespace {

	template<typename Collection>
	class stl_enum_holder : public comet::simple_object<> //<IUnknown>
	{
	public:
		stl_enum_holder(shared_ptr<Collection> collection)
			: m_collection(collection) {}
		shared_ptr<Collection> m_collection;
	};

	Listing listing_from_sftp_file(const sftp_file& file)
	{
		return listing::fill_listing_entry(
			file.name(), file.long_entry(), file.raw_attributes());
	}

	bool not_special_file(const sftp_file& file)
	{
		return file.name() != "." && file.name() != "..";
	}

}

/**
* Retrieves a file listing, @c ls, of a given directory.
*
* The listing is returned as an IEnumListing of Listing objects.
*
* @param consumer   UI callback.  
* @param directory  Absolute path of the directory to list.
*
* @see Listing for details of what file information is retrieved.
*/
IEnumListing* provider::get_listing(
	com_ptr<ISftpConsumer> consumer, const wpath& directory)
{
	if (directory.empty())
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

	_Connect(consumer);

	sftp_channel channel(m_session->get(), m_session->sftp());

	string path = WideStringToUtf8String(directory.string());

	shared_ptr<vector<Listing> > files(make_shared<vector<Listing> >());
	transform(
		make_filter_iterator(
			not_special_file, directory_iterator(channel, path)),
		make_filter_iterator(
			not_special_file, directory_iterator()),
		back_inserter(*files),
		listing_from_sftp_file);

	// Put the list of Listings into an AddReffed 'holder' so the
	// IEnumListing implementation can control its lifetime
	com_ptr<stl_enum_holder<vector<Listing> > > holder = 
		new stl_enum_holder<vector<Listing> >(files);
	IUnknown* unknown = holder->get_unknown();

	return stl_enumeration<IEnumListing>::create(
		*holder->m_collection, unknown).detach();
}

/**
 * @todo  Add flag to interface to allow choice of read or write.
 */
IStream* provider::get_file(
	com_ptr<ISftpConsumer> consumer, const wpath& file_path, bool writeable)
{
	if (file_path.empty())
		BOOST_THROW_EXCEPTION(com_error("File cannot be empty", E_INVALIDARG));

	_Connect(consumer);

	CSftpStream::OpenFlags flags = CSftpStream::read;
	if (writeable)
		flags |= CSftpStream::write | CSftpStream::create;

	string path = WideStringToUtf8String(file_path.string());
	com_ptr<IStream> stream = new CSftpStream(m_session, path, flags);
	return stream.detach();
}

/**
 * Renames a file or directory.
 *
 * The source and target file or directory must be specified using absolute 
 * paths for the remote filesystem.  The results of passing relative paths are 
 * not guaranteed (though, libssh2 seems to default to operating in the home 
 * directory) and may be dangerous.
 *
 * If a file or folder already exists at the target path, @a to_path, 
 * we inform the front-end consumer through a call to OnConfirmOverwrite.
 * If confirmation is given, we attempt to overwrite the
 * obstruction with the source path, @a from_path, and if successful we
 * return @c VARIANT_TRUE.  This can be used by the caller to decide whether
 * or not to update a directory view.
 *
 * @remarks
 * Due to the limitations of SFTP versions 4 and below, most servers will not
 * allow atomic overwrite.  We attempt to do this non-atomically by:
 * -# appending @c ".swish_renaming_temp" to the obstructing target's filename
 * -# renaming the source file to the old target name
 * -# deleting the renamed target
 * If step 2 fails, we try to rename the temporary file back to its old name.
 * It is possible that this last step may fail, in which case the temporary file
 * would remain in place.  It could be recovered by manually renaming it back.
 *
 * @warning
 * If either of the paths are not absolute, this function may cause files
 * in whichever directory libssh2 considers 'current' to be renamed or deleted 
 * if they happen to have matching filenames.
 *
 * @param consumer   UI callback.  
 * @param from_path  Absolute path of the file or directory to be renamed.
 * @param to_path    Absolute path that @a from_path should be renamed to.
 *
 * @returns  Whether or not we needed to overwrite an existing file or
 *           directory at the target path. 
 */
VARIANT_BOOL provider::rename(
	com_ptr<ISftpConsumer> consumer, const wpath& from_path,
	const wpath& to_path)
{
	if (from_path.empty())
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));
	if (to_path.empty())
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

	// NOP if filenames are equal
	if (from_path == to_path)
		return VARIANT_FALSE;

	_Connect(consumer);

	// Attempt to rename old path to new path
	string from = WideStringToUtf8String(from_path.string());
	string to = WideStringToUtf8String(to_path.string());
	HRESULT hr = _RenameSimple(from, to);
	if (SUCCEEDED(hr)) // Rename was successful without overwrite
		return VARIANT_FALSE;

	// Rename failed - this is OK, it might be an overwrite - check
	bstr_t message;
	int nErr; PSTR pszErr; int cchErr;
	nErr = libssh2_session_last_error(*m_session, &pszErr, &cchErr, false);
	if (nErr == LIBSSH2_ERROR_SFTP_PROTOCOL)
	{
		wstring error_out;
		hr = _RenameRetryWithOverwrite(
			consumer.get(), libssh2_sftp_last_error(*m_session), from, to,
			error_out);
		if (SUCCEEDED(hr))
			return VARIANT_TRUE;
		else if (hr == E_ABORT) // User denied overwrite
			BOOST_THROW_EXCEPTION(com_error(hr));
		else
			message = error_out;
	}
	else // A non-SFTP error occurred
		message = pszErr;

	// Report remaining errors
	BOOST_THROW_EXCEPTION(com_error(message, E_FAIL));
}

/**
 * Renames a file or directory but prevents overwriting any existing item.
 *
 * @returns Success or failure of the operation.
 *    @retval S_OK   if the file or directory was successfully renamed
 *    @retval E_FAIL if there already is a file or directory at the target path
 *
 * @param from  Absolute path of the file or directory to be renamed.
 * @param to    Absolute path to rename @a from to.
 */
HRESULT provider::_RenameSimple(const string& from, const string& to)
{
	int rc = libssh2_sftp_rename_ex(
		*m_session, from.data(), from.size(), to.data(), to.size(),
		LIBSSH2_SFTP_RENAME_ATOMIC | LIBSSH2_SFTP_RENAME_NATIVE);

	return (!rc) ? S_OK : E_FAIL;
}

/**
 * Retry renaming file or directory if possible, after seeking permission to 
 * overwrite the obstruction at the target.
 *
 * If this fails the file or directory really can't be renamed and the error
 * message from libssh2 is returned in @a error_out.
 *
 * @param [in]  uPreviousError Error code of the previous rename attempt in
 *                             order to determine if an overwrite has any chance
 *                             of being successful.
 *
 * @param [in]  from           Absolute path of the file or directory to 
 *                             be renamed.
 *
 * @param [in]  to             Absolute path to rename @a from to.
 *
 * @param [out] error_out       Error message if the operation fails.
 *
 * @bug  The strings aren't converted from UTF-8 to UTF-16 before displaying
 *       to the user.  Any unicode filenames will produce gibberish in the
 *       confirmation dialogues.
 */
HRESULT provider::_RenameRetryWithOverwrite(
	ISftpConsumer *pConsumer,
	ULONG uPreviousError, const string& from, const string& to, 
	wstring& error_out)
{
	HRESULT hr;

	if (uPreviousError == LIBSSH2_FX_FILE_ALREADY_EXISTS)
	{
		hr = pConsumer->OnConfirmOverwrite(bstr_t(from).in(), bstr_t(to).in());
		if (FAILED(hr))
			return E_ABORT; // User disallowed overwrite

		// Attempt rename again this time allowing overwrite
		return _RenameAtomicOverwrite(from, to, error_out);
	}
	else if (uPreviousError == LIBSSH2_FX_FAILURE)
	{
		// The failure is an unspecified one. This isn't the end of the world. 
		// SFTP servers < v5 (i.e. most of them) return this error code if the
		// file already exists as they don't explicitly support overwriting.
		// We need to stat() the file to find out if this is the case and if 
		// the user confirms the overwrite we will have to explicitly delete
		// the target file first (via a temporary) and then repeat the rename.
		//
		// NOTE: this is not a perfect solution due to the possibility
		// for race conditions.
		LIBSSH2_SFTP_ATTRIBUTES attrsTarget;
		::ZeroMemory(&attrsTarget, sizeof attrsTarget);
		if (!libssh2_sftp_stat(*m_session, to.c_str(), &attrsTarget))
		{
			// File already exists
			hr = pConsumer->OnConfirmOverwrite(
				bstr_t(from).in(), bstr_t(to).in());
			if (FAILED(hr))
				return E_ABORT; // User disallowed overwrite

			return _RenameNonAtomicOverwrite(from, to, error_out);
		}
	}
		
	// File does not already exist, another error caused rename failure
	error_out = _GetSftpErrorMessage(uPreviousError);
	return E_FAIL;
}

/**
 * Rename file or directory and atomically overwrite any obstruction.
 *
 * @remarks
 * This will only work on a server supporting SFTP version 5 or above.
 *
 * @param [in]  from      Absolute path of the file or directory to be renamed.
 * @param [in]  to        Absolute path to rename @a from to.
 * @param [out] error_out  Error message if the operation fails.
 */
HRESULT provider::_RenameAtomicOverwrite(
	const string& from, const string& to, wstring& error_out)
{
	int rc = libssh2_sftp_rename_ex(
		*m_session, from.data(), from.size(), to.data(), to.size(), 
		LIBSSH2_SFTP_RENAME_OVERWRITE | LIBSSH2_SFTP_RENAME_ATOMIC | 
		LIBSSH2_SFTP_RENAME_NATIVE
	);

	if (!rc)
		return S_OK;
	else
	{
		char *pszMessage; int cchMessage;
		libssh2_session_last_error(
			*m_session, &pszMessage, &cchMessage, false);
		error_out = Utf8StringToWideString(pszMessage);
		return E_FAIL;
	}
}


/**
 * Rename file or directory and overwrite any obstruction non-atomically.
 *
 * This involves renaming the obstruction at the target to a temporary file, 
 * renaming the source file to the target and then deleting the renamed 
 * obstruction.  As this is not an atomic operation it is possible to fail 
 * between any of these stages and is not a prefect solution.  It may, for 
 * instance, leave the temporary file behind.
 *
 * @param [in]  from      Absolute path of the file or directory to be renamed.
 * @param [in]  to        Absolute path to rename @a from to.
 * @param [out] error_out  Error message if the operation fails.
 */
HRESULT provider::_RenameNonAtomicOverwrite(
	const string& from, const string& to, wstring& error_out)
{
	// First, rename existing file to temporary
	string temporary(to);
	temporary += ".swish_rename_temp";
	int rc = libssh2_sftp_rename(*m_session, to.c_str(), temporary.c_str());
	if (!rc)
	{
		// Rename our subject
		rc = libssh2_sftp_rename(*m_session, from.c_str(), to.c_str());
		if (!rc)
		{
			// Delete temporary
			wstring error_out; // unused
			HRESULT hr = _DeleteRecursive(temporary.c_str(), error_out);
			assert(SUCCEEDED(hr));
			(void)hr; // The rename succeeded even if this fails
			return S_OK;
		}

		// Rename failed, rename our temporary back to its old name
		rc = libssh2_sftp_rename(*m_session, from.c_str(), to.c_str());
		assert(!rc);

		error_out = _T("Cannot overwrite \"");
		error_out += Utf8StringToWideString(from + "\" with \"" + to);
		error_out += _T("\": Please specify a different name or delete \"");
		error_out += Utf8StringToWideString(to + "\" first.");
	}

	return E_FAIL;
}

void provider::delete_file(com_ptr<ISftpConsumer> consumer, const wpath& path)
{
	if (path.empty())
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

	_Connect(consumer);

	// Delete file
	wstring error_out;
	string utf8_path = WideStringToUtf8String(path.string());
	HRESULT hr = _Delete(utf8_path.c_str(), error_out);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_error(error_out, E_FAIL));
}

HRESULT provider::_Delete( const char *szPath, wstring& error_out )
{
	if (libssh2_sftp_unlink(*m_session, szPath) == 0)
		return S_OK;

	// Delete failed
	error_out = _GetLastErrorMessage();
	return E_FAIL;
}

void provider::delete_directory(
	com_ptr<ISftpConsumer> consumer, const wpath& path)
{
	if (path.empty())
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

	_Connect(consumer);

	// Delete directory recursively
	wstring error_out;
	string utf8_path = WideStringToUtf8String(path.string());
	HRESULT hr = _DeleteDirectory(utf8_path.c_str(), error_out);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_error(error_out, E_FAIL));
}

HRESULT provider::_DeleteDirectory(const char* szPath, wstring& error_out)
{
	HRESULT hr;

	// Open directory
	LIBSSH2_SFTP_HANDLE *pSftpHandle = libssh2_sftp_opendir(
		*m_session, szPath
	);
	if (!pSftpHandle)
	{
		error_out = _GetLastErrorMessage();
		return E_FAIL;
	}

	// Delete content of directory
	do {
		// Read filename and attributes. Returns length of filename.
		char szFilename[MAX_FILENAME_LENZ];
		LIBSSH2_SFTP_ATTRIBUTES attrs;
		::ZeroMemory(&attrs, sizeof(attrs));
		int rc = libssh2_sftp_readdir(
			pSftpHandle, szFilename, sizeof(szFilename), &attrs
		);
		if (rc <= 0)
			break;

		assert(szFilename[0]); // TODO: can files have no filename?
		if (szFilename[0] == '.' && !szFilename[1])
			continue; // Skip .
		if (szFilename[0] == '.' && szFilename[1] == '.' && !szFilename[2])
			continue; // Skip ..

		string strSubPath(szPath);
		strSubPath += "/";
		strSubPath += szFilename;
		hr = _DeleteRecursive(strSubPath.c_str(), error_out);
		if (FAILED(hr))
		{
			rc = libssh2_sftp_close_handle(pSftpHandle);
			assert(rc == 0);
			return hr;
		}
	} while (true);
	int rc = libssh2_sftp_close_handle(pSftpHandle);
	assert(rc == 0); (void)rc;

	// Delete directory itself
	if (libssh2_sftp_rmdir(*m_session, szPath) == 0)
		return S_OK;

	// Delete failed
	error_out = _GetLastErrorMessage();
	return E_FAIL;
}

HRESULT provider::_DeleteRecursive(
	const char *szPath, wstring& error_out)
{
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	::ZeroMemory(&attrs, sizeof attrs);
	if (libssh2_sftp_lstat(*m_session, szPath, &attrs) != 0)
	{
		error_out = _GetLastErrorMessage();
		return E_FAIL;
	}

	assert( // Permissions field is valid
		attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS);
	if (attrs.permissions & LIBSSH2_SFTP_S_IFDIR)
		return _DeleteDirectory(szPath, error_out);
	else
		return _Delete(szPath, error_out);
}

void provider::create_new_file(
	com_ptr<ISftpConsumer> consumer, const wpath& path)
{
	if (path.empty())
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

	_Connect(consumer);

	string utf8_path = WideStringToUtf8String(path.string());
	LIBSSH2_SFTP_HANDLE *pHandle = libssh2_sftp_open(
		*m_session, utf8_path.c_str(), LIBSSH2_FXF_CREAT, 0644);
	if (pHandle == NULL)
		BOOST_THROW_EXCEPTION(com_error(_GetLastErrorMessage(), E_FAIL));

	int rc = libssh2_sftp_close_handle(pHandle);
	assert(rc == 0); (void)rc;
}

void provider::create_new_directory(
	com_ptr<ISftpConsumer> consumer, const wpath& path)
{
	if (path.empty())
		BOOST_THROW_EXCEPTION(
			com_error(
				"Cannot create a directory without a name", E_INVALIDARG));

	_Connect(consumer);

	string utf8_path = WideStringToUtf8String(path.string());
	if (libssh2_sftp_mkdir(*m_session, utf8_path.c_str(), 0755) != 0)
		BOOST_THROW_EXCEPTION(com_error(_GetLastErrorMessage(), E_FAIL));
}

BSTR provider::resolve_link(com_ptr<ISftpConsumer> consumer, const wpath& path)
{
	_Connect(consumer);

	string utf8_path = WideStringToUtf8String(path.string());

	sftp_channel channel(m_session->get(), m_session->sftp());
	bstr_t target =
		Utf8StringToWideString(canonical_path(channel, utf8_path).string());

	return target.detach();
}

/**
 * Get the details of a file by path.
 *
 * The Listing returned by this function doesn't include a long entry or
 * owner and group names as string (these being derived from the long entry).
 */
Listing provider::stat(ISftpConsumer* consumer, BSTR path, BOOL follow_links)
{
	_Connect(consumer);
	
	string utf8_path = WideStringToUtf8String(bstr_t(path).w_str());
	sftp_channel channel(m_session->get(), m_session->sftp());

	file_attributes attr = attributes(
		channel, utf8_path, follow_links != FALSE);

	Listing lt = Listing();

	// Permissions
	try
	{
		lt.uPermissions = attr.permissions();
		lt.fIsLink = attr.type() == file_attributes::symbolic_link;
		lt.fIsDirectory = attr.type() == file_attributes::directory;
	} catch (const unsupported_attribute_error&) {}

	// User & Group
	try
	{
		lt.uUid = attr.uid();
		lt.uGid = attr.gid();
	} catch (const unsupported_attribute_error&) {}

	// Size of file
	try
	{
		lt.uSize = attr.size();
	} catch (const unsupported_attribute_error&) {}

	// Access & Modification time
	try
	{
		datetime_t modified(static_cast<time_t>(attr.mtime()));
		datetime_t accessed(static_cast<time_t>(attr.atime()));
		lt.dateModified = modified.detach();
		lt.dateAccessed = accessed.detach();
	} catch (const unsupported_attribute_error&) {}

	lt.bstrFilename = bstr_t(wpath(bstr_t(path).w_str()).filename()).detach();

	return lt;
}

/**
 * Retrieves a string description of the last error reported by libssh2.
 *
 * In the case that the last SSH error is an SFTP error it returns the SFTP
 * error message in preference.
 */
wstring provider::_GetLastErrorMessage()
{
	int nErr; PSTR pszErr; int cchErr;

	nErr = libssh2_session_last_error(*m_session, &pszErr, &cchErr, false);
	if (nErr == LIBSSH2_ERROR_SFTP_PROTOCOL)
	{
		ULONG uErr = libssh2_sftp_last_error(*m_session);
		return _GetSftpErrorMessage(uErr);
	}
	else // A non-SFTP error occurred
		return Utf8StringToWideString(pszErr);
}

/**
 * Maps between libssh2 SFTP error codes and an appropriate error string.
 *
 * @param uError  SFTP error code as returned by libssh2_sftp_last_error().
 */
wstring provider::_GetSftpErrorMessage(ULONG uError)
{
	switch (uError)
	{
	case LIBSSH2_FX_OK:
		return _T("Successful");
	case LIBSSH2_FX_EOF:
		return _T("File ended unexpectedly");
	case LIBSSH2_FX_NO_SUCH_FILE:
		return _T("Required file or folder does not exist");
	case LIBSSH2_FX_PERMISSION_DENIED:
		return _T("Permission denied");
	case LIBSSH2_FX_FAILURE:
		return _T("Unknown failure");
	case LIBSSH2_FX_BAD_MESSAGE:
		return _T("Server returned an invalid message");
	case LIBSSH2_FX_NO_CONNECTION:
		return _T("No connection");
	case LIBSSH2_FX_CONNECTION_LOST:
		return _T("Connection lost");
	case LIBSSH2_FX_OP_UNSUPPORTED:
		return _T("Server does not support this operation");
	case LIBSSH2_FX_INVALID_HANDLE:
		return _T("Invalid handle");
	case LIBSSH2_FX_NO_SUCH_PATH:
		return _T("The path does not exist");
	case LIBSSH2_FX_FILE_ALREADY_EXISTS:
		return _T("A file or folder of that name already exists");
	case LIBSSH2_FX_WRITE_PROTECT:
		return _T("This file or folder has been write-protected");
	case LIBSSH2_FX_NO_MEDIA:
		return _T("No media was found");
	case LIBSSH2_FX_NO_SPACE_ON_FILESYSTEM:
		return _T("There is no space left on the server's filesystem");
	case LIBSSH2_FX_QUOTA_EXCEEDED:
		return _T("You have exceeded your disk quota on the server");
	case LIBSSH2_FX_UNKNOWN_PRINCIPLE:
		return _T("Unknown principle");
	case LIBSSH2_FX_LOCK_CONFlICT:
		return _T("Lock conflict");
	case LIBSSH2_FX_DIR_NOT_EMPTY:
		return _T("The folder is not empty");
	case LIBSSH2_FX_NOT_A_DIRECTORY:
		return _T("This file is not a folder");
	case LIBSSH2_FX_INVALID_FILENAME:
		return _T("The filename is not valid on the server's filesystem");
	case LIBSSH2_FX_LINK_LOOP:
		return _T("Operation would cause a link loop which is not permitted");
	default:
		return _T("Unexpected error code returned by server");
	}
}

}} // namespace swish::provider
