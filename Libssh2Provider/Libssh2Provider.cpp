/*  Implementation of libssh2-based SFTP component.

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

#include "stdafx.h"
#include "resource.h"
#include "../remotelimits.h"

#include "Libssh2Provider.h"

#include <ws2tcpip.h>    // Winsock
#include <wspiapi.h>     // Winsock

#include <ATLComTime.h>  // COleDateTime


/**
 * Create libssh2-based data provider component instance.
 *
 * @warning
 *   The Initialize() method must be called before the other methods
 *   of the object can be used.
 */
CLibssh2Provider::CLibssh2Provider() :
	m_fInitialized(false),
	m_pConsumer(NULL),
	m_pSession(NULL),
	m_pSftpSession(NULL),
	m_socket(INVALID_SOCKET)
{
}

/**
 * Returns constructor success or failure.
 *
 * As various parts of the initialisation can potentially fail, they
 * are done here rather than in the constructor.
 */
HRESULT CLibssh2Provider::FinalConstruct()
{
	// Initialise libssh2
	ATLASSUME(!m_pSession);
	m_pSession = libssh2_session_init();
	ATLENSURE_RETURN_HR(m_pSession, E_UNEXPECTED);

	// Start up Winsock
	WSADATA wsadata;
	int rc = ::WSAStartup(WINSOCK_VERSION, &wsadata);
	ATLENSURE_RETURN_HR(!rc, E_UNEXPECTED);

	// TODO: examine wsadata to verify Winsock version is acceptable

	return S_OK;
}

/**
 * Free libssh2 and shutdown Winsock
 */
void CLibssh2Provider::FinalRelease()
{
	_Disconnect();

	if (m_pSession)	// dual of libssh2_session_init()
		libssh2_session_free(m_pSession);

	ATLVERIFY( !::WSACleanup() );

	if (m_pConsumer)
		m_pConsumer->Release();
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
 * @param pConsumer An ISftpConsumer to handle user-interaction callbacks. This
 *                  is half of the bi-dir ISftpProvider/ISftpConsumer pair.
 * @param bstrUser The user name of the SSH account.
 * @param bstrHost The name of the machine to connect to.
 * @param uPort    The TCP/IP port on which the SSH server is running.
 * @returns
 *   @c S_OK if successful,
 *   @c E_INVALIDARG if either string parameter was empty or port is invalid, 
 *   @c E_POINTER is pConsumer is invalid,
 *   @c E_FAIL if other error encountered.
 */
STDMETHODIMP CLibssh2Provider::Initialize(
	ISftpConsumer *pConsumer, BSTR bstrUser, BSTR bstrHost, short uPort )
{

	ATLENSURE_RETURN_HR( pConsumer, E_POINTER );
	ATLENSURE_RETURN_HR( 
		::SysStringLen(bstrUser) > 0 && ::SysStringLen(bstrHost) > 0,
		E_INVALIDARG );
	ATLENSURE_RETURN_HR( uPort >= MIN_PORT, E_INVALIDARG );
	ATLENSURE_RETURN_HR( uPort <= MAX_PORT, E_INVALIDARG );

	m_pConsumer = pConsumer;
	m_pConsumer->AddRef();
	m_strUser = bstrUser;
	m_strHost = bstrHost;
	m_uPort = uPort;

	ATLASSUME( !m_strUser.IsEmpty() );
	ATLASSUME( !m_strHost.IsEmpty() );
	ATLASSUME( m_uPort >= MIN_PORT );
	ATLASSUME( m_uPort <= MAX_PORT );

	// Create a session instance
    m_pSession = libssh2_session_init();
    ATLENSURE_RETURN_HR( m_pSession, E_FAIL );

	// Tell libssh2 we are blocking
    libssh2_session_set_blocking(m_pSession, 1);

	m_fInitialized = true;
	return S_OK;
}

/**
 * Creates a socket and connects it to the host.
 *
 * The socket is stored as the member variable @c m_socket. The hostname 
 * and port used are the class members @c m_strHost and @c m_uPort.
 * If the socket has already been initialised, the function leaves it
 * unchanged (or asserts in a DEBUG build).  If any errors occur, the 
 * socket is set to @c INVALID_SOCKET and returns @c E_ABORT.
 *
 * @returns
 *   @c S_OK if successful,
 *   @c E_FAIL if the hostname could not be resolved or connecting to it failed,
 *   @c E_ABORT if the socket was already intialised,
 *   @c E_UNEXPECTED if other error occurs.
 *
 * @remarks The socket should be cleaned up when no longer needed using
 *          @c ::closesocket().
 */
HRESULT CLibssh2Provider::_OpenSocketToHost()
{
#pragma warning (push)
#pragma warning (disable: 4267) // ssize_t to unsigned int

	ATLASSUME(!m_strHost.IsEmpty());
	ATLASSUME(m_uPort >= MIN_PORT && m_uPort <= MAX_PORT);
	ATLENSURE_RETURN_HR(
		m_socket == INVALID_SOCKET, E_ABORT // Socket already set up!
	);

	// The hints address info struct which is passed to getaddrinfo()
	addrinfo aiHints;
	::ZeroMemory(&aiHints, sizeof(aiHints));
	aiHints.ai_family = AF_INET;
	aiHints.ai_socktype = SOCK_STREAM;
	aiHints.ai_protocol = IPPROTO_TCP;

	// Convert numeric port to an ANSI string
	char szPort[6];
	ATLENSURE_RETURN_HR(!::_itoa_s(m_uPort, szPort, 6, 10), E_UNEXPECTED);

	// Convert host address to an ANSI string
	CT2A szAddress(m_strHost);

	// Call getaddrinfo(). If the call succeeds, paiList will hold a linked list
	// of addrinfo structures containing response information about the host.
	addrinfo *paiList = NULL;
	int rc = ::getaddrinfo(szAddress, szPort, &aiHints, &paiList);
	// It is valid to fail here - e.g. unknown host
	// ATLASSERT_REPORT(!rc, ::WSAGetLastError());
	if (rc)
		return E_FAIL;
	ATLASSERT(paiList);
	ATLASSERT(paiList->ai_addr);

	// Create socket and establish connection
	HRESULT hr = S_OK;
	m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
	ATLASSERT_REPORT(m_socket != INVALID_SOCKET, ::WSAGetLastError());
	if (m_socket != INVALID_SOCKET)
	{
		rc = ::connect(m_socket, paiList->ai_addr, paiList->ai_addrlen);
		// ATLASSERT_REPORT(!rc, ::WSAGetLastError());
		hr = (!rc) ? S_OK : E_FAIL;
	}
	else
		hr = E_UNEXPECTED;

	::freeaddrinfo(paiList);
	return hr;

#pragma warning (pop)
}

HRESULT CLibssh2Provider::_VerifyHostKey()
{
#pragma warning (push)
#pragma warning (disable: 4267) // ssize_t to unsigned int

	ATLASSUME(m_pSession);

    const char *fingerprint = 
		libssh2_hostkey_hash(m_pSession, LIBSSH2_HOSTKEY_HASH_MD5);
	const char *key_type = 
		libssh2_session_methods(m_pSession, LIBSSH2_METHOD_HOSTKEY);

	// TODO: check the key is known here
	//   fprintf(stderr, "Fingerprint: ");
	//   for(i = 0; i < 16; i++) {
	//       fprintf(stderr, "%02X ", (unsigned char)fingerprint[i]);
	//   }
	//   fprintf(stderr, "\n");

#pragma warning (pop)
	return S_OK;
}

/**
 * Tries to authenticate the user with the remote server.
 *
 * The remote server is queried for which authentication methods it supports
 * and these are tried one at time until one succeeds in the order:
 * public-key, keyboard-interactive, plain password.
 *
 * @returns S_OK if authentication succeeded or E_ABORT if all methods failed.
 */
HRESULT CLibssh2Provider::_AunthenticateUser()
{
#pragma warning (push)
#pragma warning (disable: 4267) // ssize_t to unsigned int

	ATLASSUME(!m_strHost.IsEmpty());
	CT2A szUsername(m_strUser);

    // Check which authentication methods are available
    char *szUserauthlist = libssh2_userauth_list(
		m_pSession, szUsername, ::strlen(szUsername)
	);
	if (!szUserauthlist)
	{
        ATLTRACE("No supported authentication methods found!");
		return E_FAIL; // If empty, server refused to let user connect
	}

	ATLTRACE("Authentication methods: %s", szUserauthlist);

	// Try each supported authentication method in turn until one succeeds
	HRESULT hr = E_FAIL;
	if (::strstr(szUserauthlist, "publickey"))
	{
		ATLTRACE("Trying public-key authentication");
		hr = _PublicKeyAuthentication(szUsername);
	}
	if (FAILED(hr) && ::strstr(szUserauthlist, "keyboard-interactive"))
	{
		ATLTRACE("Trying keyboard-interactive authentication");
		hr = _KeyboardInteractiveAuthentication(szUsername);
    }
    if (FAILED(hr) && ::strstr(szUserauthlist, "password"))
	{
		ATLTRACE("Trying simple password authentication");
		hr = _PasswordAuthentication(szUsername);
    }

	return hr;
#pragma warning (pop)
}

/**
 * Authenticates with remote host by asking the user to supply a password.
 *
 * This uses the callback to the SftpConsumer to obtain the password from
 * the user.  If the password is wrong or other error occurs, the user is
 * asked for the password again.  This repeats until the user supplies a 
 * correct password or cancel the request.
 *
 * @returns S_OK if authentication is successful or E_ABORT if user cancels
 *          authentication
 */
HRESULT CLibssh2Provider::_PasswordAuthentication(PCSTR szUsername)
{
#pragma warning (push)
#pragma warning (disable: 4267) // ssize_t to unsigned int

	HRESULT hr;
	CComBSTR bstrPrompt = _T("Please enter your password:");
	CComBSTR bstrPassword;

	// Loop until successfully authenticated or request returns cancel
	int ret = -1; // default to failure
	do {
		hr = m_pConsumer->OnPasswordRequest( bstrPrompt, &bstrPassword );
		if FAILED(hr)
			return hr;
		CT2A szPassword(bstrPassword);
		ret = libssh2_userauth_password(
			m_pSession, szUsername, szPassword
		);
		// TODO: handle password change callback here
		bstrPassword.Empty(); // Prevent memory leak on repeat
	} while (ret != 0);

	ATLASSERT(SUCCEEDED(hr)); ATLASSERT(ret == 0);
	return hr;

#pragma warning (pop)
}

HRESULT CLibssh2Provider::_KeyboardInteractiveAuthentication(PCSTR szUsername)
{
	// TODO: implement keyboard-interactive callback
    //if (libssh2_userauth_keyboard_interactive(
	//    session, username, &kbd_callback) ) {
    //    ATLTRACE("keyboard-interactive authentication failed");
    //} else {
    //    ATLTRACE("keyboard-interactive authentication succeeded");
    //}
	(void) szUsername;
	return E_ABORT;
}

HRESULT CLibssh2Provider::_PublicKeyAuthentication(PCSTR szUsername)
{
#pragma warning (push)
#pragma warning (disable: 4267) // ssize_t to unsigned int

	// TODO: use proper file paths
	const char *keyfile1="~/.ssh/id_rsa.pub";
	const char *keyfile2="~/.ssh/id_rsa";
	// TODO: unlock public key using passphrase
	if (libssh2_userauth_publickey_fromfile(
			m_pSession, szUsername, keyfile1, keyfile2, ""))
		return E_ABORT;

	ATLASSERT(libssh2_userauth_authenticated(m_pSession)); // Double-check
	return S_OK;

#pragma warning (pop)
}

/**
 * Sets up the SFTP session, prompting user for input if neccessary.
 *
 * The remote server must have its identity verified which may require user
 * confirmation and the user must authenticate with the remote server
 * which might be done silently (i.e. with a public-key) or may require
 * user input.
 *
 * @returns S_OK if session successfully created or an error otherwise.
 */
HRESULT CLibssh2Provider::_Connect()
{
	HRESULT hr;
	
	// Connect to host over TCP/IP
	hr = _OpenSocketToHost();
	if (FAILED(hr))
		return hr; // Legal to fail here, e.g. if host can't be found

    // Start up libssh2 and trade welcome banners, exchange keys,
    // setup crypto, compression, and MAC layers
    int rc = libssh2_session_startup(m_pSession, static_cast<int>(m_socket));
	if (rc)
		return E_FAIL; // Legal to fail here, e.g. server refuses banner/kex

    // Check the hostkey against our known hosts
	hr = _VerifyHostKey();
	if (FAILED(hr))
		return hr; // Legal to fail here, e.g. user refused to accept host key

    // Authenticate the user with the remote server
	hr = _AunthenticateUser();
	if (FAILED(hr))
		return hr; // Legal to fail here, e.g. wrong password/key

	// Start up SFTP session
    m_pSftpSession = libssh2_sftp_init(m_pSession);
#ifdef _DEBUG
	if (!m_pSftpSession)
	{
		char *szError; int cchError;
		int rc = ::libssh2_session_last_error(
			m_pSession, &szError, &cchError, false);
		ATLTRACE("libssh2_sftp_init failed (%d): %s", rc, szError);
	}
#endif
	ATLENSURE_RETURN_HR( m_pSftpSession, E_FAIL );
	return S_OK;
}

/**
 * Cleans up any connections or resources that may have been created.
 */
HRESULT CLibssh2Provider::_Disconnect()
{
	
	if (m_pSftpSession)	// dual of libssh2_sftp_init()
		ATLVERIFY( !libssh2_sftp_shutdown(m_pSftpSession) );
	
	if (m_pSession)		// dual of libssh2_session_startup()
		ATLVERIFY( !libssh2_session_disconnect(m_pSession, "Session over") );

	if (m_socket != INVALID_SOCKET) // dual of ::socket()
		ATLVERIFY_REPORT( !::closesocket(m_socket), ::WSAGetLastError() );

	return S_OK;
}

/**
* Retrieves a file listing, @c ls, of a given directory.
*
* The listing is returned as an IEnumListing of Listing objects.
*
* @pre The Initialize() method must have been called first
*
* @param [in]  bstrDirectory The absolute path of the directory to list
* @param [out] ppEnum        A pointer to the IEnumListing interface pointer
* @returns 
*   @c S_OK and IEnumListing pointer if successful, @c E_UNEXPECTED if
*   Initialize() was not previously called, @c E_FAIL if other error occurs.
*
* @todo Handle the case where the directory does not exist
* @todo Return a collection rather than an enumerator
* @todo Refactor connection code out of this method
*
* @see Listing for details of what file information is retrieved.
*/
STDMETHODIMP CLibssh2Provider::GetListing(
	BSTR bstrDirectory, IEnumListing **ppEnum )
{
	ATLENSURE_RETURN_HR(ppEnum, E_POINTER); *ppEnum = NULL;
	ATLENSURE_RETURN_HR(::SysStringLen(bstrDirectory) > 0, E_INVALIDARG);
	ATLENSURE_RETURN_HR(m_fInitialized, E_UNEXPECTED); // Call Initialize first
	ATLASSUME(m_pSession);
	//ATLASSUME(m_pSftpSession);
	ATLASSUME(!m_strUser.IsEmpty());
	ATLASSUME(!m_strHost.IsEmpty());

	typedef CComEnumOnSTL<IEnumListing, &__uuidof(IEnumListing), Listing, _Copy<Listing>, list<Listing> > CComEnumListing;

	HRESULT hr;

	// Connect to server
	hr = _Connect();
	if (FAILED(hr))
		return hr;
	ATLASSUME(m_pSftpSession);

	// Open directory
#pragma warning (push)
#pragma warning (disable: 4267) // ssize_t to unsigned int
	CW2A szDirectory(bstrDirectory);
	LIBSSH2_SFTP_HANDLE *pSftpHandle = libssh2_sftp_opendir(
		m_pSftpSession, szDirectory
	);
	if (!pSftpHandle)
		return E_FAIL;
#pragma warning (pop)

	// Read entries from directory until we fail
	int rc;
    do {
		// Read filename and attributes. Returns length of filename retrieved.
        char szFilename[512];
        LIBSSH2_SFTP_ATTRIBUTES attrs;
		::ZeroMemory(&attrs, sizeof(attrs));
        rc = libssh2_sftp_readdir(
			pSftpHandle, szFilename, sizeof(szFilename), &attrs
		);
		if(rc <= 0)
		{
			szFilename[0] = '\0';
			break;
		}

		m_lstFiles.push_back( _FillListingEntry(szFilename, attrs) );
    } while (1);

    ATLASSERT(!libssh2_sftp_closedir(pSftpHandle));

	// Create instance of Enum from ATL template class and get interface pointer
	CComObject<CComEnumListing> *pEnum = NULL;
	hr = CComObject<CComEnumListing>::CreateInstance(&pEnum);
	if (SUCCEEDED(hr))
	{
		pEnum->AddRef();
			hr = pEnum->Init(this->GetUnknown(), m_lstFiles);
			if (SUCCEEDED(hr))
				hr = pEnum->QueryInterface(ppEnum);
		pEnum->Release();
	}

	return hr;
}

/**
 * Creates a Listing object for a file entry based on filename and attributes.
 *
 * @param pszFilename The filename as an ANSI char string.
 * @param attrs       A reference to the LIBSSH2_SFTP_ATTRIBUTES containing
 *                    the file's details.
 *
 * @returns A listing object representing the file.
 */
Listing CLibssh2Provider::_FillListingEntry(
	PCSTR pszFilename,
	LIBSSH2_SFTP_ATTRIBUTES& attrs )
{
	Listing lt;

	// Filename
	CComBSTR bstrFile(pszFilename);
	HRESULT hr = bstrFile.CopyTo(&(lt.bstrFilename));
	ATLASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
		lt.bstrFilename = ::SysAllocString(OLESTR(""));

	// Permissions
    if (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS)
	{
		// TODO: fill with real permissions and add octal field
		lt.bstrPermissions = ::SysAllocString(OLESTR("--todo----"));
    }

	// User & Group
    if (attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID)
	{
		// TODO: find some way to return names not numbers
		CString strOwner;
		CString strGroup;
		strOwner.Format(_T("%4ld"), attrs.uid);
		strGroup.Format(_T("%4ld"), attrs.gid);
		// TODO: these never get free.  We need to find a way.
		lt.bstrOwner = strOwner.AllocSysString();
		lt.bstrGroup = strGroup.AllocSysString();
    }

	// Size of file
    if (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE)
	{
        // TODO: attrs.filesize is an uint64_t. The listings field is not big
		// enough
		lt.cSize = static_cast<unsigned long>(attrs.filesize);
    }

	// Access & Modification time
	if (attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME)
	{
		COleDateTime dateModified(static_cast<time_t>(attrs.mtime));
		COleDateTime dateAccessed(static_cast<time_t>(attrs.atime));
		lt.dateModified = dateModified;
		// TODO: add this field to Swish
		//lt.dateAccessed = dateAccessed;
	}

	return lt;
}

/*
   The module attribute causes 
     DllMain
     DllRegisterServer
     DllUnregisterServer
     DllCanUnloadNow
     DllGetClassObject. 
   to be automatically implemented
 */
[ module(dll, uuid = "{b816a846-5022-11dc-9153-0090f5284f85}", 
		 name = "Libssh2Provider", 
		 helpstring = "Libssh2Provider 0.1 Type Library",
		 resource_name = "IDR_LIBSSH2PROVIDER") ];
/*
class CLibssh2ProviderModule
{
public:
// Override CAtlDllModuleT members
};
*/
