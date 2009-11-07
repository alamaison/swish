/*  Dummy IShellFolder namespace extension to test CFolder abstract class.

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
*/

#include "DummyFolder.h"

#include "swish/shell_folder/Pidl.h"

#include "swish/atl.hpp"  // Common ATL setup
#include <atlstr.h>  // CString

#include <vector>

using ATL::CComObject;
using ATL::CComPtr;
using ATL::CComQIPtr;
using ATL::CString;
using ATL::CRegKey;

using std::vector;
using std::iterator;

CDummyFolder::CDummyFolder()
{
	::ZeroMemory(&m_apidl[0], sizeof m_apidl[0]);
}

CDummyFolder::~CDummyFolder()
{
	if (m_apidl[0])
		::ILFree(m_apidl[0]);
}

HRESULT CDummyFolder::FinalConstruct()
{
	return S_OK;
}

void CDummyFolder::FinalRelease()
{
}

STDMETHODIMP CDummyFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
	FUNCTION_TRACE;

	HRESULT hr = __super::Initialize(pidl);

	if (SUCCEEDED(hr))
	{
		int level;
		try
		{
			// If the last item is of our type, interpret it in order to
			// get next level, otherwise default to a level of 0
			PCUITEMID_CHILD pidlEnd = ::ILFindLastID(pidl);

			validate_pidl(pidlEnd);

			level = (reinterpret_cast<const DummyItemId *>(pidlEnd)->level) + 1;
		}
		catch (...)
		{
			level = 0;
		}

		if (m_apidl[0])
			::ILFree(m_apidl[0]);

		size_t nDummyPIDLSize = sizeof DummyItemId + sizeof USHORT;
		m_apidl[0] = (PITEMID_CHILD)CoTaskMemAlloc(nDummyPIDLSize);
		::ZeroMemory(m_apidl[0], nDummyPIDLSize);

		// Use first PIDL member as a DummyItemId structure
		DummyItemId *pidlDummy = (DummyItemId *)m_apidl[0];

		// Fill members of the PIDL with data
		pidlDummy->cb = sizeof DummyItemId;
		pidlDummy->dwFingerprint = DummyItemId::FINGERPRINT; // Sign with fingerprint
		pidlDummy->level = level;
	}

	return hr;
}

CLSID CDummyFolder::clsid() const
{
	// {708F09A0-FED0-46E8-9C56-55B7AA6AD1B2}
	static const GUID dummyguid = {
		0x708F09A0, 0xFED0, 0x46E8, {
			0x9C, 0x56, 0x55, 0xB7, 0xAA, 0x6A, 0xD1, 0xB2 } };

	return dummyguid;
}

void CDummyFolder::validate_pidl(PCUIDLIST_RELATIVE pidl) const
{
	if (pidl == NULL)
		AtlThrow(E_POINTER);

	const DummyItemId *pitemid = reinterpret_cast<const DummyItemId *>(pidl);

	if (pitemid->cb != sizeof DummyItemId ||
		pitemid->dwFingerprint != DummyItemId::FINGERPRINT)
		AtlThrow(E_INVALIDARG);
}

/**
 * Create and initialise new folder object for subfolder.
 */
CComPtr<IShellFolder> CDummyFolder::subfolder(PCIDLIST_ABSOLUTE pidlRoot)
const
{
	HRESULT hr;

	// Create and initialise new folder object for subfolder
	CComPtr<CDummyFolder> spDummyFolder = spDummyFolder->CreateCoObject();
	hr = spDummyFolder->Initialize(pidlRoot);
	ATLENSURE_THROW(SUCCEEDED(hr), hr);

	CComQIPtr<IShellFolder> spFolder = spDummyFolder;
	ATLENSURE_THROW(spFolder, E_NOINTERFACE);

	return spFolder;
}

STDMETHODIMP CDummyFolder::ParseDisplayName(
	HWND /*hwnd*/, IBindCtx * /*pbc*/, PWSTR pwszDisplayName, ULONG *pchEaten, 
	PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pwszDisplayName, E_POINTER);
	ATLENSURE_RETURN_HR(*pwszDisplayName != L'\0', E_INVALIDARG);
	ATLENSURE_RETURN_HR(ppidl, E_POINTER);

	if (pchEaten)
		*pchEaten = 0;
	*ppidl = NULL;
	if (pdwAttributes)
		*pdwAttributes = 0;

	ATLTRACENOTIMPL(__FUNCTION__);
}

STDMETHODIMP CDummyFolder::EnumObjects(
	HWND /*hwnd*/, SHCONTF grfFlags, IEnumIDList **ppenumIDList)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(ppenumIDList, E_POINTER);

	*ppenumIDList = NULL;

	if (!(grfFlags & SHCONTF_FOLDERS))
		return S_FALSE;

	typedef ATL::CComEnum<IEnumIDList, &__uuidof(IEnumIDList), PITEMID_CHILD,
		                  _CopyPidl>
	        CComEnumIDList;

	CComObject<CComEnumIDList> *pEnum = NULL;
	HRESULT hr = pEnum->CreateInstance(&pEnum);
	if (SUCCEEDED(hr))
	{
		pEnum->AddRef();

		hr = pEnum->Init(&m_apidl[0], &m_apidl[1], GetUnknown());
		ATLASSERT(SUCCEEDED(hr));

		hr = pEnum->QueryInterface(ppenumIDList);
		ATLASSERT(SUCCEEDED(hr));

		pEnum->Release();
	}

	return hr;
}

/**
 * Determine the relative order of two file objects or folders.
 *
 * @implementing CFolder
 *
 * Given their item identifier lists, compare the two objects and return a value
 * in the HRESULT indicating the result of the comparison:
 * - Negative: pidl1 < pidl2
 * - Positive: pidl1 > pidl2
 * - Zero:     pidl1 == pidl2
 */
int CDummyFolder::compare_pidls(
	PCUITEMID_CHILD pidl1, PCUITEMID_CHILD pidl2, int /*column*/,
	bool /*compare_all_fields*/, bool /*canonical*/) const
{
	const DummyItemId *pitemid1 = reinterpret_cast<const DummyItemId *>(pidl1);
	const DummyItemId *pitemid2 = reinterpret_cast<const DummyItemId *>(pidl2);
	return pitemid1->level - pitemid2->level;
}

STDMETHODIMP CDummyFolder::GetAttributesOf( 
	UINT /*cpidl*/, PCUITEMID_CHILD_ARRAY apidl, SFGAOF *rgfInOut)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(apidl, E_POINTER);
	ATLENSURE_RETURN_HR(rgfInOut, E_POINTER);

	SFGAOF rgfRequested = *rgfInOut;
	*rgfInOut = 0;

	if (rgfRequested & SFGAO_FOLDER)
		*rgfInOut |= SFGAO_FOLDER;
	if (rgfRequested & SFGAO_HASSUBFOLDER)
		*rgfInOut |= SFGAO_HASSUBFOLDER;
	if (rgfRequested & SFGAO_FILESYSANCESTOR)
		*rgfInOut |= SFGAO_FILESYSANCESTOR;
	if (rgfRequested & SFGAO_BROWSABLE)
		*rgfInOut |= SFGAO_BROWSABLE;

	return S_OK;
}

CComPtr<IQueryAssociations> CDummyFolder::query_associations(
	HWND /*hwnd*/, UINT /*cpidl*/, PCUITEMID_CHILD_ARRAY /*apidl*/)
{
	TRACE("Request: IQueryAssociations");

	CComPtr<IQueryAssociations> spAssoc;
	HRESULT hr = ::AssocCreate(
		CLSID_QueryAssociations, IID_PPV_ARGS(&spAssoc));
	ATLENSURE_SUCCEEDED(hr);
	
	// Initialise default assoc provider for Folders
	hr = spAssoc->Init(0, L"Folder", NULL, NULL);
	ATLENSURE_SUCCEEDED(hr);

	return spAssoc;
}

CComPtr<IContextMenu> CDummyFolder::context_menu(
	HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl)
{
	TRACE("Request: IContextMenu");

	// Get keys associated with filetype from registry.
	HKEY *akeys; UINT ckeys;
	ATLENSURE_THROW(SUCCEEDED(
		_GetAssocRegistryKeys(&ckeys, &akeys)),
		E_UNEXPECTED  // Might fail if registry is corrupted
	);

	CComPtr<IShellFolder> spThisFolder = this;
	ATLENSURE_THROW(spThisFolder, E_OUTOFMEMORY);

	// Create default context menu from list of PIDLs
	CComPtr<IContextMenu> spMenu;
	HRESULT hr = ::CDefFolderMenu_Create2(
		root_pidl(), hwnd, cpidl, apidl, spThisFolder, 
		MenuCallback, ckeys, akeys, &spMenu);
	ATLENSURE_SUCCEEDED(hr);

	return spMenu;
}

CComPtr<IDataObject> CDummyFolder::data_object(
	HWND /*hwnd*/, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl)
{
	TRACE("Request: IDataObject");

	// A DataObject is required in order for the call to 
	// CDefFolderMenu_Create2 (above) to succeed on versions of Windows
	// earlier than Vista

	CComPtr<IDataObject> spdo;
	HRESULT hr = ::CIDLData_CreateFromIDArray(
		root_pidl(), cpidl, 
		reinterpret_cast<PCUIDLIST_RELATIVE_ARRAY>(apidl), &spdo);
	ATLENSURE_SUCCEEDED(hr);

	return spdo;
}

STDMETHODIMP CDummyFolder::GetDisplayNameOf( 
	PCUITEMID_CHILD pidl, SHGDNF /*uFlags*/, STRRET *pName)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pidl, E_POINTER);
	ATLENSURE_RETURN_HR(pName, E_POINTER);

	::ZeroMemory(pName, sizeof *pName);

	const DummyItemId *pitemid = reinterpret_cast<const DummyItemId *>(pidl);

	CString strName;
	strName.AppendFormat(L"Level %d", pitemid->level);
	
	// Store in a STRRET and return
	pName->uType = STRRET_WSTR;
	return ::SHStrDup(strName, &(pName->pOleStr));
}

STDMETHODIMP CDummyFolder::SetNameOf( 
	HWND /*hwnd*/, PCUITEMID_CHILD pidl, PCWSTR pwszName, SHGDNF /*uFlags*/, 
	PITEMID_CHILD *ppidlOut)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pidl, E_POINTER);
	ATLENSURE_RETURN_HR(pwszName, E_POINTER);
	ATLENSURE_RETURN_HR(*pwszName != L'\0', E_INVALIDARG);
	ATLENSURE_RETURN_HR(ppidlOut, E_POINTER);

	*ppidlOut = NULL;

	ATLTRACENOTIMPL(__FUNCTION__);
}

STDMETHODIMP CDummyFolder::GetDefaultColumn(
	DWORD /*dwRes*/, ULONG *pSort, ULONG *pDisplay)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pSort, E_POINTER);
	ATLENSURE_RETURN_HR(pDisplay, E_POINTER);

	*pSort = 0;
	*pDisplay = 0;

	return S_OK;
}

STDMETHODIMP CDummyFolder::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pcsFlags, E_POINTER);

	*pcsFlags = 0;

	if (iColumn != 0)
		return E_FAIL;

	*pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;

	return S_OK;
}

STDMETHODIMP CDummyFolder::GetDetailsOf(
	PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(psd, E_POINTER);

	::ZeroMemory(psd, sizeof SHELLDETAILS);

	if (iColumn != 0)
		return E_FAIL;

	CString str;

	if (pidl == NULL) // Wants header
	{
		str = L"Name";
	}
	else              // Wants contents
	{
		const DummyItemId *pitemid = reinterpret_cast<const DummyItemId *>(pidl);
		str.AppendFormat(L"Level %d", pitemid->level);
	}
	
	psd->fmt = LVCFMT_LEFT;
	psd->cxChar = 4;

	// Store in STRRET and return
	psd->str.uType = STRRET_WSTR;
	return ::SHStrDup(str, &(psd->str.pOleStr));
}

STDMETHODIMP CDummyFolder::GetDetailsEx(
	PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pidl, E_POINTER);
	ATLENSURE_RETURN_HR(pscid, E_POINTER);
	ATLENSURE_RETURN_HR(pv, E_POINTER);

	//::VariantClear(pv);

	ATLTRACENOTIMPL(__FUNCTION__);
}

STDMETHODIMP CDummyFolder::MapColumnToSCID(UINT /*iColumn*/, SHCOLUMNID *pscid)
{
	FUNCTION_TRACE;
	ATLENSURE_RETURN_HR(pscid, E_POINTER);

	::ZeroMemory(pscid, sizeof *pscid);

	ATLTRACENOTIMPL(__FUNCTION__);
}



/**
 * Cracks open the @c DFM_* callback messages and dispatched them to handlers.
 */
HRESULT CDummyFolder::OnMenuCallback(
	HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	ATLTRACE(__FUNCTION__" called (uMsg=%d)\n", uMsg);
	UNREFERENCED_PARAMETER(hwnd);

	switch (uMsg)
	{
	case DFM_MERGECONTEXTMENU:
		return this->OnMergeContextMenu(
			hwnd,
			pdtobj,
			static_cast<UINT>(wParam),
			*reinterpret_cast<QCMINFO *>(lParam)
		);
	case DFM_INVOKECOMMAND:
		return this->OnInvokeCommand(
			hwnd,
			pdtobj,
			static_cast<int>(wParam),
			reinterpret_cast<PCWSTR>(lParam)
		);
	case DFM_INVOKECOMMANDEX:
		return this->OnInvokeCommandEx(
			hwnd,
			pdtobj,
			static_cast<int>(wParam),
			reinterpret_cast<PDFMICS>(lParam)
		);
	default:
		return E_NOTIMPL;
	}
}

/**
 * Handle @c DFM_MERGECONTEXTMENU callback.
 */
HRESULT CDummyFolder::OnMergeContextMenu(
	HWND hwnd, IDataObject *pDataObj, UINT uFlags, QCMINFO& info )
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(pDataObj);
	UNREFERENCED_PARAMETER(uFlags);
	UNREFERENCED_PARAMETER(info);

	// It seems we have to return S_OK even if we do nothing else or Explorer
	// won't put Open as the default item and in the right order
	return S_OK;
}

/**
 * Handle @c DFM_INVOKECOMMAND callback.
 */
HRESULT CDummyFolder::OnInvokeCommand(
	HWND hwnd, IDataObject *pDataObj, int idCmd, PCWSTR pszArgs )
{
	ATLTRACE(__FUNCTION__" called (hwnd=%p, pDataObj=%p, idCmd=%d, "
		"pszArgs=%ls)\n", hwnd, pDataObj, idCmd, pszArgs);
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(pDataObj);
	UNREFERENCED_PARAMETER(idCmd);
	UNREFERENCED_PARAMETER(pszArgs);

	return S_FALSE;
}

/**
 * Handle @c DFM_INVOKECOMMANDEX callback.
 */
HRESULT CDummyFolder::OnInvokeCommandEx(
	HWND /*hwnd*/, IDataObject *pDataObj, int idCmd, PDFMICS pdfmics )
{
	ATLTRACE(__FUNCTION__" called (pDataObj=%p, idCmd=%d, pdfmics=%p)\n",
		pDataObj, idCmd, pdfmics);
	UNREFERENCED_PARAMETER(pDataObj);
	UNREFERENCED_PARAMETER(idCmd);
	UNREFERENCED_PARAMETER(pdfmics);

	return S_FALSE;
}

/**
 * Get a list names of registry keys for the dummy folders.  This is not 
 * required for Windows Vista but is necessary on any earlier version in
 * order to display the default context menu.
 * The list of keys is:  
 *   HKCU\Directory
 *   HKCU\Directory\Background
 *   HKCU\Folder
 *   HKCU\*
 *   HKCU\AllFileSystemObjects
 */
HRESULT CDummyFolder::_GetAssocRegistryKeys(UINT *pcKeys, HKEY **paKeys)
{
	LSTATUS rc = ERROR_SUCCESS;	
	vector<CString> vecKeynames;

	// Add directory-specific items
	vecKeynames.push_back(_T("Directory"));
	vecKeynames.push_back(_T("Directory\\Background"));
	vecKeynames.push_back(_T("Folder"));

	// Add names of keys that apply to items of all types
	vecKeynames.push_back(_T("AllFilesystemObjects"));
	vecKeynames.push_back(_T("*"));

	// Create list of registry handles from list of keys
	vector<HKEY> vecKeys;
	for (UINT i = 0; i < vecKeynames.size(); i++)
	{
		CRegKey reg;
		CString strKey = vecKeynames[i];
		ATLASSERT(strKey.GetLength() > 0);
		ATLTRACE(_T("Opening HKCR\\%s\n"), strKey);
		rc = reg.Open(HKEY_CLASSES_ROOT, strKey, KEY_READ);
		ATLASSERT(rc == ERROR_SUCCESS);
		if (rc == ERROR_SUCCESS)
			vecKeys.push_back(reg.Detach());
	}
	
	ATLASSERT( vecKeys.size() >= 3 );  // The minimum we must have added here
	ATLASSERT( vecKeys.size() <= 16 ); // CDefFolderMenu_Create2's maximum

	HKEY *aKeys = (HKEY *)::SHAlloc(vecKeys.size() * sizeof HKEY); 
	for (UINT i = 0; i < vecKeys.size(); i++)
		aKeys[i] = vecKeys[i];

	*pcKeys = (UINT)vecKeys.size();
	*paKeys = aKeys;

	return S_OK;
}

