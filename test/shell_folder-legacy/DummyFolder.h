/**
    @file

    Dummy IShellFolder namespace extension to test CFolder abstract class.

    @if license

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

#include "swish/atl.hpp"  // Common ATL setup
#include <atlstr.h>  // CString

#include "swish/shell_folder/SwishFolder.hpp"  // Superclass
#include "swish/CoFactory.hpp"  // CComObject factory
#include "swish/debug.hpp"  // TRACE

#include <stdexcept> // range_error

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

/**
 * Minimal class handling folder details.
 */
class DummyColumn
{
public:
	DummyColumn(size_t index)
	{
		if (index != 0)
			throw std::range_error("Column out-of-range");
	}

	std::wstring header() const { return L"Name"; }

	std::wstring detail(const winapi::shell::pidl::cpidl_t& pidl) const
	{
		const DummyItemId *pitemid =
			reinterpret_cast<const DummyItemId *>(pidl.get());
		ATL::CString str;
		str.AppendFormat(L"Level %d", pitemid->level);
		return str.GetString();
	}

	int average_width_in_chars() const { return 4; }

	SHCOLSTATEF state() const
	{ return SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT; }

	int format() const { return LVCFMT_LEFT; }

	/**
	 * Determine the relative order of two file objects or folders.
	 *
	 * @implementing CFolder
	 *
	 * Given their item identifier lists, compare the two objects and return
	 * a value in the HRESULT indicating the result of the comparison:
	 * - Negative: pidl1 < pidl2
	 * - Positive: pidl1 > pidl2
	 * - Zero:     pidl1 == pidl2
	 */
	int compare(
		const winapi::shell::pidl::cpidl_t& lhs,
		const winapi::shell::pidl::cpidl_t& rhs) const
	{
		const DummyItemId *pitemid1 =
			reinterpret_cast<const DummyItemId *>(lhs.get());
		const DummyItemId *pitemid2 =
			reinterpret_cast<const DummyItemId *>(rhs.get());
		return pitemid1->level - pitemid2->level;	
	}
};

class ATL_NO_VTABLE CDummyFolder :
	public swish::shell_folder::folder::CSwishFolder<DummyColumn>,
	private swish::CCoFactory<CDummyFolder>
{
public:

	BEGIN_COM_MAP(CDummyFolder)
		COM_INTERFACE_ENTRY(IShellFolder)
		COM_INTERFACE_ENTRY_CHAIN(CSwishFolder)
	END_COM_MAP()
	
	/**
	 * Create instance of the CDummyFolder class and return IShellFolder.
	 *
	 * @returns Smart pointer to the CDummyFolder's IShellFolder interface.
	 * @throws  com_error if creation fails.
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

public:

	// IShellFolder (via folder_error_adapter)
	virtual IEnumIDList* enum_objects(HWND hwnd, SHCONTF flags);
	
	virtual void get_attributes_of(
		UINT pidl_count, PCUITEMID_CHILD_ARRAY pidl_array,
		SFGAOF* flags_inout);

	virtual STRRET get_display_name_of(
		PCUITEMID_CHILD pidl, SHGDNF uFlags);

	virtual PIDLIST_RELATIVE parse_display_name(
		HWND hwnd, IBindCtx* bind_ctx, const wchar_t* display_name,
		ULONG* attributes_inout);

	virtual PITEMID_CHILD set_name_of(
		HWND hwnd, PCUITEMID_CHILD pidl, const wchar_t* name,
		SHGDNF flags);

	// IShellFolder2 (via folder_error_adapter2)
	virtual SHCOLUMNID map_column_to_scid(UINT column_index);

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
