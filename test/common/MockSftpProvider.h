/**
 * @file
 *
 * Mock ISftpProvider COM object for testing without using the network.
 */

#pragma once

#include "CppUnitExtensions.h"
#include "testlimits.h"

#include "tree.h"  // N-ary tree container for mocking a filesystem

#include "swish/atl.hpp"  // Common ATL setup

#include "swish/interfaces/SftpProvider.h"  // ISftpProvider/Consumer

#include <vector>
#include <string>
#include <algorithm>
#include <functional>

class ATL_NO_VTABLE CMockSftpProvider :
	public ATL::CComObjectRootEx<ATL::CComObjectThreadModel>,
	public ISftpProvider
{
public:
	
BEGIN_COM_MAP(CMockSftpProvider)
	COM_INTERFACE_ENTRY(ISftpProvider)
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

	/** @name Filesystem
	 * Mock filesystem holding dummy file heirarchy as an n-ary tree.
	 */
	// @{
	typedef std::wstring Filename;
	typedef Listing FilesystemItem;
	typedef tree<FilesystemItem> Filesystem;
	typedef Filesystem::iterator FilesystemLocation;

	struct eq_item : 
		public std::binary_function<FilesystemItem, std::wstring, bool>
	{
		bool operator()(
			const FilesystemItem& item, const std::wstring& name)
		const
		{
			return std::wstring(item.bstrFilename) == name;
		}
	};

	struct lt_item : public std::less<FilesystemItem>
	{
		bool operator()(
			const FilesystemItem& left, const FilesystemItem& right)
		const
		{
			return std::wstring(left.bstrFilename) < 
				std::wstring(right.bstrFilename);
		}
	};
	
	Filesystem m_filesystem;

	Listing _MakeDirectoryItem(PCWSTR pwszName);

	void _MakeItemIn(FilesystemLocation loc, const Listing& item);
	void _MakeItemIn(const std::wstring& path, const Listing& item);

	std::vector<std::wstring> _TokenisePath(const std::wstring& path);
	FilesystemLocation _FindLocationFromPath(const std::wstring& path);
	// @}

	ATL::CComBSTR _TagFilename(__in PCWSTR pszFilename, __in PCWSTR pszTag);
	void _FillMockListing(__in PCWSTR pwszDirectory);
	void _TestMockPathExists(__in PCWSTR pwszPath);

public:

	/** @name ISftpProvider methods */
	// @{
	IFACEMETHODIMP Initialize(
		__in BSTR bstrUser,
		__in BSTR bstrHost,
		__in UINT uPort
	);
	IFACEMETHODIMP GetListing(
		__in ISftpConsumer* pConsumer,
		__in BSTR bstrDirectory,
		__out IEnumListing **ppEnum
	);
	IFACEMETHODIMP GetFile(
		__in ISftpConsumer* pConsumer,
		__in BSTR bstrFilePath,
		__in BOOL fWriteable,
		__out IStream **ppStream
	);
	IFACEMETHODIMP Rename(
		__in ISftpConsumer* pConsumer,
		__in BSTR bstrFromPath,
		__in BSTR bstrToPath,
		__deref_out VARIANT_BOOL *fWasTargetOverwritten
	);
	IFACEMETHODIMP Delete(
		__in ISftpConsumer* pConsumer,
		__in BSTR bstrPath );
	IFACEMETHODIMP DeleteDirectory(
		__in ISftpConsumer* pConsumer,
		__in BSTR bstrPath );
	IFACEMETHODIMP CreateNewFile(
		__in ISftpConsumer* pConsumer,
		__in BSTR bstrPath );
	IFACEMETHODIMP CreateNewDirectory(
		__in ISftpConsumer* pConsumer,
		__in BSTR bstrPath );
	// @}

};


/**
 * Copy-policy for use by enumerators of Listing items.
 */
template<>
class ATL::_Copy<Listing>
{
public:
	static HRESULT copy(Listing* p1, const Listing* p2)
	{
		p1->bstrFilename = SysAllocStringLen(
			p2->bstrFilename, ::SysStringLen(p2->bstrFilename));
		p1->uPermissions = p2->uPermissions;
		p1->bstrOwner = SysAllocStringLen(
			p2->bstrOwner, ::SysStringLen(p2->bstrOwner));
		p1->bstrGroup = SysAllocStringLen(
			p2->bstrGroup, ::SysStringLen(p2->bstrGroup));
		p1->uUid = p2->uUid;
		p1->uGid = p2->uGid;
		p1->uSize = p2->uSize;
		p1->cHardLinks = p2->cHardLinks;
		p1->dateModified = p2->dateModified;
		p1->dateAccessed = p2->dateAccessed;

		return S_OK;
	}
	static void init(Listing* p)
	{
		::ZeroMemory(p, sizeof(Listing));
	}
	static void destroy(Listing* p)
	{
		::SysFreeString(p->bstrFilename);
		::SysFreeString(p->bstrOwner);
		::SysFreeString(p->bstrGroup);
		::ZeroMemory(p, sizeof(Listing));
	}
};
	
typedef ATL::CComEnum<IEnumListing, &__uuidof(IEnumListing),
	Listing, ATL::_Copy<Listing> >
	CMockEnumListing;