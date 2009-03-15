/*  Base class for IShellFolder implementations.

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

#include "pch.h"
#include "Folder.h"

#include "debug.hpp"
#include "catch_com.hpp"

using ATL::CComPtr;

CFolder::CFolder() : m_pidlRoot(NULL)
{
}

CFolder::~CFolder()
{
	if (m_pidlRoot)
	{
		::ILFree(m_pidlRoot);
		m_pidlRoot = NULL;
	}
}

CAbsolutePidl CFolder::CloneRootPIDL()
const throw(...)
{
	return GetRootPIDL();
}

PCIDLIST_ABSOLUTE CFolder::GetRootPIDL()
const
{
	return m_pidlRoot;
}

CComPtr<IShellFolderViewCB> CFolder::GetFolderViewCallback()
const throw(...)
{
	return NULL;
}

/**
 * Get the class identifier (CLSID) of the object.
 * 
 * @implementing IPersist
 *
 * @param[in] pClassID  Location in which to return the CLSID.
 */
STDMETHODIMP CFolder::GetClassID(CLSID *pClassID)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(pClassID, E_POINTER);

	*pClassID = GetCLSID();

    return S_OK;
}

/**
 * Assign an @b absolute PIDL to be the root of this folder.
 *
 * @implementing IPersistFolder
 *
 * This function tells a folder its place in the system namespace. 
 * If the folder implementation needs to construct a fully qualified PIDL
 * to elements that it contains, the PIDL passed to this method is 
 * used to construct these.
 *
 * @param pidl  PIDL that specifies the absolute location of this folder.
 */
STDMETHODIMP CFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(!::ILIsEmpty(pidl), E_INVALIDARG);
	ATLENSURE_RETURN_HR(m_pidlRoot == NULL, E_UNEXPECTED); // Multiple init

	m_pidlRoot = ::ILCloneFull(pidl);
	ATLENSURE_RETURN_HR(!::ILIsEmpty(m_pidlRoot), E_OUTOFMEMORY);

	return S_OK;
}


/**
 * Get the root of this folder - the absolute PIDL relative to the desktop.
 *
 * @implementing IPersistFolder2
 *
 * @param[out] ppidl  Location in which to return the copied PIDL.  If the folder
 *                    hasn't been initialised yet, this value will be NULL.
 *
 * @returns  S_FALSE if Initialize() hasn't been called.
 */
STDMETHODIMP CFolder::GetCurFolder(PIDLIST_ABSOLUTE *ppidl)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(ppidl, E_POINTER);

	*ppidl = NULL;

	if (m_pidlRoot == NULL) // Legal to call this before Initialize()
		return S_FALSE;

	// Copy the PIDL that was passed to us in Initialize()
	*ppidl = ::ILCloneFull(m_pidlRoot);
	ATLENSURE_RETURN_HR(*ppidl, E_OUTOFMEMORY);
	
	return S_OK;
}

STDMETHODIMP CFolder::InitializeEx(
	IBindCtx *pbc, PCIDLIST_ABSOLUTE pidlRoot, 
	const PERSIST_FOLDER_TARGET_INFO *ppfti)
{
	METHOD_TRACE;
	UNREFERENCED_PARAMETER(pbc);
	UNREFERENCED_PARAMETER(ppfti);
	ATLENSURE_RETURN_HR(pidlRoot, E_POINTER);

	return Initialize(pidlRoot);
}
	
STDMETHODIMP CFolder::GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO *ppfti)
{
	ATLENSURE_RETURN_HR(ppfti, E_POINTER);

	::ZeroMemory(ppfti, sizeof PERSIST_FOLDER_TARGET_INFO);

	ATLTRACENOTIMPL(__FUNCTION__);
}

STDMETHODIMP CFolder::SetIDList(PCIDLIST_ABSOLUTE pidl)
{
	METHOD_TRACE;
	return Initialize(pidl);
}
	
STDMETHODIMP CFolder::GetIDList(PIDLIST_ABSOLUTE *ppidl)
{
	METHOD_TRACE;
	return GetCurFolder(ppidl);
}

STDMETHODIMP CFolder::BindToStorage( 
	PCUIDLIST_RELATIVE pidl, IBindCtx *pbc, REFIID riid, void **ppv)
{
	METHOD_TRACE;
	UNREFERENCED_PARAMETER(pbc);
	UNREFERENCED_PARAMETER(riid);
	ATLENSURE_RETURN_HR(pidl, E_POINTER);
	ATLENSURE_RETURN_HR(ppv, E_POINTER);

	*ppv = NULL;

	ATLTRACENOTIMPL(__FUNCTION__);
}


/**
 * Subobject of the folder has been requested (typically IShellFolder).
 *
 * @implementing IShellFolder
 *
 * Create and initialise an instance of the subfolder represented by @a pidl
 * and return the interface asked for in @a riid.
 *
 * @param[in]  pidl  PIDL to the requested object @b relative to this folder.
 * @param[in]  pbc   Binding context (ignored).
 * @param[in]  riid  IID of the interface being requested.
 * @param[out] ppv   Location in which to return the requested interface.
 */
STDMETHODIMP CFolder::BindToObject( 
	PCUIDLIST_RELATIVE pidl, IBindCtx *pbc, REFIID riid, void **ppv)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(!::ILIsEmpty(pidl), E_INVALIDARG);
	ATLENSURE_RETURN_HR(ppv, E_POINTER);

	*ppv = NULL;

	// TODO: We can optimise this function by immediately returning E_NOTIMPL 
	// for any riid that we know our children and grandchildren don't provide.
	// This is not in the spirit of COM QueryInterface but it may be a 
	// performance boost.

	try
	{
		// First item in pidl must be of our type
		ValidatePidl(pidl);

		if (::ILIsChild(pidl)) // Our child subfolder is the target
		{
			// Create absolute PIDL to the subfolder by combining with our root
			CAbsolutePidl pidlNewRoot;
			pidlNewRoot.Attach(::ILCombine(m_pidlRoot, pidl));
			ATLENSURE_RETURN_HR(pidlNewRoot, E_OUTOFMEMORY);

			CComPtr<IShellFolder> spFolder = CreateSubfolder(pidlNewRoot);

			return spFolder->QueryInterface(riid, ppv);
		}
		else // One of our grandchildren is the target - delegate to its parent
		{
			CComPtr<IShellFolder> spFolder;

			PCUITEMID_CHILD pidlGrandchild = NULL;
			HRESULT hr = _BindToParentFolderOfPIDL(
				this, pidl, IID_PPV_ARGS(&spFolder), &pidlGrandchild);
			ATLENSURE_SUCCEEDED(hr);

			return spFolder->BindToObject(pidlGrandchild, pbc, riid, ppv);
		}
	}
	catchCom()
}

/**
 * Determine the relative order of two file objects or folders.
 *
 * @implementing IShellFolder
 *
 * Given their item identifier lists, compare the two objects and return a value
 * in the HRESULT indicating the result of the comparison:
 * - Negative: pidl1 < pidl2
 * - Positive: pidl1 > pidl2
 * - Zero:     pidl1 == pidl2
 */
STDMETHODIMP CFolder::CompareIDs( 
	LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
	METHOD_TRACE;

	USHORT uColumn = LOWORD(lParam);
	bool fCompareAllFields = (HIWORD(lParam) == SHCIDS_ALLFIELDS);
	bool fCanonical = (HIWORD(lParam) == SHCIDS_CANONICALONLY);
	ATLASSERT(!fCompareAllFields || uColumn == 0);
	ATLASSERT(!fCanonical || !fCompareAllFields);

	try
	{
		// Handles empty PIDL cases
		if (::ILIsEmpty(pidl1) && ::ILIsEmpty(pidl2))
			return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);  // Both empty - equal
		else if (::ILIsEmpty(pidl1))
			return MAKE_HRESULT(SEVERITY_SUCCESS, 0, -1); // Only pidl1 empty <
		else if (::ILIsEmpty(pidl2))
			return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);  // Only pidl2 empty >
		
		// Explorer can pass us invalid PIDLs from its cache if our PIDL 
		// representation changes.  We catch that here to stop us asserting later.
		ValidatePidl(pidl1);
		ValidatePidl(pidl2);

		// The casts here are OK.  We are aware ComparePIDLs only compares a
		// single item.  We recurse later if needed.
		int result = ComparePIDLs(
			static_cast<PCUITEMID_CHILD>(pidl1),
			static_cast<PCUITEMID_CHILD>(pidl2),
			uColumn, fCompareAllFields, fCanonical);

		if ((::ILIsChild(pidl1) && ::ILIsChild(pidl2)) || result != 0)
		{
			// The cast to unsigned short is *crucial*!  Without it, sorting
			// in Explorer does all sorts of wierd stuff
			return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (unsigned short)result);
		}
		else
		{
			// The first items are equal and there are more items to compare.
			// Delegate the rest of the comparison to our child.

			CComPtr<IShellFolder> spFolder;

			PITEMID_CHILD pidlChild = ::ILCloneFirst(pidl1);		
			HRESULT hr = BindToObject(
				pidlChild, NULL, IID_PPV_ARGS(&spFolder));
			::ILFree(pidlChild);
			ATLENSURE_SUCCEEDED(hr);

			return spFolder->CompareIDs(
				lParam, ::ILNext(pidl1), ::ILNext(pidl2));
		}
	}
	catchCom()
}

/**
 * Create one of the objects associated with the @b current folder view.
 *
 * @implementing IShellFolder
 *
 * The types of object which can be requested include IShellView, IContextMenu, 
 * IExtractIcon, IQueryInfo, IShellDetails or IDropTarget.  This method is in
 * contrast to GetUIObjectOf() which performs the same task but for an item
 * contained *within* the current folder rather than the folder itself.
 */
STDMETHODIMP CFolder::CreateViewObject(HWND hwndOwner, REFIID riid, void **ppv)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(ppv, E_POINTER);
	UNREFERENCED_PARAMETER(hwndOwner); // Not a custom folder, no parent needed

	*ppv = NULL;

	try
	{
		if (riid == __uuidof(IShellView))
		{
			TRACE("\tRequest: IShellView");

			SFV_CREATE sfvData = { sizeof sfvData, 0 };

			// Create a pointer to this IShellFolder to pass to view
			CComPtr<IShellFolder> spFolder = this;
			ATLENSURE_THROW(spFolder, E_NOINTERFACE);
			sfvData.pshf = spFolder;

			// Get the callback object for this folder view, if any.
			// Must hold reference to it in this CComPtr over the
			// ::SHCreateShellFolderView() call in case GetFolderViewCallback()
			// also creates it (hands back the only pointer to it).
			CComPtr<IShellFolderViewCB> spCB = GetFolderViewCallback();
			sfvData.psfvcb = spCB;

			// Create Default Shell Folder View object
			return ::SHCreateShellFolderView(
				&sfvData, reinterpret_cast<IShellView**>(ppv));
		}
		else if (riid == __uuidof(IShellDetails))
		{
			TRACE("\tRequest: IShellDetails");
			return QueryInterface(riid, ppv);
		}
		else
		{
			TRACE("\tRequest: <unknown>");
			return E_NOINTERFACE;
		}
	}
	catchCom()
}

/**
 * Sort by a given column of the folder view.
 *
 * @implementing IShellDetails
 *
 * @return S_FALSE to instruct the shell to perform the sort itself.
 */
STDMETHODIMP CFolder::ColumnClick(UINT iColumn)
{
	METHOD_TRACE;
	UNREFERENCED_PARAMETER(iColumn);
	return S_FALSE; // Tell the shell to sort the items itself
}

STDMETHODIMP CFolder::GetDefaultSearchGUID(GUID *pguid)
{
	ATLENSURE_RETURN_HR(pguid, E_POINTER);

	*pguid = GUID_NULL;

	ATLTRACENOTIMPL(__FUNCTION__);
}

/**
 * Return pointer to interface allowing client to enumerate search objects.
 *
 * @implementing IShellFolder2
 *
 * We do not support search objects so this method is not implemented.
 */
STDMETHODIMP CFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
	ATLENSURE_RETURN_HR(ppenum, E_POINTER);

	*ppenum = NULL;

	ATLTRACENOTIMPL(__FUNCTION__);
}

/**
 * Get the default sorting and display columns.
 *
 * @implementing IShellFolder2
 *
 * This is a default implementation which simply return the 1st (zeroth)
 * column for both sorting and display.  Derived classes can override this
 * if they need custom behaviour.
 */
STDMETHODIMP CFolder::GetDefaultColumn(
	DWORD /*dwReserved*/, ULONG* pSort, ULONG* pDisplay)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(pSort, E_POINTER);
	ATLENSURE_RETURN_HR(pDisplay, E_POINTER);

	*pSort = 0;
	*pDisplay = 0;

	return S_OK;
}