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

#pragma once
#include "shobjidl.h"

#include "Pidl.h"

#ifndef __IPersistIDList_INTERFACE_DEFINED__
#define __IPersistIDList_INTERFACE_DEFINED__

	EXTERN_C const IID IID_IPersistIDList;
    
    MIDL_INTERFACE("1079acfc-29bd-11d3-8e0d-00c04f6837d5")
    IPersistIDList : public IPersist
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetIDList( 
            /* [in] */ __RPC__in PCIDLIST_ABSOLUTE pidl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIDList( 
            /* [out] */ __RPC__deref_out_opt PIDLIST_ABSOLUTE *ppidl) = 0;
        
    };
#endif

/**
 * Return the parent IShellFolder of the last item in the PIDL.
 *
 * Optionally, return the a pointer to the last item as well.  This
 * function emulates the Vista-specific SHBindToFolderIDListParent API
 * call.
 */
inline HRESULT _BindToParentFolderOfPIDL(
	IShellFolder *psfRoot, PCUIDLIST_RELATIVE pidl, REFIID riid, 
	__out void **ppvParent, __out_opt PCUITEMID_CHILD *ppidlChild)
{
	*ppvParent = NULL;
	if (ppidlChild)
		*ppidlChild = NULL;

	// Equivalent to 
	//     ::SHBindToFolderIDListParent(
	//         psfRoot, pidl, riid, ppvParent, ppidlChild);
	
	// Create PIDL to penultimate item (parent)
	PIDLIST_RELATIVE pidlParent = ::ILClone(pidl);
	ATLVERIFY(::ILRemoveLastID(pidlParent));

	// Bind to the penultimate PIDL's folder (parent folder)
	HRESULT hr = psfRoot->BindToObject(pidlParent, NULL, riid, ppvParent);
	::ILFree(pidlParent);
	
	if (SUCCEEDED(hr) && ppidlChild)
	{
		*ppidlChild = ::ILFindLastID(pidl);
	}

	return hr;
}

class CFolder :
	public CComObjectRoot,
	public IPersistFolder3,
	public IShellFolder2,
	public IPersistIDList,
	public IShellDetails
{
public:

	BEGIN_COM_MAP(CFolder)
		COM_INTERFACE_ENTRY(IPersistFolder3)
		COM_INTERFACE_ENTRY(IShellFolder2)
		COM_INTERFACE_ENTRY(IShellDetails)
		COM_INTERFACE_ENTRY(IPersistIDList)
		COM_INTERFACE_ENTRY2(IPersist,        IPersistFolder3)
		COM_INTERFACE_ENTRY2(IPersistFolder,  IPersistFolder3)
		COM_INTERFACE_ENTRY2(IPersistFolder2, IPersistFolder3)
		COM_INTERFACE_ENTRY2(IShellFolder,    IShellFolder2)
	END_COM_MAP()

	CFolder();
	virtual ~CFolder();

public: // IPersist methods

	IFACEMETHODIMP GetClassID(__out CLSID *pClassID);

public: // IPersistFolder methods

	IFACEMETHODIMP Initialize(__in PCIDLIST_ABSOLUTE pidl);

public: // IPersistFolder2 methods
	
	IFACEMETHODIMP GetCurFolder(__deref_out_opt PIDLIST_ABSOLUTE *ppidl);

public: // IPersistFolder3 methods
	
	IFACEMETHODIMP InitializeEx(
		__in_opt IBindCtx *pbc,
		__in PCIDLIST_ABSOLUTE pidlRoot,
		__in_opt const PERSIST_FOLDER_TARGET_INFO *ppfti);
	
	IFACEMETHODIMP GetFolderTargetInfo(
		__out PERSIST_FOLDER_TARGET_INFO *ppfti);

public: // IPersistIDList methods
	
	IFACEMETHODIMP SetIDList(__in PCIDLIST_ABSOLUTE pidl);
	
	IFACEMETHODIMP GetIDList(__deref_out_opt PIDLIST_ABSOLUTE *ppidl);

public: // IShellFolder methods

	IFACEMETHODIMP BindToObject( 
		__in PCUIDLIST_RELATIVE pidl,
		__in_opt IBindCtx *pbc,
		__in REFIID riid,
		__deref_out_opt void **ppv);

	IFACEMETHODIMP BindToStorage( 
		__in PCUIDLIST_RELATIVE pidl,
		__in_opt IBindCtx *pbc,
		__in REFIID riid,
		__deref_out_opt void **ppv);

	IFACEMETHODIMP CompareIDs( 
		LPARAM lParam,
		__in PCUIDLIST_RELATIVE pidl1,
		__in PCUIDLIST_RELATIVE pidl2);

	IFACEMETHODIMP CreateViewObject( 
		__in_opt HWND hwndOwner,
		__in REFIID riid,
		__deref_out_opt void **ppv);

public: // IShellDetails methods

	IFACEMETHODIMP ColumnClick(UINT iColumn);

public: // IShellFolder2 methods
	
	IFACEMETHODIMP GetDefaultSearchGUID(__out GUID *pguid);

	IFACEMETHODIMP EnumSearches(__deref_out_opt IEnumExtraSearch **ppenum);
	
	IFACEMETHODIMP GetDefaultColumn(
		DWORD dwReserved,
		__out ULONG* pSort,
		__out ULONG* pDisplay);

protected:

	CAbsolutePidl CloneRootPIDL() const;
	PCIDLIST_ABSOLUTE GetRootPIDL() const;

	virtual void ValidatePidl(__in PCUIDLIST_RELATIVE pidl)
		const throw(...) PURE;
	virtual CLSID GetCLSID()
		const PURE;
	virtual CComPtr<IShellFolder> CreateSubfolder(
		__in PCIDLIST_ABSOLUTE pidlRoot)
		const throw(...) PURE;
	virtual int ComparePIDLs(
		__in PCUITEMID_CHILD pidl1, __in PCUITEMID_CHILD pidl2,
		USHORT uColumn, bool fCompareAllFields, bool fCanonical)
		const throw(...) PURE;

	virtual CComPtr<IShellFolderViewCB> GetFolderViewCallback()
		const throw(...);

private:
	PIDLIST_ABSOLUTE m_pidlRoot;
};
