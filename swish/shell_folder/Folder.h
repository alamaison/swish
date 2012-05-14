/**
    @file

    Base class for IShellFolder implementations.

    @if license

    Copyright (C) 2008, 2009, 2010, 2012
    Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include "swish/debug.hpp" // METHOD_TRACE

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE
#include <winapi/shell/folder_error_adapters.hpp> // folder2_error_adapter
#include <winapi/shell/pidl.hpp> // apidl_t, cpidl_t
#include <winapi/shell/property_key.hpp> // property_key
#include <winapi/shell/shell.hpp> // string_to_strret

#include <comet/error.h> // com_error
#include <comet/variant.h> // variant_t

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <shlobj.h>     // Windows Shell API

#include <cstring> // memset, memcpy
#include <stdexcept> // range_error

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

namespace comet {

template<> struct comtype<IPersistFolder2>
{
	static const IID& uuid() { return IID_IPersistFolder; }
	typedef IPersistFolder base;
};

}

namespace swish {
namespace shell_folder {
namespace folder {

namespace detail {

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

template<typename ColumnType>
class CFolder :
	public ATL::CComObjectRoot,
	public winapi::shell::folder2_error_adapter,
	public winapi::shell::shell_details_error_adapter,
	public IPersistFolder3,
	public IPersistIDList
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

	CFolder() {}

	virtual ~CFolder() {}

	const winapi::shell::pidl::apidl_t& root_pidl() const	
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
		ATLENSURE_RETURN_HR(!m_root_pidl, E_UNEXPECTED); // Multiple init

		m_root_pidl = pidl;
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

		try
		{
			if (!root_pidl()) // Legal to call this before Initialize()
				return S_FALSE;

			// Copy the PIDL that was passed to us in Initialize()
			root_pidl().copy_to(*ppidl);
		}
		WINAPI_COM_CATCH_INTERFACE(IPersistFolder2);
		
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

	virtual void bind_to_storage(
		PCUIDLIST_RELATIVE /*pidl*/, IBindCtx* /*bind_ctx*/,
		const IID& /*iid*/, void** /*interface_out*/)
	{
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOTIMPL));
	}

	/**
	 * Caller is requesting a subobject of this folder.
	 *
	 * @implementing folder_error_adapter
	 *
	 * Create and initialise an instance of the subitem represented by @a pidl
	 * and return the interface asked for in @a iid.
	 *
	 * Typically this is an IShellFolder although it may be an IStream.  
	 * Whereas create_view_object() and get_ui_object_of() request 'associated
	 * objects' of items in the hierarchy, calls to bind_to_object()
	 * are for the objects representing the items themselves.  E.g.,
	 * IShellFolder for folders and IStream for files.
	 *
	 * @todo  Find out more about how IStreams are dealt with and what it 
	 *        gains us.
	 *
	 * @param[in]  pidl           PIDL to the requested object @b relative to
	 *                            this folder.
	 * @param[in]  bind_ctx       Binding context.
	 * @param[in]  iid            IID of the interface being requested.
	 * @param[out] interface_out  Location in which to return the requested
	 *                            interface.
	 *
	 * @note  Corresponds to IShellFolder::BindToObject.
	 */
	virtual void bind_to_object(
		PCUIDLIST_RELATIVE pidl, IBindCtx* bind_ctx, const IID& iid,
		void** interface_out)
	{
		if (::ILIsEmpty(pidl))
			BOOST_THROW_EXCEPTION(comet::com_error(E_INVALIDARG));

		// TODO: We can optimise this function by immediately throwing
		// E_NOTIMPL for any riid that we know our children and grandchildren
		// don't provide. This is not in the spirit of COM QueryInterface but
		// it may be a performance boost.

		// First item in pidl must be of our type
		validate_pidl(pidl);

		if (::ILIsChild(pidl)) // Our child subfolder is the target
		{
			comet::com_ptr<IShellFolder> folder = subfolder(
				reinterpret_cast<PCUITEMID_CHILD>(pidl));

			// Create absolute PIDL to the subfolder by combining with 
			// our root
			HRESULT hr = folder->QueryInterface(iid, interface_out);
			if (FAILED(hr))
				comet::throw_com_error(folder.get(), hr);
		}
		else
		// One of our grandchildren is the target - delegate to its parent
		{
			comet::com_ptr<IShellFolder> folder;

			PCUITEMID_CHILD pidl_grandchild = NULL;
			HRESULT hr = detail::BindToParentFolderOfPIDL(
				this, pidl, IID_PPV_ARGS(folder.out()), &pidl_grandchild);
			if (FAILED(hr))
				comet::throw_com_error(
					comet::com_ptr<IShellFolder>(this).get(), hr);

			hr = folder->BindToObject(
				pidl_grandchild, bind_ctx, iid, interface_out);
			if (FAILED(hr))
				comet::throw_com_error(folder.get(), hr);
		}
	}

	/**
	 * Determine the relative order of two items in or below this folder.
	 *
	 * @implementing folder_error_adapter
	 *
	 * Given their item identifier lists, compare the two objects and return a
	 * value indicating the result of the comparison:
	 * - Negative: pidl1 < pidl2
	 * - Positive: pidl1 > pidl2
	 * - Zero:     pidl1 == pidl2
	 *
	 * @note  Corresponds to IShellFolder::CompareIDs.
	 */
	int compare_ids( 
		LPARAM lparam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
	{
		int column = LOWORD(lparam);

		bool compare_all_fields = (HIWORD(lparam) == SHCIDS_ALLFIELDS);
		bool canonical = (HIWORD(lparam) == SHCIDS_CANONICALONLY);

		assert(!compare_all_fields || column == 0);
		assert(!canonical || !compare_all_fields);

		// Handle empty PIDL cases
		if (::ILIsEmpty(pidl1) && ::ILIsEmpty(pidl2)) // Both empty - equal
			return 0;
		else if (::ILIsEmpty(pidl1))                  // Only pidl1 empty <
			return -1;
		else if (::ILIsEmpty(pidl2))                  // Only pidl2 empty >
			return 1;
		
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
			return result;
		}
		else
		{
			// The first items are equal and there are more items to
			// compare.
			// Delegate the rest of the comparison to our child.

			comet::com_ptr<IShellFolder> folder;

			winapi::shell::pidl::cpidl_t child;
			child.attach(::ILCloneFirst(pidl1));		
			bind_to_object(child.get(), NULL, IID_PPV_ARGS(folder.out()));

			HRESULT hr = folder->CompareIDs(
				lparam, ::ILNext(pidl1), ::ILNext(pidl2));
			if (FAILED(hr))
				comet::throw_com_error(folder.get(), hr);

			return (short)HRESULT_CODE(hr);
		}
	}

	/**
	 * Create an object associated with @b this folder.
	 *
	 * @implementing folder_error_adapter
	 *
	 * The types of object which can be requested include IShellView,
	 * IContextMenu, IExtractIcon, IQueryInfo, IShellDetails or IDropTarget.
	 * This method is in contrast to get_ui_object_of() which performs the same
	 * task but for an item contained *within* the current folder rather than
	 * the folder itself.
	 *
	 * @param[in]   hwnd           Handle to the parent window, if any, of any
	 *                             UI that may be needed to complete the 
	 *                             request.
	 * @param[in]   iid            Interface UUID for the object being
	 *                             requested.
	 * @param[out]  interface_out  Return value.
	 *
	 * @note  Corresponds to IShellFolder::CreateViewObject.
	 */
	void create_view_object(HWND hwnd_owner, REFIID iid, void** interface_out)
	{
		comet::com_ptr<IUnknown> object = folder_object(hwnd_owner, iid);

		HRESULT hr = object.get()->QueryInterface(iid, interface_out);
		if (FAILED(hr))
			comet::throw_com_error(object.get(), hr);
	}

	/**
	 * Create an object associated with an item in the current folder.
	 *
	 * @implementing folder_error_adapter
	 *
	 * Callers will request an associated object, such as a context menu, for 
	 * items in the folder by calling this method with the IID of the object 
	 * they want and the PIDLs of the items they want it for.  In addition, 
	 * if the don't pass any PIDLs then they are requesting an associated 
	 * object of this folder.
	 *
	 * We deal with the request as follows:
	 * - If the request is for an object associated with this folder, we
	 *   call folder_object() with the requested IID.  You should override
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
	 * create_view_object() performs the same task as get_ui_object_of() but
	 * only for the folder, not for items within it.
	 *
	 * @param[in]  hwnd_owner     Handle to the parent window, if any, of any
	 *                            UI that may be needed to complete the
	 *                            request.
	 * @param[in]  riid           Interface UUID for the object being
	 *                            requested.
	 * @param[out] interface_out  Return value.
	 *
	 * @note  Corresponds to IShellFolder::GetUIObjectIf.
	 */
	void get_ui_object_of(
		HWND hwnd_owner, UINT pidl_count, PCUITEMID_CHILD_ARRAY pidl_array,
		const IID& iid, void** interface_out)
	{
		comet::com_ptr<IUnknown> object;

		if (pidl_count == 0)
		{
			// Equivalent to CreateViewObject
			object = folder_object(hwnd_owner, iid);
		}
		else
		{
			try
			{
				object = folder_item_object(
					hwnd_owner, iid, pidl_count, pidl_array);
			}
			catch (const comet::com_error& e)
			{
				if (e.hr() == E_NOINTERFACE && pidl_count == 1)
					object = _delegate_object_lookup_to_subfolder(
						hwnd_owner, iid, pidl_array[0]);
				else
					throw;
			}
		}

		HRESULT hr = object.get()->QueryInterface(iid, interface_out);
		if (FAILED(hr))
			comet::throw_com_error(object.get(), hr);
	}

public: // IShellDetails methods

	/**
	 * Sort by a given column of the folder view.
	 *
	 * @implementing shell_details_base_interface
	 *
	 * @return false to instruct the shell to perform the sort itself.
	 */
	bool column_click(UINT /*column_index*/)
	{
		return false;
	}
	
public: // IShellFolder2 methods

	/**
	 * Detailed information about an item in a folder.
	 *
	 * The desired detail is specified by a column index.
	 *
	 * @implementing folder_base_interface2
	 *
	 * This function operates in two distinctly different ways:
	 *  - if pidl is NULL:
	 *       Retrieve the the names of the columns themselves.
	 *  - if pidl is not NULL:
	 *       Retrieve  information for the item in the given pidl.
	 *
	 * The caller indicates which detail they want by specifying a column index
	 * in column_index.  If this column does not exist, throw an error.
	 *
	 * @retval  A SHELLDETAILS structure holding the requested detail as a
	 *          string along with various metadata.
	 *
	 * @note  Typically, a folder view calls this method repeatedly,
	 *        incrementing the column index each time.  The first column for
	 *        which we throw an error, marks the end of the columns in this
	 *        folder.
	 */
	SHELLDETAILS get_details_of(PCUITEMID_CHILD pidl, UINT column_index)
	{
		ColumnType col(column_index);

		SHELLDETAILS details;
		std::memset(&details, 0, sizeof(SHELLDETAILS));

		if (!pidl)
		{
			details.cxChar = col.average_width_in_chars();
			details.fmt = col.format();
			details.str = winapi::shell::string_to_strret(col.header());
		}
		else
		{
			details.str = winapi::shell::string_to_strret(col.detail(pidl));
		}

		return details;
	}

	/**
	 * GUID of the search to invoke when the user clicks on the search
	 * toolbar button.
	 *
	 * @implementing folder_base_interface2
	 *
	 * We do not support search objects so this method is not implemented.
	 */
	GUID get_default_search_guid()
	{
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOTIMPL));
		return GUID();
	}

	/**
	 * Enumeration of all searches supported by this folder
	 *
	 * @implementing folder_base_interface2
	 *
	 * We do not support search objects so this method is not implemented.
	 */
	IEnumExtraSearch* enum_searches()
	{
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOTIMPL));
		return NULL;
	}

	/**
	 * Default sorting and display columns.
	 *
	 * @implementing folder_base_interface2
	 *
	 * This is a default implementation which simply return the 1st (zeroth)
	 * column for both sorting and display.  Derived classes can override this
	 * if they need custom behaviour.
	 */
	void get_default_column(ULONG* sort_out, ULONG* display_out)
	{
		*sort_out = 0;
		*display_out = 0;
	}

	/**
	 * Default UI state (hidden etc.) and type (string, integer, etc.) for the
	 * column specified by column_index.
	 *
	 * @implementing folder_base_interface2
	 */
	SHCOLSTATEF get_default_column_state(UINT column_index)
	{
		ColumnType col(column_index);
		return col.state();
	}
	
	/**
	 * Detailed information about an item in a folder.
	 *
	 * The desired detail is specified by PROPERTYKEY.
	 *
	 * @implementing folder_base_interface2
	 */
	VARIANT get_details_ex(PCUITEMID_CHILD pidl, const SHCOLUMNID* pscid)
	{
		if (::ILIsEmpty(pidl))
			BOOST_THROW_EXCEPTION(comet::com_error(E_INVALIDARG));

		return property(*pscid, pidl).detach();
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
	 * Determine the relative order of two file objects or folders.
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
	 *
	 * If compare_all_fields is false, the PIDLs are compared by the
	 * data that corresponds to the given column index.  Otherwise, the PIDLs
	 * are compared by all the data the contain.
	 *
	 * @todo  We aren't actually comparing raw PIDLs here when
	 *        compare_all_fields is true.  We should be.
	 *
	 * @todo  Do something with canonical flag.
	 */
	virtual int compare_pidls(
		PCUITEMID_CHILD pidl1, PCUITEMID_CHILD pidl2,
		int column, bool compare_all_fields, bool /*canonical*/) const
		// TODO: This should be a PURE virtual method.  We're putting the
		//       impl here temporarily.  Move it somewhere better!
	{
		if (compare_all_fields)
		{
			// FIXME:  This should ignore columns completely and do a raw PIDL
			//         comparison
			try
			{
				int i = 0;
				while (true)
				{
					int result = compare_pidls(pidl1, pidl2, i, false, false);
					if (result != 0)
						return result;
				}
			}
			catch (const std::range_error&)
			{
				return 0;
			}
		}
		else
		{
			ColumnType col(column);
			return col.compare(pidl1, pidl2);
		}
	}

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
	 * The pidl passed to this method will be the @b child item whose folder
	 * is being requested.  It may not end in one of our own PIDLs if this
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
	virtual ATL::CComPtr<IShellFolder> subfolder(
		const winapi::shell::pidl::cpidl_t& pidl) = 0;

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
			BOOST_THROW_EXCEPTION(comet::com_error(hr));

		ATL::CComPtr<IUnknown> object;
		hr = subfolder->CreateViewObject(
			hwnd, riid, reinterpret_cast<void**>(&object));
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(comet::com_error(hr));

		return object;
	}

	winapi::shell::pidl::apidl_t m_root_pidl;
};

}}} // namespace swish::shell_folder::folder
