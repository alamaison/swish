/**
 * @file
 *
 * Mock ISftpProvider COM object implementation.
 */

#include "standard.h"

#include "MockSftpProvider.h"

#include "DummyStream.h"      // Bare-bones IStream implementation

#include <AtlComTime.h>       // COleDateTime

using namespace ATL;

using std::vector;

using std::wstring;

using std::find_if;
using std::sort;

using std::bind2nd;

// Set up default behaviours
CMockSftpProvider::CMockSftpProvider() :
	m_enumListingBehaviour(MockListing),
	m_enumRenameBehaviour(RenameOK)
{
	// Create filesystem root
	FilesystemLocation root = m_filesystem.insert(
		m_filesystem.begin(), _MakeDirectoryItem(L""));

	// Create two subdirectories and fill them with an expected set of items
	// whose names are 'tagged' with the directory name
	FilesystemLocation tmp =
		m_filesystem.append_child(root, _MakeDirectoryItem(L"tmp"));
	FilesystemLocation swish =
		m_filesystem.append_child(tmp, _MakeDirectoryItem(L"swish"));
	_FillMockListing(L"/tmp");
	_FillMockListing(L"/tmp/swish");
}

void CMockSftpProvider::SetListingBehaviour( ListingBehaviour enumBehaviour )
{
	m_enumListingBehaviour = enumBehaviour;
}

void CMockSftpProvider::SetRenameBehaviour( RenameBehaviour enumBehaviour )
{
	m_enumRenameBehaviour = enumBehaviour;
}

STDMETHODIMP CMockSftpProvider::Initialize(
	BSTR bstrUser, BSTR bstrHost, UINT uPort )
{
	// Test strings
	CPPUNIT_ASSERT( CComBSTR(bstrUser).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrUser).Length() <= MAX_USERNAME_LEN );
	CPPUNIT_ASSERT( CComBSTR(bstrHost).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrHost).Length() <= MAX_HOSTNAME_LEN );

	// Test port number
	CPPUNIT_ASSERT( uPort >= MIN_PORT );
	CPPUNIT_ASSERT( uPort <= MAX_PORT ); 

	return S_OK;
}


STDMETHODIMP CMockSftpProvider::SwitchConsumer( ISftpConsumer *pConsumer )
{
	m_spConsumer = pConsumer;
	return S_OK;
}

STDMETHODIMP CMockSftpProvider::GetListing(
	BSTR bstrDirectory, IEnumListing **ppEnum )
{
	// Test directory name
	CPPUNIT_ASSERT( CComBSTR(bstrDirectory).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrDirectory).Length() <= MAX_PATH_LEN );
	// Temporary condtion - remove for Windows support
	CPPUNIT_ASSERT( CComBSTR(bstrDirectory)[0] == OLECHAR('/') );

	// Test pointer
	CPPUNIT_ASSERT( ppEnum );
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"[out] pointer must be NULL when referenced (i.e. point to NULL)",
		(IEnumListing *)NULL, *ppEnum
	);

	vector<Listing> files; // List of files in directory
	Listing *pStart;
	Listing *pEnd;

	switch (m_enumListingBehaviour)
	{
	case EmptyListing:
		// Create dummy start and end pointers.  Making the equal and
		// non-NULL indicates an empty collection.
		pStart = pEnd = (Listing*)this; // Why 'this'? Why not.
		break;

	case MockListing:
		{
			FilesystemLocation dir = _FindLocationFromPath(bstrDirectory);
			CPPUNIT_ASSERT_MESSAGE(
				"Requested a listing that hasn't been generated.",
				 dir != m_filesystem.end()
			);
				
			// Copy directory out of tree and sort alphabetically
			files.insert(
				files.begin(), m_filesystem.begin(dir), m_filesystem.end(dir));
			sort(files.begin(), files.end(), lt_item());
			
			if (!files.empty())
			{
				pStart = &files[0];
				pEnd = &files[0] + files.size();
			}
			else // Make dummy pointer to empty collection
			{
				pStart = pEnd = (Listing*)this; // Why 'this'? Why not.
			}
		}
		break;

	case SFalseNoListing:
		return S_FALSE;

	case AbortListing:
		return E_ABORT;

	case FailListing:
		return E_FAIL;

	default:
		CPPUNIT_FAIL("Unreachable: Unrecognised GetListing() behaviour");
		return E_UNEXPECTED;
	}

	// Create instance of Enum from ATL template class
	CComObject<CMockEnumListing> *pEnum = NULL;
	HRESULT hr = CComObject<CMockEnumListing>::CreateInstance(&pEnum);
	CPPUNIT_ASSERT_OK(hr);

	// Initialise enumerator with list of files
	pEnum->AddRef();

	hr = pEnum->Init(pStart, pEnd, NULL, AtlFlagCopy);
	CPPUNIT_ASSERT_OK(hr);

	hr = pEnum->QueryInterface(ppEnum);
	CPPUNIT_ASSERT_OK(hr);

	pEnum->Release();

	return S_OK;

}


STDMETHODIMP CMockSftpProvider::GetFile(
	BSTR bstrFilePath, BOOL, IStream **ppStream)
{
	CPPUNIT_ASSERT( ppStream );
	CPPUNIT_ASSERT( CComBSTR(bstrFilePath).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrFilePath).Length() <= MAX_FILENAME_LEN );
	// Temporary condtion - remove for Windows support
	CPPUNIT_ASSERT( CComBSTR(bstrFilePath)[0] == OLECHAR('/') );

	*ppStream = NULL;

	_TestMockPathExists(bstrFilePath);

	// Create dummy IStream instance whose data is the file path
	CComObject<CDummyStream> *pStream = NULL;
	HRESULT hr = pStream->CreateInstance(&pStream);
	if (SUCCEEDED(hr))
	{
		pStream->AddRef();

		hr = pStream->Initialize(CW2A(bstrFilePath));
		if (SUCCEEDED(hr))
		{
			hr = pStream->QueryInterface(ppStream);
		}

		pStream->Release();
	}

	return hr;
}

STDMETHODIMP CMockSftpProvider::Rename(
	BSTR bstrFromPath, BSTR bstrToPath, VARIANT_BOOL *fWasTargetOverwritten )
{
	// Test filenames
	CPPUNIT_ASSERT( CComBSTR(bstrFromPath).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrFromPath).Length() <= MAX_FILENAME_LEN );
	CPPUNIT_ASSERT( CComBSTR(bstrToPath).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrToPath).Length() <= MAX_FILENAME_LEN );
	// Temporary condtion - remove for Windows support
	CPPUNIT_ASSERT( CComBSTR(bstrFromPath)[0] == OLECHAR('/') );
	CPPUNIT_ASSERT( CComBSTR(bstrToPath)[0] == OLECHAR('/') );

	*fWasTargetOverwritten = VARIANT_FALSE;

	_TestMockPathExists(bstrFromPath);

	// Perform selected behaviour
	HRESULT hr;
	switch (m_enumRenameBehaviour)
	{
	case RenameOK:
		return S_OK;

	case ConfirmOverwrite:
		hr = m_spConsumer->OnConfirmOverwrite(bstrFromPath, bstrToPath);
		if (SUCCEEDED(hr))
			*fWasTargetOverwritten = VARIANT_TRUE;
		return hr;

	case ConfirmOverwriteEx:
		{
		// TODO: Lookup Listing from collection we returned in GetListing()
		Listing ltOld = {
			bstrFromPath, 0666, CComBSTR("mockowner"), 
			CComBSTR("mockgroup"), 1001, 1002, 1024, 12, COleDateTime()
		};
		Listing ltExisting =  {
			bstrToPath, 0666, CComBSTR("mockowner"), 
			CComBSTR("mockgroup"), 1001, 1002, 1024, 12, COleDateTime()
		};

		hr = m_spConsumer->OnConfirmOverwriteEx(ltOld, ltExisting);
		if (SUCCEEDED(hr))
			*fWasTargetOverwritten = VARIANT_TRUE;
		return hr;
		}

	case ReportError:
		hr = m_spConsumer->OnReportError(
			CComBSTR("Mock error message \"CMockSftpProvider::Rename\"")
		);
		return E_FAIL;

	case AbortRename:
		return E_ABORT;

	case FailRename:
		return E_FAIL;

	default:
		CPPUNIT_FAIL("Unreachable: Unrecognised Rename() behaviour");
		return E_UNEXPECTED;
	}
}

STDMETHODIMP CMockSftpProvider::Delete( BSTR bstrPath )
{
	CPPUNIT_ASSERT( CComBSTR(bstrPath).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrPath).Length() <= MAX_FILENAME_LEN );
	// Temporary condtion - remove for Windows support
	CPPUNIT_ASSERT( CComBSTR(bstrPath)[0] == OLECHAR('/') );

	return S_OK;
}

STDMETHODIMP CMockSftpProvider::DeleteDirectory( BSTR bstrPath )
{
	CPPUNIT_ASSERT( CComBSTR(bstrPath).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrPath).Length() <= MAX_FILENAME_LEN );
	// Temporary condtion - remove for Windows support
	CPPUNIT_ASSERT( CComBSTR(bstrPath)[0] == OLECHAR('/') );

	return S_OK;
}

STDMETHODIMP CMockSftpProvider::CreateNewFile( BSTR bstrPath )
{
	CPPUNIT_ASSERT( CComBSTR(bstrPath).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrPath).Length() <= MAX_FILENAME_LEN );
	// Temporary condtion - remove for Windows support
	CPPUNIT_ASSERT( CComBSTR(bstrPath)[0] == OLECHAR('/') );

	return S_OK;
}

STDMETHODIMP CMockSftpProvider::CreateNewDirectory( BSTR bstrPath )
{
	CPPUNIT_ASSERT( CComBSTR(bstrPath).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrPath).Length() <= MAX_FILENAME_LEN );
	// Temporary condtion - remove for Windows support
	CPPUNIT_ASSERT( CComBSTR(bstrPath)[0] == OLECHAR('/') );

	return S_OK;
}

CComBSTR CMockSftpProvider::_TagFilename(PCWSTR pszFilename, PCWSTR pszTag)
{
	CString strName;
	strName.Format(pszFilename, pszTag);
	return CComBSTR(strName);
}

/**
 * Generates a listing for the given directory and tags each filename with the
 * name of the parent folder.  This allows us to detect a correct listing later.
 */
void CMockSftpProvider::_FillMockListing(PCWSTR pwszDirectory)
{
	// Get directory name part of path for tag: "/tmp/swish/" -> "swish"
	CString strPath(pwszDirectory);
	strPath.TrimRight(L'/');
	int iLastSep = strPath.ReverseFind(L'/');
	int cFilenameLen = strPath.GetLength() - (iLastSep+1);
	CString strDir = strPath.Right(cFilenameLen);

	// Fill our vector with dummy files
	vector<CComBSTR> vecFilenames;
	vecFilenames.push_back(_TagFilename(L"test%sfile", strDir));
	vecFilenames.push_back(_TagFilename(L"test%sFile", strDir));
	vecFilenames.push_back(_TagFilename(L"test%sfile.ext", strDir));
	vecFilenames.push_back(_TagFilename(L"test%sfile.txt", strDir));
	vecFilenames.push_back(_TagFilename(L"test%sfile with spaces", strDir));
	vecFilenames.push_back(
		_TagFilename(L"test%sfile with \"quotes\" and spaces", strDir));
	vecFilenames.push_back(_TagFilename(L"test%sfile.ext.txt", strDir));
	vecFilenames.push_back(_TagFilename(L"test%sfile..", strDir));
	vecFilenames.push_back(_TagFilename(L".test%shiddenfile", strDir));

	vector<DATE> vecDates;
	vecDates.push_back(COleDateTime());
	vecDates.push_back(COleDateTime::GetCurrentTime());
	vecDates.push_back(COleDateTime(1899, 7, 13, 17, 59, 12));
	vecDates.push_back(COleDateTime(9999, 12, 31, 23, 59, 59));
	vecDates.push_back(COleDateTime(2000, 2, 29, 12, 47, 1));
	vecDates.push_back(COleDateTime(1978, 3, 3, 3, 00, 00));
	vecDates.push_back(COleDateTime(1601, 1, 1, 0, 00, 00));
	vecDates.push_back(COleDateTime(2007, 2, 28, 0, 0, 0));
	vecDates.push_back(COleDateTime(1752, 9, 03, 7, 27, 8));

	unsigned uCycle = 0, uSize = 0;
	while (!vecFilenames.empty())
	{
		// Try to cycle through the permissions on each successive file
		// TODO: I have no idea if this works
		unsigned uPerms = 
			(uCycle % 1) || ((uCycle % 2) << 1) || ((uCycle % 3) << 2);

		Listing lt;
		lt.bstrFilename = vecFilenames.back().Detach();
		lt.uPermissions = uPerms;
		lt.bstrOwner =  CComBSTR("mockowner").Detach();
		lt.bstrGroup = CComBSTR("mockgroup").Detach();
		lt.uSize = uSize;
		lt.cHardLinks = uCycle;
		lt.dateModified = vecDates.back();
		ATLASSERT( 
			COleDateTime(lt.dateModified).GetStatus() == COleDateTime::valid
		);
		_MakeItemIn(pwszDirectory, lt);

		vecDates.pop_back();
		vecFilenames.pop_back();
		uCycle++;
		uSize = (uSize + uCycle) << 10;
	}

	// Add some dummy folders also
	vector<CComBSTR> vecFoldernames;
	vecFoldernames.push_back(_TagFilename(L"Test%sfolder", strDir));
	vecFoldernames.push_back(_TagFilename(L"test%sfolder.ext", strDir));
	vecFoldernames.push_back(_TagFilename(L"test%sfolder.bmp", strDir));
	vecFoldernames.push_back(_TagFilename(L"test%sfolder with spaces", strDir));
	vecFoldernames.push_back(_TagFilename(L".test%shiddenfolder", strDir));

	while (!vecFoldernames.empty())
	{
		_MakeItemIn(pwszDirectory, _MakeDirectoryItem(vecFoldernames.back()));
		vecFoldernames.pop_back();
	}
}

Listing CMockSftpProvider::_MakeDirectoryItem(PCWSTR pwszName)
{
	Listing lt;
	lt.bstrFilename = CComBSTR(pwszName).Detach();
	lt.uPermissions = 040777;
	lt.bstrOwner =  CComBSTR("mockowner").Detach();
	lt.bstrGroup = CComBSTR("mockgroup").Detach();
	lt.uSize = 42;
	lt.cHardLinks = 7;
	lt.dateModified = COleDateTime(1601, 10, 5, 13, 54, 22);
	ATLASSERT( 
		COleDateTime(lt.dateModified).GetStatus() == COleDateTime::valid);

	return lt;
}

void CMockSftpProvider::_MakeItemIn(
	FilesystemLocation loc, const Listing& item)
{
	m_filesystem.append_child(loc, item);
}

void CMockSftpProvider::_MakeItemIn(
	const wstring& path, const Listing& item)
{
	FilesystemLocation loc = _FindLocationFromPath(path);
	ATLASSERT(loc != m_filesystem.end());
	_MakeItemIn(loc, item);
}

/**
 * Return an iterator to the node in the mock filesystem indicated by the
 * path given as a string.
 */
CMockSftpProvider::FilesystemLocation CMockSftpProvider::_FindLocationFromPath(
	const wstring& path)
{
	vector<wstring> tokens = _TokenisePath(path);

	// Start searching in root of 'filesystem'
	FilesystemLocation currentDir = m_filesystem.begin();

	// Walk down list of tokens finding each item below the previous
	for (vector<wstring>::const_iterator it = tokens.begin();
		it != tokens.end();
		it++)
	{
		FilesystemLocation dir = find_if(
			m_filesystem.begin(currentDir), m_filesystem.end(currentDir),
			bind2nd(eq_item(), *it));

		if (dir == m_filesystem.end(currentDir))
			return m_filesystem.end();
		
		currentDir = dir;
	}

	return currentDir;
}

vector<wstring> CMockSftpProvider::_TokenisePath(const wstring& path)
{
	vector<wstring> tokens;

	size_t pos = path.find_first_not_of(L'/'); // Skip initial slashes
	size_t limit = pos;
	size_t last = path.find_last_not_of(L'/'); // Last valid character

	while (limit < last)
	{
		limit = path.find(L'/', pos);
		tokens.push_back(path.substr(pos, limit - pos));
		pos = limit + 1; // Skip delimeter
	}

	return tokens;
}

void CMockSftpProvider::_TestMockPathExists(PCWSTR pwszPath)
{
	// Find filename and directory
	CString strFile(pwszPath);
	strFile.TrimRight(L'/');
	int iLastSep = strFile.ReverseFind(L'/');
	wstring strDirectory = strFile.Left(iLastSep+1);

	CPPUNIT_ASSERT_MESSAGE(
		"The requested file is in a directory which hasn't been generated. "
		"This is probably not intended.",
		_FindLocationFromPath(strDirectory) != m_filesystem.end()
	);
	CPPUNIT_ASSERT_MESSAGE(
		"The file was not found in the mock collection.",
		_FindLocationFromPath(pwszPath) != m_filesystem.end()
	);
}