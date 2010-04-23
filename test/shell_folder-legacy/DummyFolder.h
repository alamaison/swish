/**
    @file

    Dummy IShellFolder namespace extension to test CFolder abstract class.

    @if licence

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    @endif
*/

#pragma once

#include "swish/shell_folder/SwishFolder.hpp"  // Superclass
#include "swish/CoFactory.hpp"  // CComObject factory
#include "swish/debug.hpp"  // TRACE

#include <pshpack1.h>
struct DummyItemId
{
	USHORT cb;
	DWORD dwFingerprint;
	int level;

	static const DWORD FINGERPRINT = 0x624a0fe5;
};
#include <poppack.h>

/**
 * Copy-policy to manage copying and destruction of dummy pidls.
 */
struct _CopyPidl
{
	static HRESULT copy(PITEMID_CHILD *ppidlCopy, const PITEMID_CHILD *ppidl)
	{
		ATLASSERT(ppidl);
		ATLASSERT(ppidlCopy);

		if (*ppidl == NULL)
		{
			*ppidlCopy = NULL;
			return S_OK;
		}

		*ppidlCopy = ::ILCloneChild(*ppidl);
		if (*ppidlCopy)
			return S_OK;
		else
			return E_OUTOFMEMORY;
	}

	static void init(PITEMID_CHILD* /* ppidl */) { }
	static void destroy(PITEMID_CHILD *ppidl)
	{
		::ILFree(*ppidl);
	}
};

class ATL_NO_VTABLE CDummyFolder :
	public swish::shell_folder::folder::CSwishFolder,
	private swish::CCoFactory<CDummyFolder>
{
public:

	BEGIN_COM_MAP(CDummyFolder)
		COM_INTERFACE_ENTRY(IShellFolder)
		COM_INTERFACE_ENTRY_CHAIN(swish::shell_folder::folder::CFolder)
	END_COM_MAP()
	
	/**
	 * Create instance of the CDummyFolder class and return IShellFolder.
	 *
	 * @returns Smart pointer to the CDummyFolder's IShellFolder interface.
	 * @throws  com_exception if creation fails.
	 */
	static ATL::CComPtr<IShellFolder> Create()
	throw(...)
	{
		ATL::CComPtr<CDummyFolder> spObject = spObject->CreateCoObject();
		return spObject.p;
	}

protected:

	CLSID clsid() const;

	void validate_pidl(PCUIDLIST_RELATIVE pidl) const;
	int compare_pidls(
		PCUITEMID_CHILD pidl1, PCUITEMID_CHILD pidl2,
		int column, bool compare_all_fields, bool canonical) const;

	ATL::CComPtr<IQueryAssociations> query_associations(
		HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);

	ATL::CComPtr<IContextMenu> context_menu(
		HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);

	ATL::CComPtr<IDataObject> data_object(
		HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);

	ATL::CComPtr<IShellFolder> subfolder(
		const winapi::shell::pidl::apidl_t& root) const;

	comet::variant_t property(
		const winapi::shell::property_key& key,
		const winapi::shell::pidl::cpidl_t& pidl);
public:

	CDummyFolder();
	~CDummyFolder();

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	HRESULT FinalConstruct();
	void FinalRelease();

	__override STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidl);

public: // IShellFolder methods

	IFACEMETHODIMP ParseDisplayName(
		__in_opt HWND hwnd,
		__in_opt IBindCtx *pbc,
		__in PWSTR pwszDisplayName,
		__reserved  ULONG *pchEaten,
		__deref_out_opt PIDLIST_RELATIVE *ppidl,
		__inout_opt ULONG *pdwAttributes);

	IFACEMETHODIMP EnumObjects(
		__in_opt HWND hwnd,
		SHCONTF grfFlags,
		__deref_out_opt IEnumIDList **ppenumIDList);

	IFACEMETHODIMP GetAttributesOf( 
		UINT cidl,
		__in_ecount_opt(cidl) PCUITEMID_CHILD_ARRAY apidl,
		__inout SFGAOF *rgfInOut);

	IFACEMETHODIMP GetDisplayNameOf( 
		__in PCUITEMID_CHILD pidl,
		SHGDNF uFlags,
		__out STRRET *pName);

	IFACEMETHODIMP SetNameOf( 
		__in_opt HWND hwnd,
		__in PCUITEMID_CHILD pidl,
		__in PCWSTR pszName,
		SHGDNF uFlags,
		__deref_out_opt PITEMID_CHILD *ppidlOut);

public: // IShellFolder2 methods

	IFACEMETHODIMP GetDefaultColumn(
		DWORD dwRes,
		__out ULONG *pSort,
		__out ULONG *pDisplay);

	IFACEMETHODIMP GetDefaultColumnState(UINT iColumn, __out SHCOLSTATEF *pcsFlags);

	IFACEMETHODIMP GetDetailsOf(
		__in_opt PCUITEMID_CHILD pidl,
		UINT iColumn,
		__out SHELLDETAILS *psd);

	IFACEMETHODIMP MapColumnToSCID(
		UINT iColumn,
		__out SHCOLUMNID *pscid);

private:
	PITEMID_CHILD m_apidl[1];

	/**
	 * Static dispatcher for Default Context Menu callback
	 */
	static HRESULT __callback CALLBACK MenuCallback(
		__in_opt IShellFolder *psf, HWND hwnd, __in_opt IDataObject *pdtobj, 
		UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		ATLENSURE_RETURN(psf);
		return static_cast<CDummyFolder *>(psf)->OnMenuCallback(
			hwnd, pdtobj, uMsg, wParam, lParam);
	}

	/** @name Default context menu event handlers */
	// @{
	HRESULT OnMenuCallback( HWND hwnd, IDataObject *pdtobj, 
		UINT uMsg, WPARAM wParam, LPARAM lParam );
	HRESULT OnMergeContextMenu(
		HWND hwnd, IDataObject *pDataObj, UINT uFlags, QCMINFO& info );
	HRESULT OnInvokeCommand(
		HWND hwnd, IDataObject *pDataObj, int idCmd, PCTSTR pszArgs );
	HRESULT OnInvokeCommandEx(
		HWND hwnd, IDataObject *pDataObj, int idCmd, PDFMICS pdfmics );
	// @}

	HRESULT _GetAssocRegistryKeys( 
		__out UINT *pcKeys, __deref_out_ecount(pcKeys) HKEY **paKeys);

};
