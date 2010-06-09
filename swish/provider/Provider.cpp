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

#include "Provider.hpp"

#include "KeyboardInteractive.hpp"
#include "SftpStream.hpp"
#include "listing/listing.hpp"   // SFTP directory listing helper functions

#include "swish/remotelimits.h"
#include "swish/catch_com.hpp"   // COM exception handler
#include "swish/exception.hpp"   // com_exception
#include "swish/utils.hpp" // running_object_table, WideStringToUtf8String
#include "swish/trace.hpp" // trace

#include <comet/bstr.h> // bstr_t
#include <comet/ptr.h> // com_ptr
#include <comet/enum.h> // stl_enumeration
#include <comet/server.h> // simple_object for STL holder with AddRef lifetime

#include <boost/lexical_cast.hpp> // lexical_cast
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/system/system_error.hpp> // system_error, system_category

#include <string>
#include <vector> // to hold listing

using namespace swish::provider;
using swish::exception::com_exception;
using swish::utils::com::running_object_table;
using swish::utils::WideStringToUtf8String;
using swish::tracing::trace;

using comet::bstr_t;
using comet::com_ptr;
using comet::stl_enumeration;

using boost::lexical_cast;
using boost::system::system_error;
using boost::system::system_category;
using boost::shared_ptr;

using ATL::CW2A;
using ATL::CComBSTR;
using ATL::CString;

using std::string;
using std::wstring;
using std::vector;

#pragma warning (push)
#pragma warning (disable: 4267) // ssize_t to unsigned int

/**
 * Create libssh2-based data provider component instance.
 *
 * @warning
 *   The Initialize() method must be called before the other methods
 *   of the object can be used.
 */
CProvider::CProvider() :
	m_fInitialized(false),
	m_dwCookie(0)
{
}

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
			user + L'@' + host + L':' + lexical_cast<wstring>(port);

		com_ptr<IMoniker> moniker;
		HRESULT hr = ::CreateItemMoniker(
			OLESTR("!"), moniker_name.c_str(), moniker.out());
		assert(SUCCEEDED(hr));
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

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
			BOOST_THROW_EXCEPTION(com_exception(hr));

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
			BOOST_THROW_EXCEPTION(com_exception(hr));
	}
}

/**
 * Free libssh2 and remove from ROT.
 */
CProvider::~CProvider() throw()
{
	try
	{
		_Disconnect(); // Destroy session before shutting down Winsock

		if (m_dwCookie)
			revoke_from_rot(m_dwCookie);
	}
	catch (const std::exception& e)
	{
		trace("EXCEPTION THROWN IN DESTRUCTOR: %s") % e.what();
	}
}

/**
 * Perform initial setup of libssh2 data provider.
 *
 * This function must be called before any other and is used to set the
 * user name, host and port with which to perform the SSH connection.  None
 * of these parameters is optional.
 *
 * @pre The port must be between 0 and 65535 (@ref MAX_PORT) inclusive.
 * @pre The user name (@a bstrUser) and the host name (@a bstrHost) must not
 *      be empty strings.
 *
 * @param bstrUser The user name of the SSH account.
 * @param bstrHost The name of the machine to connect to.
 * @param uPort    The TCP/IP port on which the SSH server is running.
 *
 * @returns Success or failure of the operation.
 *   @retval S_OK if successful
 *   @retval E_INVALIDARG if either string parameter was empty or port 
 *           is invalid
 *   @retval E_POINTER is pConsumer is invalid
 *   @retval E_FAIL if other error encountered
 */
STDMETHODIMP CProvider::Initialize(BSTR bstrUser, BSTR bstrHost, UINT uPort )
{
	if (::SysStringLen(bstrUser) == 0 || ::SysStringLen(bstrHost) == 0)
		return E_INVALIDARG;
	if (uPort < MIN_PORT || uPort > MAX_PORT)
		return E_INVALIDARG;

	m_strUser = bstrUser;
	m_strHost = bstrHost;
	m_uPort = uPort;

	ATLASSUME( !m_strUser.IsEmpty() );
	ATLASSUME( !m_strHost.IsEmpty() );
	ATLASSUME( m_uPort >= MIN_PORT );
	ATLASSUME( m_uPort <= MAX_PORT );

	try
	{
		m_dwCookie = register_in_rot(this, bstrUser, bstrHost, uPort);
	}
	catchCom()

	m_fInitialized = true;
	return S_OK;
}

/**
 * Sets up the SFTP session, prompting user for input if neccessary.
 *
 * The remote server must have its identity verified which may require user
 * confirmation and the user must authenticate with the remote server
 * which might be done silently (i.e. with a public-key) or may require
 * user input.
 *
 * If the session has already been created, this function does nothing.
 *
 * @returns S_OK if session successfully created or an error otherwise.
 */
HRESULT CProvider::_Connect(ISftpConsumer *pConsumer)
{
	try
	{
		try
		{
			if (!m_session.get())
			{
				m_session = CSessionFactory::CreateSftpSession(
					m_strHost, m_uPort, m_strUser, pConsumer);
			}
		}
		catch (const std::exception& e)
		{
			bstr_t message("Could not connect to server:\n\n");
			message += e.what();
			pConsumer->OnReportError(message.get_raw());
			throw;
		}

	}
	catchCom()

	return S_OK;
}

void CProvider::_Disconnect()
{
	m_session.reset();
}


template<> struct comet::comtype<IEnumListing>
{
	static const IID& uuid() throw() { return IID_IEnumListing; }
	typedef IUnknown base;
};

template<> struct comet::enumerated_type_of<IEnumListing> { typedef Listing is; };

/**
 * Copy-policy for use by enumerators of Listing items.
 */
template<> struct comet::impl::type_policy<Listing>
{
	template<typename S>
	static void init(Listing& t, const S& s) 
	{
		::ZeroMemory(&t, sizeof(t));

		t.bstrFilename = SysAllocStringLen(
			s.bstrFilename, ::SysStringLen(s.bstrFilename));
		t.uPermissions = s.uPermissions;
		t.bstrOwner = SysAllocStringLen(
			s.bstrOwner, ::SysStringLen(s.bstrOwner));
		t.bstrGroup = SysAllocStringLen(
			s.bstrGroup, ::SysStringLen(s.bstrGroup));
		t.uUid = s.uUid;
		t.uGid = s.uGid;
		t.uSize = s.uSize;
		t.cHardLinks = s.cHardLinks;
		t.dateModified = s.dateModified;
		t.dateAccessed = s.dateAccessed;
	}

	static void clear(Listing& t)
	{
		::SysFreeString(t.bstrFilename);
		::SysFreeString(t.bstrOwner);
		::SysFreeString(t.bstrGroup);
		::ZeroMemory(&t, sizeof(t));
	}	
};

namespace {

template<typename Collection>
class stl_enum_holder : public comet::simple_object<> //<IUnknown>
{
public:
	stl_enum_holder(shared_ptr<Collection> collection)
		: m_collection(collection) {}
	shared_ptr<Collection> m_collection;
};

}

/**
* Retrieves a file listing, @c ls, of a given directory.
*
* The listing is returned as an IEnumListing of Listing objects.
*
* @pre The Initialize() method must have been called first
*
* @param [in]  bstrDirectory Absolute path of the directory to list.
* @param [out] ppEnum        Pointer to the IEnumListing interface pointer.
*
* @returns  Success or failure of the operation.
*   @retval S_OK and IEnumListing pointer if successful
*   @retval E_UNEXPECTED if Initialize() was not previously called
*   @retval E_FAIL if other error occurs
*
* @todo Handle the case where the directory does not exist
* @todo Return a collection rather than an enumerator
* @todo Refactor connection code out of this method
*
* @see Listing for details of what file information is retrieved.
*/
STDMETHODIMP CProvider::GetListing(
	ISftpConsumer *pConsumer, BSTR bstrDirectory, IEnumListing **ppEnum )
{
	ATLENSURE_RETURN_HR(ppEnum, E_POINTER); *ppEnum = NULL;
	ATLENSURE_RETURN_HR(::SysStringLen(bstrDirectory) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(m_fInitialized, E_UNEXPECTED); // Call Initialize first

	HRESULT hr;

	try
	{

	// Connect to server
	hr = _Connect(pConsumer);
	if (FAILED(hr))
		return hr;

	// Open directory
	CW2A szDirectory(bstrDirectory);
	LIBSSH2_SFTP_HANDLE *pSftpHandle = libssh2_sftp_opendir(
		*m_session, szDirectory
	);
	if (!pSftpHandle)
	{
		bstr_t message("Could not open directory '");
		message += szDirectory.m_psz;
		message += "':\n\n";
		
		message += _GetLastErrorMessage();

		pConsumer->OnReportError(message.get_raw());
		return E_FAIL;
	}

	// Read entries from directory until we fail
	shared_ptr<vector<Listing> > files(new vector<Listing>);
    do {
		// Read filename and attributes. Returns length of filename retrieved.
		char szFilename[MAX_FILENAME_LENZ];
		::ZeroMemory(szFilename, sizeof(szFilename));

		char szLongEntry[MAX_LONGENTRY_LENZ];
		::ZeroMemory(szLongEntry, sizeof(szLongEntry));

        LIBSSH2_SFTP_ATTRIBUTES attrs;
		::ZeroMemory(&attrs, sizeof(attrs));

        int len = libssh2_sftp_readdir_ex(
			pSftpHandle, szFilename, sizeof(szFilename) - 1,
			szLongEntry, sizeof(szLongEntry) - 1, &attrs);
		if (len <= 0)
			break;
		else
		{
			// Make doubly sure
			szFilename[MAX_FILENAME_LEN] = '\0';
			szLongEntry[MAX_LONGENTRY_LEN] = '\0';
		}

		// Exclude . and ..
		if (szFilename[0] == '.')
		{
			if (len == 1 || (len == 2 && szFilename[1] == '.'))
				continue;
		}

		string strFilename(szFilename);
		string strLongEntry(szLongEntry);
		files->push_back(
			listing::fill_listing_entry(strFilename, strLongEntry, attrs));
	} while (true);

    ATLVERIFY(libssh2_sftp_closedir(pSftpHandle) == 0);

	// Create copy of our list of Listings and put into an AddReffed 'holder'
	com_ptr<stl_enum_holder<vector<Listing> > > holder = 
		new stl_enum_holder<vector<Listing> >(files);
	IUnknown* unknown = holder->get_unknown();
	com_ptr<IEnumListing> listing = stl_enumeration<IEnumListing>::create(
		*holder->m_collection, unknown);
	*ppEnum = listing.detach();
	}
	catchCom()

	return hr;
}

/**
 * @todo  Add flag to interface to allow choice of read or write.
 */
STDMETHODIMP CProvider::GetFile(
	ISftpConsumer *pConsumer, BSTR bstrFilePath, BOOL fWriteable,
	IStream **ppStream)
{
	ATLENSURE_RETURN_HR(ppStream, E_POINTER);
	 *ppStream = NULL;
	ATLENSURE_RETURN_HR(::SysStringLen(bstrFilePath) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(m_fInitialized, E_UNEXPECTED); // Call Initialize first

	HRESULT hr;

	// Connect to server
	hr = _Connect(pConsumer);
	if (FAILED(hr))
		return hr;

	try
	{
		CSftpStream::OpenFlags flags = CSftpStream::read | CSftpStream::create;
		if (fWriteable)
			flags |= CSftpStream::write;

		com_ptr<IStream> stream = new CSftpStream(
			m_session, CW2A(bstrFilePath).m_psz, flags);
		*ppStream = stream.detach();
	}
	catchCom()

	return hr;
}

/**
 * Renames a file or directory.
 *
 * The source and target file or directory must be specified using absolute 
 * paths for the remote filesytem.  The results of passing relative paths are 
 * not guaranteed (though, libssh2 seems to default to operating in the home 
 * directory) and may be dangerous.
 *
 * If a file or folder already exists at the target path, @a bstrToPath, 
 * we inform the front-end consumer through a call to OnConfirmOverwrite.
 * If confirmation is given, we attempt to overwrite the
 * obstruction with the source path, @a bstrFromPath, and if successful we
 * @c VARIANT_TRUE in @a pfWasTargetOverwritten.  This can be used by the 
 * caller to decide whether or not to update a directory view.
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
 * @param [in] bstrFromPath            Absolute path of the file or directory
 *                                     to be renamed.
 *
 * @param [in] bstrToPath              Absolute path that @a bstrFromPath
 *                                     should be renamed to.
 *
 * @param [out] pfWasTargetOverwritten Address in which to return whether or not
 *                                     we needed to overwrite an existing file 
 *                                     or directory at the target path. 
 */
STDMETHODIMP CProvider::Rename(
	ISftpConsumer *pConsumer, BSTR bstrFromPath, BSTR bstrToPath, 
	VARIANT_BOOL *pfWasTargetOverwritten)
{
	ATLENSURE_RETURN_HR(::SysStringLen(bstrFromPath) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(::SysStringLen(bstrToPath) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(m_fInitialized, E_UNEXPECTED); // Call Initialize first

	*pfWasTargetOverwritten = VARIANT_FALSE;

	// NOP if filenames are equal
	if (bstr_t(bstrFromPath) == bstr_t(bstrToPath))
		return S_OK;

	HRESULT hr;

	// Connect to server
	hr = _Connect(pConsumer);
	if (FAILED(hr))
		return hr;

	// Attempt to rename old path to new path
	string from = WideStringToUtf8String(bstrFromPath);
	string to = WideStringToUtf8String(bstrToPath);
	hr = _RenameSimple(from, to);
	if (SUCCEEDED(hr)) // Rename was successful without overwrite
		return S_OK;

	// Rename failed - this is OK, it might be an overwrite - check
	bstr_t message;
	int nErr; PSTR pszErr; int cchErr;
	nErr = libssh2_session_last_error(*m_session, &pszErr, &cchErr, false);
	if (nErr == LIBSSH2_ERROR_SFTP_PROTOCOL)
	{
		CString strError;
		hr = _RenameRetryWithOverwrite(
			pConsumer, libssh2_sftp_last_error(*m_session), from, to,
			strError);
		if (SUCCEEDED(hr))
		{
			*pfWasTargetOverwritten = VARIANT_TRUE;
			return S_OK;
		}
		if (hr == E_ABORT) // User denied overwrite
			return hr;
		else
			message = strError.GetString();
	}
	else // A non-SFTP error occurred
		message = pszErr;

	// Report remaining errors to front-end
	pConsumer->OnReportError(message.in());

	return E_FAIL;
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
HRESULT CProvider::_RenameSimple(const string& from, const string& to)
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
 * message from libssh2 is returned in @a strError.
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
 * @param [out] strError       Error message if the operation fails.
 *
 * @bug  The strings aren't converted from UTF-8 to UTF-16 before displaying
 *       to the user.  Any unicode filenames will produce gibberish in the
 *       confirmation dialogues.
 */
HRESULT CProvider::_RenameRetryWithOverwrite(
	ISftpConsumer *pConsumer,
	ULONG uPreviousError, const string& from, const string& to, 
	CString& strError)
{
	HRESULT hr;

	if (uPreviousError == LIBSSH2_FX_FILE_ALREADY_EXISTS)
	{
		hr = pConsumer->OnConfirmOverwrite(bstr_t(from).in(), bstr_t(to).in());
		if (FAILED(hr))
			return E_ABORT; // User disallowed overwrite

		// Attempt rename again this time allowing overwrite
		return _RenameAtomicOverwrite(from, to, strError);
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

			return _RenameNonAtomicOverwrite(from, to, strError);
		}
	}
		
	// File does not already exist, another error caused rename failure
	strError = _GetSftpErrorMessage(uPreviousError);
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
 * @param [out] strError  Error message if the operation fails.
 */
HRESULT CProvider::_RenameAtomicOverwrite(
	const string& from, const string& to, CString& strError)
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
		strError = pszMessage;
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
 * @param [out] strError  Error message if the operation fails.
 */
HRESULT CProvider::_RenameNonAtomicOverwrite(
	const string& from, const string& to, CString& strError)
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
			CString strError; // unused
			HRESULT hr = _DeleteRecursive(temporary.c_str(), strError);
			ATLASSERT(SUCCEEDED(hr));
			(void)hr; // The rename succeeded even if this fails
			return S_OK;
		}

		// Rename failed, rename our temporary back to its old name
		rc = libssh2_sftp_rename(*m_session, from.c_str(), to.c_str());
		ATLASSERT(!rc);

		strError = _T("Cannot overwrite \"");
		strError += (from + "\" with \"" + to).c_str();
		strError += _T("\": Please specify a different name or delete \"");
		strError += (to + "\" first.").c_str();
	}

	return E_FAIL;
}

STDMETHODIMP CProvider::Delete(ISftpConsumer *pConsumer, BSTR bstrPath)
{
	ATLENSURE_RETURN_HR(::SysStringLen(bstrPath) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(m_fInitialized, E_UNEXPECTED); // Call Initialize first

	HRESULT hr;

	// Connect to server
	hr = _Connect(pConsumer);
	if (FAILED(hr))
		return hr;

	// Delete file
	CString strError;
	string path = WideStringToUtf8String(bstrPath);
	hr = _Delete(path.c_str(), strError);
	if (SUCCEEDED(hr))
		return S_OK;

	// Report errors to front-end
	pConsumer->OnReportError(CComBSTR(strError));
	return E_FAIL;
}

HRESULT CProvider::_Delete( const char *szPath, CString& strError )
{
	if (libssh2_sftp_unlink(*m_session, szPath) == 0)
		return S_OK;

	// Delete failed
	strError = _GetLastErrorMessage();
	return E_FAIL;
}

STDMETHODIMP CProvider::DeleteDirectory(
										ISftpConsumer *pConsumer, BSTR bstrPath)
{
	ATLENSURE_RETURN_HR(::SysStringLen(bstrPath) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(m_fInitialized, E_UNEXPECTED); // Call Initialize first

	HRESULT hr;

	// Connect to server
	hr = _Connect(pConsumer);
	if (FAILED(hr))
		return hr;

	// Delete directory recursively
	CString strError;
	string path = WideStringToUtf8String(bstrPath);
	hr = _DeleteDirectory(path.c_str(), strError);
	if (SUCCEEDED(hr))
		return S_OK;

	// Report errors to front-end
	pConsumer->OnReportError(CComBSTR(strError));
	return E_FAIL;
}

HRESULT CProvider::_DeleteDirectory(
	const char *szPath, CString& strError )
{
	HRESULT hr;

	// Open directory
	LIBSSH2_SFTP_HANDLE *pSftpHandle = libssh2_sftp_opendir(
		*m_session, szPath
	);
	if (!pSftpHandle)
	{
		strError = _GetLastErrorMessage();
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

		ATLENSURE(szFilename[0]); // TODO: can files have no filename?
		if (szFilename[0] == '.' && !szFilename[1])
			continue; // Skip .
		if (szFilename[0] == '.' && szFilename[1] == '.' && !szFilename[2])
			continue; // Skip ..

		string strSubPath(szPath);
		strSubPath += "/";
		strSubPath += szFilename;
		hr = _DeleteRecursive(strSubPath.c_str(), strError);
		if (FAILED(hr))
		{
			ATLVERIFY(libssh2_sftp_close_handle(pSftpHandle) == 0);
			return hr;
		}
	} while (true);
	ATLVERIFY(libssh2_sftp_close_handle(pSftpHandle) == 0);

	// Delete directory itself
	if (libssh2_sftp_rmdir(*m_session, szPath) == 0)
		return S_OK;

	// Delete failed
	strError = _GetLastErrorMessage();
	return E_FAIL;
}

HRESULT CProvider::_DeleteRecursive(
	const char *szPath, CString& strError)
{
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	::ZeroMemory(&attrs, sizeof attrs);
	if (libssh2_sftp_lstat(*m_session, szPath, &attrs) != 0)
	{
		strError = _GetLastErrorMessage();
		return E_FAIL;
	}

	ATLASSERT( // Permissions field is valid
		attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS);
	if (attrs.permissions & LIBSSH2_SFTP_S_IFDIR)
		return _DeleteDirectory(szPath, strError);
	else
		return _Delete(szPath, strError);
}

STDMETHODIMP CProvider::CreateNewFile(
	ISftpConsumer *pConsumer, BSTR bstrPath)
{
	ATLENSURE_RETURN_HR(::SysStringLen(bstrPath) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(m_fInitialized, E_UNEXPECTED); // Call Initialize first

	// Connect to server
	HRESULT hr = _Connect(pConsumer);
	if (FAILED(hr))
		return hr;

	CW2A szPath(bstrPath);
	LIBSSH2_SFTP_HANDLE *pHandle = libssh2_sftp_open(
		*m_session, szPath, LIBSSH2_FXF_CREAT, 0644);
	if (pHandle == NULL)
	{
		// Report error to front-end
		pConsumer->OnReportError(CComBSTR(_GetLastErrorMessage()));
		return E_FAIL;
	}

	ATLVERIFY(libssh2_sftp_close_handle(pHandle) == 0);
	return S_OK;
}

STDMETHODIMP CProvider::CreateNewDirectory(
	ISftpConsumer *pConsumer, BSTR bstrPath)
{
	ATLENSURE_RETURN_HR(::SysStringLen(bstrPath) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(m_fInitialized, E_UNEXPECTED); // Call Initialize first

	// Connect to server
	HRESULT hr = _Connect(pConsumer);
	if (FAILED(hr))
		return hr;

	CW2A szPath(bstrPath);
	if (libssh2_sftp_mkdir(*m_session, szPath, 0755) != 0)
	{
		// Report error to front-end
		pConsumer->OnReportError(CComBSTR(_GetLastErrorMessage()));
		return E_FAIL;
	}

	return S_OK;
}

/**
 * Retrieves a string description of the last error reported by libssh2.
 *
 * In the case that the last SSH error is an SFTP error it returns the SFTP
 * error message in preference.
 */
CString CProvider::_GetLastErrorMessage()
{
	CString bstrMessage;
	int nErr; PSTR pszErr; int cchErr;

	nErr = libssh2_session_last_error(*m_session, &pszErr, &cchErr, false);
	if (nErr == LIBSSH2_ERROR_SFTP_PROTOCOL)
	{
		ULONG uErr = libssh2_sftp_last_error(*m_session);
		return _GetSftpErrorMessage(uErr);
	}
	else // A non-SFTP error occurred
		return CString(pszErr);
}

/**
 * Maps between libssh2 SFTP error codes and an appropriate error string.
 *
 * @param uError  SFTP error code as returned by libssh2_sftp_last_error().
 */
CString CProvider::_GetSftpErrorMessage(ULONG uError)
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

#pragma warning (pop)
