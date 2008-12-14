/*  Base class for IShellFolder implementations.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "stdafx.h"
#include "Folder.h"
#include "Pidl.h"

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
	ATLENSURE_RETURN_HR(pidl, E_POINTER);
	ATLENSURE_RETURN_HR(ppv, E_POINTER);

	*ppv = NULL;

	ATLTRACENOTIMPL(__FUNCTION__);
}

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

STDMETHODIMP CFolder::GetDefaultSearchGUID(GUID *pguid)
{
	ATLENSURE_RETURN_HR(pguid, E_POINTER);

	*pguid = GUID_NULL;

	ATLTRACENOTIMPL(__FUNCTION__);
}

STDMETHODIMP CFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
	ATLENSURE_RETURN_HR(ppenum, E_POINTER);

	*ppenum = NULL;

	ATLTRACENOTIMPL(__FUNCTION__);
}
