/**
    @file

    Translate between COM errors and C++ exceptions for folder interfaces.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef WINAPI_SHELL_FOLDER_ERROR_ADAPTERS_HPP
#define WINAPI_SHELL_FOLDER_ERROR_ADAPTERS_HPP
#pragma once

#include "folder_interfaces.hpp" // folder_base_interface,
                                 // folder2_base_interface,
                                 // shell_details_base_interface

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <comet/error.h> // com_error
#include <comet/interface.h> // comtype

#include <boost/static_assert.hpp> // BOOST_STATIC_ASSERT
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/type_traits/is_base_of.hpp> // is_base_of

#include <cassert> // assert
#include <cstring> // memset

#include <Shlobj.h> // IShellDetails
#include <ShObjIdl.h> // IShellFolder, IShellFolder2

/**
 * @file
 * The adapter classes in this file reduce the amount of work needed to
 * implement a shell folder by handling the translation of C++ exceptions to
 * COM error codes for common shell interfaces.
 *
 * Each adapter has a public COM interface which it implements and a
 * protected C++ interface which it doesn't.  For every COM method, they call
 * an corresponding method of the protected interface. Subclass the adapter
 * and implement the protected interface to create a concrete COM object.
 * The C++ methods are free to throw any exceptions derived from std::exception
 * as well as any that you handle using your own @c comet_exception_handler
 * override.
 *
 * The adapters ensure that the final COM objects obey COM rules in several
 * ways:
 *
 * - On entry to a COM method they first clear any [out]-parameters.  This is
 *   required by COM rules so that, for example, cross-apartment marshalling
 *   doesn't try to marshal uninitialised memory (see item 19 of 
 *   'Effective Com').
 * - If certain required parameters are missing, they immediately return a
 *   COM error without calling the inner C++ method.
 * - Then they call the inner method.
 * - Catch any exception, calling @c SetErrorInfo with as much detail as
 *   we have available, and translate it to a COM @c HRESULT.
 * - Return the HRESULT or set the [out]-params if the inner function didn't
 *   throw.
 *
 * As the return-values are no longer being used for error codes, the protected
 * methods are sometimes changed to return a value directly instead of using
 * an [out]-parameter.
 *
 * Although the adapters make use of Comet, they do not have to be instantiated
 * as Comet objects.  They work just as well using ATL::CComObject.
 */

/**
 * Comet IID lookup for IShellFolder.
 *
 * Allows folder_error_adapter to be used as a base for Comet objects.
 */
template<> struct ::comet::comtype<::IShellFolder>
{
	static const ::IID& uuid() throw() { return ::IID_IShellFolder; }
	typedef ::IUnknown base;
};

/**
 * Comet IID lookup for IShellDetails.
 *
 * Allows shell_details_error_adapter to be used as a base for Comet objects.
 */
template<> struct ::comet::comtype<::IShellDetails>
{
	static const ::IID& uuid() throw() { return ::IID_IShellDetails; }
	typedef ::IUnknown base;
};

/**
 * Comet IID lookup for IShellFolder2.
 *
 * Allows folder2_error_adapter to be used as a base for Comet objects.
 */
template<> struct ::comet::comtype<::IShellFolder2>
{
	static const ::IID& uuid() throw() { return ::IID_IShellFolder2; }
	typedef ::IShellFolder base;
};

namespace winapi {
namespace shell {

/**
 * Exception translation for methods common to @c IShellFolder
 * and @c IShellFolder2
 *
 * Only error translation code should be included in this class.  Any
 * further C++erisation such as translating to C++ datatypes must be done in
 * by the subclasses.
 */
template<typename Interface>
class folder_error_adapter_base :
	public Interface,
	protected folder_base_interface
{
	BOOST_STATIC_ASSERT((boost::is_base_of<IShellFolder, Interface>::value));

public:

	typedef IShellFolder interface_is;

	virtual IFACEMETHODIMP ParseDisplayName(
		HWND hwnd, IBindCtx* pbc, LPWSTR pszDisplayName, ULONG* /*pchEaten*/,
		PIDLIST_RELATIVE* ppidl, ULONG* pdwAttributes)
	{
		try
		{
			if (!ppidl)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*ppidl = NULL;

			if (!pszDisplayName)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			// Use a temporary to store the attributes so that the inner
			// method doesn't have to check whether the caller wanted them.
			// We store them back later if they did.
			ULONG dwAttributes = (pdwAttributes) ? *pdwAttributes : 0;
			*ppidl = parse_display_name(
				hwnd, pbc, pszDisplayName, &dwAttributes);

			assert(*ppidl || !"No error but no retval");

			if (pdwAttributes)
				*pdwAttributes = dwAttributes;
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP EnumObjects(
		HWND hwnd, SHCONTF grfFlags, IEnumIDList** ppenumIDList)
	{
		try
		{
			if (!ppenumIDList)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*ppenumIDList = NULL;

			*ppenumIDList = enum_objects(hwnd, grfFlags);

			// If the implementation returns NULL, we interpret it to mean
			// no items in the folder match the given query flags
			if (!*ppenumIDList)
				return S_FALSE;
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	/**
	 * Caller is requesting a subobject of this folder.
	 *
	 * @implementing IShellFolder
	 *
	 * Create and initialise an instance of the subitem represented by @a pidl
	 * and return the interface asked for in @a riid.
	 *
	 * Typically this is an IShellFolder although it may be an IStream.  
	 * Whereas CreateViewObject() and GetUIObjectOf() request 'associated
	 * objects' of items in the hierarchy, calls to BindToObject()
	 * are for the objects representing the items themselves.  E.g,
	 * IShellFolder for folders and IStream for files.
	 *
	 * @param[in]  pidl  PIDL to the requested object @b relative to
	 *                   this folder.
	 * @param[in]  pbc   Binding context.
	 * @param[in]  riid  IID of the interface being requested.
	 * @param[out] ppv   Location in which to return the requested interface.
	 */
	virtual IFACEMETHODIMP BindToObject(
		PCUIDLIST_RELATIVE pidl, IBindCtx* pbc, REFIID riid, void** ppv)
	{
		try
		{
			if (!ppv)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*ppv = NULL;

			bind_to_object(pidl, pbc, riid, ppv);

			assert(*ppv || !"No error but no retval");
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP BindToStorage(
		PCUIDLIST_RELATIVE pidl, IBindCtx* pbc, REFIID riid, void** ppv)
	{
		try
		{
			if (!ppv)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*ppv = NULL;

			bind_to_storage(pidl, pbc, riid, ppv);

			assert(*ppv || !"No error but no retval");
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP CompareIDs(
		LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
	{
		try
		{
			int result = compare_ids(lParam, pidl1, pidl2);

			// The cast to unsigned short is *crucial*!  Without it,
			// sorting in Explorer does all sorts of wierd stuff
			return MAKE_HRESULT(
				SEVERITY_SUCCESS, 0, (unsigned short)result);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	/**
	 * Create an object associated with @b this folder.
	 *
	 * This method is in contrast to GetUIObjectOf() which performs the same
	 * task but for an item contained *within* the current folder rather than
	 * the folder itself.
	 *
	 * @param[in]  hwnd  Handle to the parent window, if any, of any UI that
	 *                    may be needed to complete the request.
	 * @param[in]  riid  Interface UUID for the object being requested.
	 * @param[out] ppv   Return value.
	 */
	virtual IFACEMETHODIMP CreateViewObject(
		HWND hwndOwner, REFIID riid, void** ppv)
	{
		try
		{
			if (!ppv)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*ppv = NULL;

			create_view_object(hwndOwner, riid, ppv);

			assert(*ppv || !"No error but no retval");
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP GetAttributesOf(
		UINT cidl, PCUITEMID_CHILD_ARRAY apidl, SFGAOF* rgfInOut)
	{
		try
		{
			if (!rgfInOut)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			// Use a temporary here so that an implementation can't mess with
			// flags and then change their minds and throw an exception
			SFGAOF flags = *rgfInOut;
			get_attributes_of(cidl, apidl, &flags);
			*rgfInOut = flags;

		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

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
	 * CreateViewObject() performs the same task as GetUIObjectOf() but only
	 * for the folder, not for items within it.
	 *
	 * @param[in]  hwnd  Handle to the parent window, if any, of any UI that
	 *                   may be needed to complete the request.
	 * @param[in]  riid  Interface UUID for the object being requested.
	 * @param[out] ppv   Return value.
	 */
	virtual IFACEMETHODIMP GetUIObjectOf(
		HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
		UINT* /*rgfReserved*/, void** ppv)
	{
		try
		{
			if (!ppv)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*ppv = NULL;

			get_ui_object_of(hwndOwner, cidl, apidl, riid, ppv);

			assert(*ppv || !"No error but no retval");
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP GetDisplayNameOf(
		PCUITEMID_CHILD pidl, SHGDNF uFlags, STRRET* pName)
	{
		try
		{
			if (!pName)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			std::memset(pName, 0, sizeof(STRRET));

			if (!pidl)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			*pName = get_display_name_of(pidl, uFlags);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP SetNameOf(
		HWND hwnd, PCUITEMID_CHILD pidl, LPCWSTR pszName, SHGDNF uFlags,
		PITEMID_CHILD* ppidlOut)
	{
		try
		{
			if (!ppidlOut)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*ppidlOut = NULL;

			if (!pidl)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			if (!pszName)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			*ppidlOut = set_name_of(hwnd, pidl, pszName, uFlags);

			assert(*ppidlOut || !"No error but no retval");
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}
};

/**
 * @c IShellFolder implementation outer layer that translates exceptions.
 *
 * Abstract class that translates C++ exceptions thrown by its implementation
 * methods to COM error codes used in the raw @c IShellFolder interface.
 *
 * Subclass this adapter and implement @c folder_base_interface to get a
 * COM component supporting @c IShellFolder.
 *
 * Because the IShellFolder implementation is shared by the @c IShellFolder2
 * adapter yet we must derive from one interface *not both*, the adapter
 * implementation is in @c folder_error_adapter_base.  We subclass it to make
 * a definite @c IShellFolder adapter (rather than @c IShellFolderX).
 */
class folder_error_adapter : public folder_error_adapter_base<IShellFolder> {};

/**
 * @c IShellFolder2 implementation outer layer that translates exceptions.
 *
 * Abstract class that translates C++ exceptions thrown by its implementation
 * methods to COM error codes used in the raw @c IShellFolder2 interface.
 *
 * Subclass this adapter and implement @c folder_base_interface and
 * @c folder2_base_interface to get a COM component supporting
 * @c IShellFolder2.
 *
 * Only error translation code should be included in this class.  Any
 * further C++erisation such as translating to C++ datatypes must be done in
 * by the subclasses.
 */
class folder2_error_adapter : 
	public folder_error_adapter_base<IShellFolder2>,
	protected folder2_base_interface
{
public:

	typedef IShellFolder2 interface_is;

	/**
	 * Return GUID of the search to invoke when the user clicks on the search
	 * toolbar button.
	 */
	virtual IFACEMETHODIMP GetDefaultSearchGUID(GUID* pguid)
	{
		try
		{
			if (!pguid)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			std::memset(pguid, 0, sizeof(GUID));

			*pguid = get_default_search_guid();
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	/**
	 * Return enumeration of all searches supported by this folder
	 */
	virtual IFACEMETHODIMP EnumSearches(IEnumExtraSearch** ppenum)
	{
		try
		{
			if (!ppenum)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*ppenum = NULL;

			*ppenum = enum_searches();

			assert(*ppenum || !"No error but no retval");
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	/**
	 * Default sorting and display columns.
	 */
	virtual IFACEMETHODIMP GetDefaultColumn(
		DWORD /*dwRes*/, ULONG* pSort, ULONG* pDisplay)
	{
		try
		{
			if (pSort)
				*pSort = 0;

			if (pDisplay)
				*pDisplay = 0;

			if (!pSort || !pDisplay)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			ULONG sort = 0;
			ULONG display = 0;
			get_default_column(&sort, &display);
			*pSort = sort;
			*pDisplay = display;
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	/**
	 * Default UI state (hidden etc.) and type (string, integer, etc.) for the
	 * column specified by iColumn.
	 */
	virtual IFACEMETHODIMP GetDefaultColumnState(
		UINT iColumn, SHCOLSTATEF* pcsFlags)
	{
		try
		{
			if (!pcsFlags)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*pcsFlags = 0;

			*pcsFlags = get_default_column_state(iColumn);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	/**
	 * Detailed information about an item in a folder.
	 *
	 * The desired detail is specified by PROPERTYKEY.
	 */
	virtual IFACEMETHODIMP GetDetailsEx(
		PCUITEMID_CHILD pidl, const SHCOLUMNID* pscid, VARIANT* pv)
	{
		try
		{
			if (!pv)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			::VariantClear(pv);

			if (!pidl)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			if (!pscid)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			*pv = get_details_ex(pidl, pscid);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	/**
	 * Detailed information about an item in a folder.
	 *
	 * The desired detail is specified by a column index.
	 *
	 * @note  This method is present in @c IShellDetails as well as
	 *        @IShellFolder2.
	 *
	 * This function operates in two distinctly different ways:
	 *  - if pidl is NULL:
	 *       Retrieve the the names of the columns themselves.
	 *  - if pidl is not NULL:
	 *       Retrieve  information for the item in the given pidl.
	 *
	 * The caller indicates which detail they want by specifying a column index
	 * in column_index.  If this column does not exist, return an error.
	 *
	 * @retval  A SHELLDETAILS structure holding the requested detail as a
	 *          string along with various metadata.
	 *
	 * @note  Typically, a folder view calls this method repeatedly,
	 *        incrementing the column index each time.  The first column for
	 *        which we return an error, marks the end of the columns in this
	 *        folder.
	 */
	virtual IFACEMETHODIMP GetDetailsOf(
		PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS* psd)
	{
		try
		{
			if (!psd)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			std::memset(psd, 0, sizeof(SHELLDETAILS));

			*psd = get_details_of(pidl, iColumn);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid)
	{
		try
		{
			if (!pscid)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			std::memset(pscid, 0, sizeof(SHCOLUMNID));

			*pscid = map_column_to_scid(iColumn);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}
};

/**
 * @c IShellDetails implementation outer layer that translates exceptions.
 *
 * Abstract class that translates C++ exceptions thrown by its implementation
 * methods to COM error codes used in the raw @c IShellDetails interface.
 *
 * Subclass this adapter and implement @c shell_details_base_interface to get a
 * COM component supporting @c IShellDetails.
 *
 * Only error translation code should be included in this class.  Any
 * further C++erisation such as translating to C++ datatypes must be done in
 * by the subclasses.
 */
class shell_details_error_adapter :
	public IShellDetails,
	protected shell_details_base_interface
{
public:

	typedef IShellDetails interface_is;

	/**
	 * Detailed information about an item in a folder.
	 *
	 * The desired detail is specified by a column index.
	 *
	 * @note  This method is present in @c IShellDetails as well as
	 *        @IShellFolder2.
	 *
	 * This function operates in two distinctly different ways:
	 *  - if pidl is NULL:
	 *       Retrieve the the names of the columns themselves.
	 *  - if pidl is not NULL:
	 *       Retrieve  information for the item in the given pidl.
	 *
	 * The caller indicates which detail they want by specifying a column index
	 * in @a iColumn.  If this column does not exist, return an error.
	 *
	 * @retval  A SHELLDETAILS structure holding the requested detail as a
	 *          string along with various metadata.
	 *
	 * @note  Typically, a folder view calls this method repeatedly,
	 *        incrementing the column index each time.  The first column for
	 *        which we return an error, marks the end of the columns in this
	 *        folder.
	 */
	virtual IFACEMETHODIMP GetDetailsOf(
		PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS* psd)
	{
		try
		{
			if (!psd)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			std::memset(psd, 0, sizeof(SHELLDETAILS));

			*psd = get_details_of(pidl, iColumn);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP ColumnClick(UINT iColumn)
	{
		try
		{
			if (!column_click(iColumn))
				return S_FALSE;
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}
};

}} // namespace winapi::shell

#endif
