// Declaration of mock SftpProvider COM object

#pragma once

#include "stdafx.h"
#include "CppUnitExtensions.h"
#include "limits.h"

#include <vector>
using std::vector;

#include <string>
using std::wstring;

#include <algorithm>
using std::find_if;
using std::sort;

#include <functional>
using std::binary_function;
using std::bind2nd;
using std::less;

#include "tree.h"

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

	/** @name Filesystem
	 * Mock filesystem holding dummy file heirarchy as an n-ary tree.
	 */
	// @{
	typedef wstring Filename;
	typedef Swish::Listing FilesystemItem;
	typedef tree<FilesystemItem> Filesystem;
	typedef Filesystem::iterator FilesystemLocation;

	struct eq_item : 
		public binary_function<FilesystemItem, wstring, bool>
	{
		bool operator()(const FilesystemItem& item, const wstring& name)
		const
		{
			return wstring(item.bstrFilename) == name;
		}
	};

	struct lt_item : public std::less<FilesystemItem>
	{
		bool operator()(
			const FilesystemItem& left, const FilesystemItem& right)
		const
		{
			return wstring(left.bstrFilename) < wstring(right.bstrFilename);
		}
	};
	
	Filesystem m_filesystem;

	Swish::Listing _MakeDirectoryItem(PCWSTR pwszName);

	void _MakeItemIn(FilesystemLocation loc, const Swish::Listing& item);
	void _MakeItemIn(const wstring& path, const Swish::Listing& item);

	vector<wstring> _TokenisePath(const wstring& path);
	FilesystemLocation _FindLocationFromPath(const wstring& path);
	// @}

	CComBSTR _TagFilename(__in PCTSTR pszFilename, __in PCTSTR pszTag);
	void _FillMockListing(__in PCWSTR pwszDirectory);
	void _TestMockPathExists(__in PCWSTR pwszPath);

public:

	/** @name ISftpProvider methods */
	// @{
	IFACEMETHODIMP Initialize(
		__in Swish::ISftpConsumer *pConsumer,
		__in BSTR bstrUser,
		__in BSTR bstrHost,
		__in UINT uPort
	);
	IFACEMETHODIMP SwitchConsumer (
		__in Swish::ISftpConsumer *pConsumer
	);
	IFACEMETHODIMP GetListing(
		__in BSTR bstrDirectory,
		__out Swish::IEnumListing **ppEnum
	);
	IFACEMETHODIMP GetFile(
		__in BSTR bstrFilePath,
		__out IStream **ppStream
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
	IFACEMETHODIMP CreateNewFile(
		__in BSTR bstrPath );
	IFACEMETHODIMP CreateNewDirectory(
		__in BSTR bstrPath );
	// @}

};


/**
 * Copy-policy for use by enumerators of Listing items.
 */
template<>
class _Copy<Swish::Listing>
{
public:
	static HRESULT copy(Swish::Listing* p1, const Swish::Listing* p2)
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
	static void init(Swish::Listing* p)
	{
		::ZeroMemory(p, sizeof(Swish::Listing));
	}
	static void destroy(Swish::Listing* p)
	{
		::SysFreeString(p->bstrFilename);
		::SysFreeString(p->bstrOwner);
		::SysFreeString(p->bstrGroup);
		::ZeroMemory(p, sizeof(Swish::Listing));
	}
};
	
typedef CComEnum<Swish::IEnumListing, &__uuidof(Swish::IEnumListing),
	Swish::Listing, _Copy<Swish::Listing> >
	CMockEnumListing;