/**
    @file

    Base class for IShellFolder implementations.

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

#include "Pidl.h"

#include <winapi/shell/pidl.hpp> // cpidl_t
#include <winapi/shell/property_key.hpp> // property_key

#include <comet/variant.h> // variant_t

#include "swish/atl.hpp"  // Common ATL setup

#define STRICT_TYPED_ITEMIDS ///< Better type safety for PIDLs (must be 
                             ///< before <shtypes.h> or <shlobj.h>).
#include <shlobj.h>     // Windows Shell API

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

namespace swish {
namespace shell_folder {
namespace folder {

class CFolder :
	public ATL::CComObjectRoot,
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

	CAbsolutePidl clone_root_pidl() const;
	PCIDLIST_ABSOLUTE root_pidl() const;

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

	IFACEMETHODIMP GetUIObjectOf( 
		__in_opt HWND hwndOwner,
		__in UINT cpidl,
		__in_ecount_opt(cpidl) PCUITEMID_CHILD_ARRAY apidl,
		__in REFIID riid,
		__reserved  UINT *rgfReserved,
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
	
	IFACEMETHODIMP GetDetailsEx(
		PCUITEMID_CHILD pidl, const SHCOLUMNID* pscid, VARIANT* pv);

protected:

	/**
	 * Return the folder implementation's CLSID.
	 *
	 * This allows callers to identify the type of folder for persistence
	 * etc.
	 */
	virtual CLSID clsid() const = 0;

	/**
	 * Check if a PIDL is recognised.
	 *
	 * Explorer has a tendency to pass our folders PIDLs that don't belong to
	 * them to see if we are paying attention (or, more likely, to see if it
	 * can use some PIDL data that it cached earlier.  We need to disbelieve 
	 * everything Explorer tells us an validate each PIDL it gives us.
	 *
	 * Implementation should sniff the PIDLs passed to this method and
	 * throw an exception of they don't recognise them.
	 */
	virtual void validate_pidl(PCUIDLIST_RELATIVE pidl) const = 0;

	/**
	 * Compare two items in the folder.
	 *
	 * @returns
	 * - Negative: pidl1 < pidl2
	 * - Positive: pidl1 > pidl2
	 * - Zero:     pidl1 == pidl2
	 *
	 * This is one of the most important methods to get right.  When
	 * implementing it, take care that if two PIDLs don't represent the 
	 * same filesystem item you don't return 0 from this method!  Explorer
	 * uses this to test if an item it has cached and if you falsely
	 * claim that it is Explorer is likely not to bother looking at your
	 * item becuase it thinks it already has it.
	 */
	virtual int compare_pidls(
		PCUITEMID_CHILD pidl1, PCUITEMID_CHILD pidl2,
		int column, bool compare_all_fields, bool canonical) const = 0;

	/**
	 * The caller is requesting an object associated with the current folder.
	 *
	 * The default implementation throws an E_NOINTERFACE exception which
	 * indicates to the caller that the object doesn't exist.  You almost 
	 * certainly want to override this method in your folder.
	 *
	 * Examples of the type of object that Explorer may request include 
	 *  - IShellView, the GUI representation of your folder
	 *  - IDropTarget, in order to support drag-and-drop into your GUI window
	 *  - IContextMenu, if your folder has a context menu
	 *
	 * This method corresponds roughly to CreateViewObject() but also
	 * deals with the unusual case where GetUIObjectOf() is called without 
	 * any PIDLs.
	 */
	virtual ATL::CComPtr<IUnknown> folder_object(HWND hwnd, REFIID riid) = 0;

	/**
	 * The caller is requesting an object associated with one of more items
	 * in the current folder.
	 *
	 * The default implementation throws an E_NOINTERFACE exception which
	 * indicates to the caller that the object doesn't exist.  You almost 
	 * certainly want to override this method in your folder to return
	 * associated objects for @b non-folder item.  It is also common to
	 * handle some objects for sub-folder items too.  For example, handling
	 * IDropTarget requests here avoids the need to bind to an instance of
	 * the subfolder implementation first.
	 *
	 * If a request isn't handled here (this method throws E_NOINTERFACE)
	 * and it's possible to bind to the item's IShellFolder interface then 
	 * the request is delegated to the folder's CreateViewObject method and, 
	 * assuming that folder is implemented with this class, will end up at 
	 * the subfolder's folder_object() method.  However if this method throws
	 * a different error, the request is not delegated.
	 *
	 * This method corresponds roughly to GetUIObjectOf().
	 */
	virtual ATL::CComPtr<IUnknown> folder_item_object(
		HWND hwnd, REFIID riid, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl) = 0;

	/**
	 * The caller is asking for an IShellFolder handler for a subfolder.
	 *
	 * The pidl passed to this method will be the @b absolute PIDL that
	 * the folder is root.  It may not end in one of our own PIDLs if this
	 * is the root folder of our own hierarchy.  In that case it will be
	 * the PIDL of the external hosting folder such as 'Desktop' or
	 * 'My Computer'
	 *
	 * Respond to the request by creating an instance of the subfolder
	 * handler object (which may well be another instance of your folder
	 * class) and initialise it with the PIDL.
	 *
	 * This method corresonds to BindToFolder() where the item is directly
	 * in the current folder (not a grandchild).
	 */
	virtual ATL::CComPtr<IShellFolder> subfolder(PCIDLIST_ABSOLUTE pidl)
		const = 0;

	/**
	 * The caller is asking for some property of an item in this folder.
	 *
	 * Which property is indicated by the given PROPERTYKEY (a GUID, aka
	 * SHCOLUMNID).  Common PROPERTYKEYs are in Propkey.h.
	 *
	 * The return value is a variant so can be any type that VARIANTs
	 * support.
	 */
	virtual comet::variant_t property(
		const winapi::shell::property_key& key,
		const winapi::shell::pidl::cpidl_t& pidl) = 0;

private:
	ATL::CComPtr<IUnknown> _delegate_object_lookup_to_subfolder(
		HWND hwnd, REFIID riid, PCUITEMID_CHILD pidl);

	PIDLIST_ABSOLUTE m_root_pidl;
};

}}} // namespace swish::shell_folder::folder
