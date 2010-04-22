/**
    @file

    Base class for IShellFolder implementations.

    @if licence

    Copyright (C) 2008, 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/atl.hpp" // Common ATL setup
#include "swish/catch_com.hpp" // catchCom
#include "swish/debug.hpp" // METHOD_TRACE
#include "swish/exception.hpp"  // com_exception

#include <winapi/shell/pidl.hpp> // cpidl_t
#include <winapi/shell/property_key.hpp> // property_key

#include <comet/variant.h> // variant_t

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

namespace detail{

	/**
	 * Return the parent IShellFolder of the last item in the PIDL.
	 *
	 * Optionally, return the a pointer to the last item as well.  This
	 * function emulates the Vista-specific SHBindToFolderIDListParent API
	 * call.
	 */
	inline HRESULT BindToParentFolderOfPIDL(
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
}

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

	CFolder() : m_root_pidl(NULL) {}

	virtual ~CFolder()
	{
		if (m_root_pidl)
		{
			::ILFree(m_root_pidl);
			m_root_pidl = NULL;
		}
	}

	CAbsolutePidl clone_root_pidl() const
	{
		return root_pidl();
	}

	PCIDLIST_ABSOLUTE root_pidl() const
	{
		return m_root_pidl;
	}

public: // IPersist methods

	/**
	 * Get the class identifier (CLSID) of the object.
	 * 
	 * @implementing IPersist
	 *
	 * @param[in] pClassID  Location in which to return the CLSID.
	 */
	IFACEMETHODIMP GetClassID(CLSID* pClassID)
	{
		METHOD_TRACE;
		ATLENSURE_RETURN_HR(pClassID, E_POINTER);

		*pClassID = clsid();

		return S_OK;
	}

public: // IPersistFolder methods

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
	IFACEMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidl)
	{
		METHOD_TRACE;
		ATLENSURE_RETURN_HR(!::ILIsEmpty(pidl), E_INVALIDARG);
		ATLENSURE_RETURN_HR(m_root_pidl == NULL, E_UNEXPECTED);
		                                         // Multiple init

		m_root_pidl = ::ILCloneFull(pidl);
		ATLENSURE_RETURN_HR(!::ILIsEmpty(m_root_pidl), E_OUTOFMEMORY);

		return S_OK;
	}

public: // IPersistFolder2 methods

	/**
	 * Get the root of this folder - the absolute PIDL relative to the
	 * desktop.
	 *
	 * @implementing IPersistFolder2
	 *
	 * @param[out] ppidl  Location in which to return the copied PIDL.  
	 *                    If the folder hasn't been initialised yet, this 
	 *                    value will be NULL.
	 *
	 * @returns  S_FALSE if Initialize() hasn't been called.
	 */
	IFACEMETHODIMP GetCurFolder(PIDLIST_ABSOLUTE* ppidl)
	{
		METHOD_TRACE;
		ATLENSURE_RETURN_HR(ppidl, E_POINTER);

		*ppidl = NULL;

		if (root_pidl() == NULL) // Legal to call this before Initialize()
			return S_FALSE;

		// Copy the PIDL that was passed to us in Initialize()
		*ppidl = ::ILCloneFull(root_pidl());
		ATLENSURE_RETURN_HR(*ppidl, E_OUTOFMEMORY);
		
		return S_OK;
	}

public: // IPersistFolder3 methods
	
	IFACEMETHODIMP InitializeEx(
		IBindCtx* /*pbc*/, PCIDLIST_ABSOLUTE pidlRoot, 
		const PERSIST_FOLDER_TARGET_INFO* /*ppfti*/)
	{
		METHOD_TRACE;
		ATLENSURE_RETURN_HR(pidlRoot, E_POINTER);

		return Initialize(pidlRoot);
	}
	
	IFACEMETHODIMP GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO* ppfti)
	{
		METHOD_TRACE;
		ATLENSURE_RETURN_HR(ppfti, E_POINTER);

		::ZeroMemory(ppfti, sizeof(PERSIST_FOLDER_TARGET_INFO));
		return E_NOTIMPL;
	}

public: // IPersistIDList methods

	IFACEMETHODIMP SetIDList(PCIDLIST_ABSOLUTE pidl)
	{
		METHOD_TRACE;
		return Initialize(pidl);
	}
		
	IFACEMETHODIMP GetIDList(PIDLIST_ABSOLUTE* ppidl)
	{
		METHOD_TRACE;
		return GetCurFolder(ppidl);
	}

public: // IShellFolder methods
	
	IFACEMETHODIMP BindToStorage( 
		PCUIDLIST_RELATIVE pidl, IBindCtx* /*pbc*/, REFIID /*riid*/,
		void** ppv)
	{
		METHOD_TRACE;
		ATLENSURE_RETURN_HR(pidl, E_POINTER);
		ATLENSURE_RETURN_HR(ppv, E_POINTER);

		*ppv = NULL;
		return E_NOTIMPL;
	}

	/**
	 * Caller is requesting a subobject of this folder.
	 *
	 * @implementing IShellFolder
	 *
	 * Typically this is an IShellFolder although it may be an IStream.  
	 * Whereas CreateViewObject() and GetUIObjectOf() request *associated*
	 * objects of items in the filesystem hierarchy, calls to BindToObject()
	 * are for the objects representing the actual filesystem items.  Hence,
	 * IShellFolder for folders and IStream for files.
	 *
	 * @todo  Find out more about how IStreams are dealt with and what it 
	 *        gains us.
	 *
	 * Create and initialise an instance of the subitem represented by @a pidl
	 * and return the interface asked for in @a riid.
	 *
	 * @param[in]  pidl  PIDL to the requested object @b relative to this
	 *                   folder.
	 * @param[in]  pbc   Binding context.
	 * @param[in]  riid  IID of the interface being requested.
	 * @param[out] ppv   Location in which to return the requested interface.
	 */
	IFACEMETHODIMP BindToObject( 
		PCUIDLIST_RELATIVE pidl, IBindCtx* pbc, REFIID riid, void** ppv)
	{
		METHOD_TRACE;
		ATLENSURE_RETURN_HR(!::ILIsEmpty(pidl), E_INVALIDARG);
		ATLENSURE_RETURN_HR(ppv, E_POINTER);

		*ppv = NULL;

		// TODO: We can optimise this function by immediately returning
		// E_NOTIMPL for any riid that we know our children and grandchildren
		// don't provide. This is not in the spirit of COM QueryInterface but
		// it may be a performance boost.

		try
		{
			// First item in pidl must be of our type
			validate_pidl(pidl);

			if (::ILIsChild(pidl)) // Our child subfolder is the target
			{
				// Create absolute PIDL to the subfolder by combining with 
				// our root
				CAbsolutePidl pidl_sub_root;
				pidl_sub_root.Attach(::ILCombine(root_pidl(), pidl));
				if (!pidl_sub_root)
					throw swish::exception::com_exception(E_OUTOFMEMORY);

				ATL::CComPtr<IShellFolder> folder = subfolder(pidl_sub_root);

				return folder->QueryInterface(riid, ppv);
			}
			else
			// One of our grandchildren is the target - delegate to its parent
			{
				ATL::CComPtr<IShellFolder> folder;

				PCUITEMID_CHILD pidl_grandchild = NULL;
				HRESULT hr = detail::BindToParentFolderOfPIDL(
					this, pidl, IID_PPV_ARGS(&folder), &pidl_grandchild);
				if FAILED(hr)
					throw swish::exception::com_exception(hr);

				return folder->BindToObject(pidl_grandchild, pbc, riid, ppv);
			}
		}
		catchCom()
	}

	/**
	 * Determine the relative order of two file objects or folders.
	 *
	 * @implementing IShellFolder
	 *
	 * Given their item identifier lists, compare the two objects and return a
	 * value in the HRESULT indicating the result of the comparison:
	 * - Negative: pidl1 < pidl2
	 * - Positive: pidl1 > pidl2
	 * - Zero:     pidl1 == pidl2
	 */
	IFACEMETHODIMP CompareIDs( 
		LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
	{
		METHOD_TRACE;

		int column = LOWORD(lParam);

		bool compare_all_fields = (HIWORD(lParam) == SHCIDS_ALLFIELDS);
		bool canonical = (HIWORD(lParam) == SHCIDS_CANONICALONLY);

		assert(!compare_all_fields || column == 0);
		assert(!canonical || !compare_all_fields);

		try
		{
			// Handle empty PIDL cases
			if (::ILIsEmpty(pidl1) && ::ILIsEmpty(pidl2)) // Both empty - equal
				return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
			else if (::ILIsEmpty(pidl1))                  // Only pidl1 empty <
				return MAKE_HRESULT(SEVERITY_SUCCESS, 0, -1);
			else if (::ILIsEmpty(pidl2))                  // Only pidl2 empty >
				return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
			
			// Explorer can pass us invalid PIDLs from its cache if our PIDL 
			// representation changes.  We catch that here to stop us asserting
			// later.
			validate_pidl(pidl1);
			validate_pidl(pidl2);

			// The casts here are OK.  We are aware compare_pidls only compares
			// a single item.  We recurse later if needed.
			int result = compare_pidls(
				static_cast<PCUITEMID_CHILD>(pidl1),
				static_cast<PCUITEMID_CHILD>(pidl2),
				column, compare_all_fields, canonical);

			if ((::ILIsChild(pidl1) && ::ILIsChild(pidl2)) || result != 0)
			{
				// The cast to unsigned short is *crucial*!  Without it,
				// sorting in Explorer does all sorts of wierd stuff
				return MAKE_HRESULT(
					SEVERITY_SUCCESS, 0, (unsigned short)result);
			}
			else
			{
				// The first items are equal and there are more items to
				// compare.
				// Delegate the rest of the comparison to our child.

				ATL::CComPtr<IShellFolder> folder;

				PITEMID_CHILD pidl_child = ::ILCloneFirst(pidl1);		
				HRESULT hr = BindToObject(
					pidl_child, NULL, IID_PPV_ARGS(&folder));
				::ILFree(pidl_child);
				ATLENSURE_SUCCEEDED(hr);

				return folder->CompareIDs(
					lParam, ::ILNext(pidl1), ::ILNext(pidl2));
			}
		}
		catchCom()
	}

	/**
	 * Create an object associated with the @b current folder.
	 *
	 * @implementing IShellFolder
	 *
	 * The types of object which can be requested include IShellView,
	 * IContextMenu, IExtractIcon, IQueryInfo, IShellDetails or IDropTarget.
	 * This method is in contrast to GetUIObjectOf() which performs the same
	 * task but for an item contained *within* the current folder rather than
	 * the folder itself.
	 *
	 * @param [in]   hwnd  Handle to the parent window, if any, of any UI that
	 *                     may be needed to complete the request.
	 * @param [in]   riid  Interface UUID for the object being requested.
	 * @param [out]  ppv   Return value.
	 */
	IFACEMETHODIMP CreateViewObject(HWND hwnd, REFIID riid, void** ppv)
	{
		METHOD_TRACE;
		ATLENSURE_RETURN_HR(ppv, E_POINTER);

		*ppv = NULL;

		try
		{
			ATL::CComPtr<IUnknown> object = folder_object(hwnd, riid);
			return object->QueryInterface(riid, ppv);
		}
		catchCom()
		return S_OK;
	}

	/**
	 * Create an object associated with an item in the current folder.
	 *
	 * @implementing IShellFolder
	 *
	 * Callers will request an associated object, such as a context menu, for 
	 * items in the folder by calling this method with the IID of the object 
	 * they want and the PIDLs of the items they want it for.  In addition, 
	 * if the don't pass any PIDLs then they are requesting an associated 
	 * object of this folder.
	 *
	 * We deal with the request as follows:
	 * - If the request is for an object associated with this folder, we
	 *   call folder_item_object() with the requested IID.  You should override
	 *   that method if your folder supports any associated objects.
	 *
	 * - If the request is for items in this in this folder we call
	 *   folder_item_object() with the IID and the PIDLs.  Again, you should
	 *   override that method if you want to support associated objects for
	 *   your folder's contents.
	 *
	 * - If the previous step fails to find the associated object and there is
	 *   only a single PIDL, we attempt to bind to the item in the PIDL as
	 *   an IShellFolder.  If this succeeds we delegate the object lookup to
	 *   this subfolder by calling its IShellFolder::CreateViewObject() method.
	 *
	 * The idea is that a given folder implementation answers object queries
	 * for itself and the non-folder items within it.  Additionally, it can
	 * answer queries for sub-folders if it chooses but it doesn't have to. If
	 * it doesn't, the request will be delegated to the subfolder
	 * implementation.
	 * 
	 * CreateViewObject() performs the same task as GetUIObjectOf but only
	 * for the folder, not for items within it.
	 *
	 * @param [in]   hwnd  Handle to the parent window, if any, of any UI that
	 *                     may be needed to complete the request.
	 * @param [in]   riid  Interface UUID for the object being requested.
	 * @param [out]  ppv   Return value.
	 */
	IFACEMETHODIMP GetUIObjectOf(
		HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
		UINT* /*puReserved*/, void** ppv) 
	{
		METHOD_TRACE;
		ATLENSURE_RETURN_HR(ppv, E_POINTER);

		*ppv = NULL;

		try
		{
			ATL::CComPtr<IUnknown> object;

			if (cpidl == 0)
				object = folder_object(hwnd, riid); // Equiv CreateViewObject
			else
			{
				try
				{
					object = folder_item_object(hwnd, riid, cpidl, apidl);
				}
				catch (swish::exception::com_exception e)
				{
					if (e == E_NOINTERFACE && cpidl == 1)
						object = _delegate_object_lookup_to_subfolder(
							hwnd, riid, apidl[0]);
					else
						throw;
				}
			}

			return object->QueryInterface(riid, ppv);
		}
		catchCom()
		return S_OK;
	}

public: // IShellDetails methods

	/**
	 * Sort by a given column of the folder view.
	 *
	 * @implementing IShellDetails
	 *
	 * @return S_FALSE to instruct the shell to perform the sort itself.
	 */
	IFACEMETHODIMP ColumnClick(UINT /*iColumn*/)
	{
		METHOD_TRACE;
		return S_FALSE; // Tell the shell to sort the items itself
	}

public: // IShellFolder2 methods

	IFACEMETHODIMP GetDefaultSearchGUID(GUID* pguid)
	{
		METHOD_TRACE;
		ATLENSURE_RETURN_HR(pguid, E_POINTER);

		*pguid = GUID_NULL;
		return E_NOTIMPL;
	}

	/**
	 * Return pointer to interface allowing client to enumerate search objects.
	 *
	 * @implementing IShellFolder2
	 *
	 * We do not support search objects so this method is not implemented.
	 */
	IFACEMETHODIMP EnumSearches(IEnumExtraSearch** ppenum)
	{
		METHOD_TRACE;
		ATLENSURE_RETURN_HR(ppenum, E_POINTER);

		*ppenum = NULL;
		return E_NOTIMPL;
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
	IFACEMETHODIMP GetDefaultColumn(
		DWORD /*dwReserved*/, ULONG* pSort, ULONG* pDisplay)
	{
		METHOD_TRACE;
		ATLENSURE_RETURN_HR(pSort, E_POINTER);
		ATLENSURE_RETURN_HR(pDisplay, E_POINTER);

		*pSort = 0;
		*pDisplay = 0;

		return S_OK;
	}
	
	/**
	 * Get property of an item as a VARIANT.
	 *
	 * @implementing IShellFolder2
	 *
	 * The work is delegated to the subclass.
	 */
	IFACEMETHODIMP GetDetailsEx(
		PCUITEMID_CHILD pidl, const SHCOLUMNID* pscid, VARIANT* pv)
	{
		if (!pv) return E_POINTER;
		::VariantClear(pv);
		if (!pscid) return E_POINTER;
		if (::ILIsEmpty(pidl)) return E_INVALIDARG;

		try
		{
			comet::variant_t var = property(*pscid, pidl);
			*pv = var.detach();
		}
		catchCom()

		return S_OK;
	}

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

	/**
	 * Delegate associated object lookup to subfolder item's CreateViewObject.
	 *
	 * Attempts to bind to the item, given in the PIDL, as an IShellFolder.
	 * If this succeeds, that folder is queried for its associated object by
	 * a call to IShellFolder::CreateViewObject().
	 */
	ATL::CComPtr<IUnknown> CFolder::_delegate_object_lookup_to_subfolder(
		HWND hwnd, REFIID riid, PCUITEMID_CHILD pidl)
	{
		HRESULT hr;
		ATL::CComPtr<IShellFolder> subfolder;
		hr = BindToObject(pidl, NULL, IID_PPV_ARGS(&subfolder));
		if (FAILED(hr))
			throw swish::exception::com_exception(hr);

		ATL::CComPtr<IUnknown> object;
		hr = subfolder->CreateViewObject(
			hwnd, riid, reinterpret_cast<void**>(&object));
		if (FAILED(hr))
			throw swish::exception::com_exception(hr);

		return object;
	}
	PIDLIST_ABSOLUTE m_root_pidl;
};

}}} // namespace swish::shell_folder::folder
