// Mock SftpProvider COM object implementation

#include "stdafx.h"
#include "MockSftpProvider.h"
#include <AtlComTime.h>

// Set up default behaviours
CMockSftpProvider::CMockSftpProvider() :
	m_enumListingBehaviour(MockListing),
	m_enumRenameBehaviour(RenameOK)
{
	_FillMockListing(_T("/tmp/"));
	_FillMockListing(_T("/tmp/swish/"));
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
	Swish::ISftpConsumer *pConsumer, BSTR bstrUser, BSTR bstrHost, UINT uPort )
{
	// Test ISftpConsumer pointer
	CPPUNIT_ASSERT( pConsumer );
	CPPUNIT_ASSERT_NO_THROW_MESSAGE(
		"AddRef failed for ISftpConsumer",
		pConsumer->AddRef()
	);
	CPPUNIT_ASSERT_NO_THROW_MESSAGE(
		"AddRef failed for ISftpConsumer",
		pConsumer->Release()
	);

	// Test strings
	CPPUNIT_ASSERT( CComBSTR(bstrUser).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrUser).Length() <= MAX_USERNAME_LEN );
	CPPUNIT_ASSERT( CComBSTR(bstrHost).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrHost).Length() <= MAX_HOSTNAME_LEN );

	// Test port number
	CPPUNIT_ASSERT( uPort >= MIN_PORT );
	CPPUNIT_ASSERT( uPort <= MAX_PORT ); 

	// Save pointer for later use
	m_spConsumer = pConsumer;
	CPPUNIT_ASSERT( m_spConsumer );

	return S_OK;
}


STDMETHODIMP CMockSftpProvider::SwitchConsumer( Swish::ISftpConsumer *pConsumer )
{
	m_spConsumer = pConsumer;
	return S_OK;
}

STDMETHODIMP CMockSftpProvider::GetListing(
	BSTR bstrDirectory, Swish::IEnumListing **ppEnum )
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
		(Swish::IEnumListing *)NULL, *ppEnum
	);

	switch (m_enumListingBehaviour)
	{
	case EmptyListing: // Fallthru: listing populated in constructor
		m_mapDirectories[bstrDirectory].clear();
	case MockListing:
		{
			typedef CComEnumOnSTL<Swish::IEnumListing,
				&__uuidof(Swish::IEnumListing), Swish::Listing,
				_Copy<Swish::Listing>, vector<Swish::Listing> >
			CComEnumListing;

			CPPUNIT_ASSERT_MESSAGE(
				"Requested a listing that hasn't been generated.",
				m_mapDirectories.find(bstrDirectory) != m_mapDirectories.end()
			);

			// Create instance of Enum from ATL template class
			CComObject<CComEnumListing> *pEnum = NULL;
			HRESULT hr = CComObject<CComEnumListing>::CreateInstance(&pEnum);
			CPPUNIT_ASSERT_OK(hr);

			pEnum->AddRef();

			hr = pEnum->Init(
				this->GetUnknown(), m_mapDirectories[bstrDirectory]);
			if (SUCCEEDED(hr))
				hr = pEnum->QueryInterface(ppEnum);

			pEnum->Release();
		}
		return S_OK;

	case SFalseNoListing:
		return S_FALSE;

	case AbortListing:
		return E_ABORT;

	case FailListing:
		return E_FAIL;

	default:
		UNREACHABLE;
		return E_UNEXPECTED;
	}
}


STDMETHODIMP CMockSftpProvider::GetFile(BSTR bstrFilePath, IStream **ppStream)
{
	return E_NOTIMPL;
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
		Swish::Listing ltOld = {
			bstrFromPath, 0666, CComBSTR("mockowner"), 
			CComBSTR("mockgroup"), 1024, 12, COleDateTime()
		};
		Swish::Listing ltExisting =  {
			bstrToPath, 0666, CComBSTR("mockowner"), 
			CComBSTR("mockgroup"), 1024, 12, COleDateTime()
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
		UNREACHABLE;
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
void CMockSftpProvider::_FillMockListing(PCWSTR pszDirectory)
{
	// Get directory name part of path
	CString strPath(pszDirectory);
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
	vecDates.push_back(COleDateTime(100, 1, 1, 0, 00, 00));
	vecDates.push_back(COleDateTime(2007, 2, 28, 0, 0, 0));
	vecDates.push_back(COleDateTime(1752, 9, 03, 7, 27, 8));

	unsigned uCycle = 0, uSize = 0;
	while (!vecFilenames.empty())
	{
		// Try to cycle through the permissions on each successive file
		// TODO: I have no idea if this works
		unsigned uPerms = 
			(uCycle % 1) || ((uCycle % 2) << 1) || ((uCycle % 3) << 2);

		Swish::Listing lt;
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
		m_mapDirectories[pszDirectory].push_back(lt);

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
		Swish::Listing lt;
		lt.bstrFilename = vecFoldernames.back().Detach();
		lt.uPermissions = 040777;
		lt.bstrOwner =  CComBSTR("mockowner").Detach();
		lt.bstrGroup = CComBSTR("mockgroup").Detach();
		lt.uSize = 42;
		lt.cHardLinks = 7;
		lt.dateModified = COleDateTime(1582, 10, 5, 13, 54, 22);
		m_mapDirectories[pszDirectory].push_back(lt);

		vecFoldernames.pop_back();
	}
}

void CMockSftpProvider::_TestMockPathExists(PCTSTR strPath)
{
	// Find filename and directory
	CString strFile(strPath);
	strFile.TrimRight(L'/');
	int iLastSep = strFile.ReverseFind(L'/');
	CString strDirectory = strFile.Left(iLastSep+1);
	int cFilenameLen = strFile.GetLength() - (iLastSep+1);
	CString strFilename = strFile.Right(cFilenameLen);

	CPPUNIT_ASSERT_MESSAGE(
		"The requested file is in a directory which hasn't been generated. "
		"This is probably not intended.",
		m_mapDirectories.find(strDirectory) != m_mapDirectories.end()
	);
	CPPUNIT_ASSERT_MESSAGE(
		"The file was not found in the mock collection.",
		_IsInListing(strDirectory, strFilename)
	);
}

bool CMockSftpProvider::_IsInListing(PCTSTR strDirectory, PCTSTR strFilename)
{
	vector<Swish::Listing>& vecListing = m_mapDirectories[strDirectory];
	for (vector<Swish::Listing>::iterator i = vecListing.begin();
		i != vecListing.end();
		i++)
	{
		if (CComBSTR(i->bstrFilename) == strFilename)
			return true;
	}

	return false;
}
