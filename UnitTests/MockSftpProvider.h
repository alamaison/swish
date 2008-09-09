// Declaration of mock SftpProvider COM object

#pragma once

#include "stdafx.h"
#include "CppUnitExtensions.h"
#include "limits.h"

#include <vector>
using std::vector;
#include <map>
using std::map;

class ATL_NO_VTABLE CMockSftpProvider :
	public CComObjectRootEx<CComObjectThreadModel>,
	public Swish::ISftpProvider
{
public:
	
BEGIN_COM_MAP(CMockSftpProvider)
	COM_INTERFACE_ENTRY(Swish::ISftpProvider)
END_COM_MAP()

public:
	
	/**
	 * Possible behaviours of listing returned by mock GetListing() method.
	 */
	typedef enum tagListingBehaviour {
		MockListing,     ///< Return a dummy list of files and S_OK.
		EmptyListing,    ///< Return an empty list and S_OK.
		SFalseNoListing, ///< Return a NULL listing and S_FALSE.
		AbortListing,    ///< Return a NULL listing E_ABORT.
		FailListing      ///< Return a NULL listing E_FAIL.
	} ListingBehaviour;
	
	/**
	 * Possible behaviours of mock Rename() method.
	 */
	typedef enum tagRenameBehaviour {
		RenameOK,           ///< Return S_OK - rename unconditionally succeeded.
		ConfirmOverwrite,   ///< Call ISftpConsumer's OnConfirmOverwrite and
		                    ///< return its return value.
		ConfirmOverwriteEx, ///< Call ISftpConsumer's OnConfirmOverwriteEx and
		                    ///< return its return value.
		ReportError,        ///< Call ISftpConsumer's OnReportError and return
		                    ///< E_FAIL.
		AbortRename,        ///< Return E_ABORT.
		FailRename          ///< Return E_FAIL.
	} RenameBehaviour;

	CMockSftpProvider();
	void SetListingBehaviour( ListingBehaviour enumBehaviour );
	void SetRenameBehaviour( RenameBehaviour enumBehaviour );

private:
	ListingBehaviour m_enumListingBehaviour;
	RenameBehaviour m_enumRenameBehaviour;

	CComPtr<Swish::ISftpConsumer> m_spConsumer;
	map<CString, vector<Swish::Listing> > m_mapDirectories;

	CComBSTR _TagFilename(__in PCTSTR pszFilename, __in PCTSTR pszTag);
	void _FillMockListing(__in PCTSTR pszDirectory);
	void _TestMockPathExists(__in PCTSTR strPath);
	bool _IsInListing(__in PCTSTR strDirectory, __in PCTSTR strFilename);

public:

	/** @name ISftpProvider methods */
	// @{
	IFACEMETHODIMP Initialize(
		__in Swish::ISftpConsumer *pConsumer,
		__in BSTR bstrUser,
		__in BSTR bstrHost,
		__in UINT uPort
	);
	IFACEMETHODIMP GetListing(
		__in BSTR bstrDirectory,
		__out Swish::IEnumListing **ppEnum
	);
	IFACEMETHODIMP Rename(
		__in BSTR bstrFromPath,
		__in BSTR bstrToPath,
		__deref_out VARIANT_BOOL *fWasTargetOverwritten
	);
	IFACEMETHODIMP Delete(
		__in BSTR bstrPath );
	IFACEMETHODIMP DeleteDirectory(
		__in BSTR bstrPath );
	// @}

};
