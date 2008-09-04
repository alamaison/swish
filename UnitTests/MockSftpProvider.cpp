// Mock SftpProvider COM object implementation

#include "stdafx.h"
#include "MockSftpProvider.h"
#include <AtlComTime.h>

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

STDMETHODIMP CMockSftpProvider::GetListing(
	BSTR bstrDirectory, Swish::IEnumListing **ppEnum )
{
	// Test directory name
	CPPUNIT_ASSERT( CComBSTR(bstrDirectory).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrDirectory).Length() <= MAX_PATH_LEN );

	// Test pointer
	CPPUNIT_ASSERT( ppEnum );
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"[out] pointer must be NULL when referenced (i.e. point to NULL)",
		(Swish::IEnumListing *)NULL, *ppEnum
	);

	switch (m_enumListingBehaviour)
	{
	case MockListing: // Fallthru: listing populated in SetListingBehaviour
		_FillMockListing();
	case EmptyListing:
		{
			typedef CComEnumOnSTL<Swish::IEnumListing,
				&__uuidof(Swish::IEnumListing), Swish::Listing,
				_Copy<Swish::Listing>, vector<Swish::Listing> >
			CComEnumListing;

			// Create instance of Enum from ATL template class
			CComObject<CComEnumListing> *pEnum = NULL;
			HRESULT hr = CComObject<CComEnumListing>::CreateInstance(&pEnum);
			ATLASSERT(SUCCEEDED(hr));
			CPPUNIT_ASSERT_OK(hr);

			pEnum->AddRef();

			hr = pEnum->Init( this->GetUnknown(), m_vecListing );
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

STDMETHODIMP CMockSftpProvider::Rename(
	BSTR bstrFromFilename, BSTR bstrToFilename,
	VARIANT_BOOL *fWasTargetOverwritten )
{
	// Test filenames
	CPPUNIT_ASSERT( CComBSTR(bstrFromFilename).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrFromFilename).Length() <= MAX_FILENAME_LEN );
	CPPUNIT_ASSERT( CComBSTR(bstrToFilename).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrToFilename).Length() <= MAX_FILENAME_LEN );

	*fWasTargetOverwritten = VARIANT_FALSE;

	// Perform selected behaviour
	HRESULT hr;
	switch (m_enumRenameBehaviour)
	{
	case RenameOK:
		return S_OK;

	case ConfirmOverwrite:
		hr = m_spConsumer->OnConfirmOverwrite(bstrFromFilename, bstrToFilename);
		if (SUCCEEDED(hr))
			*fWasTargetOverwritten = VARIANT_TRUE;
		return hr;

	case ConfirmOverwriteEx:
		{
		// TODO: Lookup Listing from collection we returned in GetListing()
		Swish::Listing ltOld = {
			bstrFromFilename, 0666, CComBSTR("mockowner"), 
			CComBSTR("mockgroup"), 1024, 12, COleDateTime()
		};
		Swish::Listing ltExisting =  {
			bstrToFilename, 0666, CComBSTR("mockowner"), 
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

void CMockSftpProvider::_FillMockListing()
{
	// Fill our vector with dummy files
	vector<CComBSTR> vecFilenames;
	vecFilenames.push_back("testfile");
	vecFilenames.push_back("testFile");
	vecFilenames.push_back("testfile.ext");
	vecFilenames.push_back("testfile.txt");
	vecFilenames.push_back("testfile with spaces");
	vecFilenames.push_back("testfile with \"quotes\" and spaces");
	vecFilenames.push_back("testfile.ext.txt");
	vecFilenames.push_back("testfile..");
	vecFilenames.push_back(".testhiddenfile");

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
		m_vecListing.push_back(lt);

		vecDates.pop_back();
		vecFilenames.pop_back();
		uCycle++;
		uSize = (uSize + uCycle) << 10;
	}

	// Add some dummy folders also
	vector<CComBSTR> vecFoldernames;
	vecFoldernames.push_back("Testfolder");
	vecFoldernames.push_back("testfolder.ext");
	vecFoldernames.push_back("testfolder.bmp");
	vecFoldernames.push_back("testfolder with spaces");
	vecFoldernames.push_back(".testhiddenfolder");

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
		m_vecListing.push_back(lt);

		vecFoldernames.pop_back();
	}
}