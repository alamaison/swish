/**
    @file

    C++ interfaces for folder COM-interface adapters.

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

#ifndef WINAPI_SHELL_FOLDER_INTERFACES_HPP
#define WINAPI_SHELL_FOLDER_INTERFACES_HPP
#pragma once

namespace winapi {
namespace shell {

/**
 * Protected C++ interface which subclasses of @c folder_error_adapter and
 * @c folder2_error_adapter must implement.
 *
 * The methods in this interface are exception-safe versions of the
 * corresponding CamelCased methods in @c IShellFolder.
 *
 * Implementations of these methods are allowed to throw any exceptions
 * derived from @c std::exception.  Where sensible, parameters which would have
 * been [out]-parameters in the raw @c IShellFolder are changed to return
 * values in this interface.  All other aspects of implementation should match
 * the documentation of @c IShellFolder unless otherwise stated.
 */
class folder_base_interface
{
public:
	virtual PIDLIST_RELATIVE parse_display_name(
		HWND hwnd, IBindCtx* bind_ctx, const wchar_t* display_name,
		ULONG* attributes_inout) = 0;

	virtual IEnumIDList* enum_objects(HWND hwnd, SHCONTF flags) = 0;
	
	virtual void bind_to_object(
		PCUIDLIST_RELATIVE pidl, IBindCtx* bind_ctx, const IID& iid,
		void** interface_out) = 0;

	virtual void bind_to_storage(
		PCUIDLIST_RELATIVE pidl, IBindCtx* bind_ctx, const IID& iid,
		void** interface_out) = 0;

	/**
	 * Determine the relative order of two items in or below this folder.
	 *
	 * @returns
	 * - Negative: pidl1 < pidl2
	 * - Positive: pidl1 > pidl2
	 * - Zero:     pidl1 == pidl2
	 */
	virtual int compare_ids(
		LPARAM lparam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2) = 0;

	virtual void create_view_object(
		HWND hwnd_owner, const IID& iid, void** interface_out) = 0;
	
	virtual void get_attributes_of(
		UINT pidl_count, PCUITEMID_CHILD_ARRAY pidl_array,
		SFGAOF* attributes_inout) = 0;

	virtual void get_ui_object_of(
		HWND hwnd_owner, UINT pidl_count, PCUITEMID_CHILD_ARRAY pidl_array,
		const IID& iid, void** interface_out) = 0;

	virtual STRRET get_display_name_of(
		PCUITEMID_CHILD pidl, SHGDNF flags) = 0;

	virtual PITEMID_CHILD set_name_of(
		HWND hwnd, PCUITEMID_CHILD pidl, const wchar_t* name,
		SHGDNF flags) = 0;
};

/**
 * Protected C++ interface which subclasses of @c folder2_error_adapter must
 * implement.
 *
 * The methods in this interface are exception-safe versions of the
 * corresponding CamelCased methods in @c IShellFolder2.
 *
 * Implementations of these methods are allowed to throw any exceptions
 * derived from @c std::exception.  Where sensible, parameters which would have
 * been [out]-parameters in the raw @c IShellFolder2 are changed to return
 * values in this interface.  All other aspects of implementation should
 * match the documentation of @c IShellFolder2 unless otherwise stated.
 */
class folder2_base_interface
{
public:

	/**
	 * GUID of the search to invoke when the user clicks on the search
	 * toolbar button.
	 */
	virtual GUID get_default_search_guid() = 0;

	/**
	 * Enumeration of all searches supported by this folder
	 */
	virtual IEnumExtraSearch* enum_searches() = 0;

	/**
	 * Default sorting and display columns.
	 */
	virtual void get_default_column(ULONG* sort_out, ULONG* display_out) = 0;

	/**
	 * Default UI state (hidden etc.) and type (string, integer, etc.) for the
	 * column specified by column_index.
	 */
	virtual SHCOLSTATEF get_default_column_state(UINT column_index) = 0;

	/**
	 * Detailed information about an item in a folder.
	 *
	 * The desired detail is specified by PROPERTYKEY.
	 */
	virtual VARIANT get_details_ex(
		PCUITEMID_CHILD pidl, const SHCOLUMNID* property_key) = 0;

	/**
	 * Detailed information about an item in a folder.
	 *
	 * The desired detail is specified by a column index.
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
	virtual SHELLDETAILS get_details_of(
		PCUITEMID_CHILD pidl, UINT column_index) = 0;

	/**
	 * Convert column index to matching PROPERTYKEY, if any.
	 */
	virtual SHCOLUMNID map_column_to_scid(UINT column_index) = 0;
};

/**
 * Protected C++ interface which subclasses of @c shell_details_error_adapter
 * must implement.
 *
 * The methods in this interface are exception-safe versions of the
 * corresponding CamelCased methods in @c IShellDetails.
 *
 * Implementations of these methods are allowed to throw any exceptions
 * derived from @c std::exception.  All other aspects of implementation should
 * match the documentation of @c IShellDetails unless otherwise stated.
 */
class shell_details_base_interface
{
public:
	
	virtual SHELLDETAILS get_details_of(
		PCUITEMID_CHILD pidl, UINT column_index) = 0;

	virtual bool column_click(UINT column_index) = 0;
};

}} // namespace winapi::shell

#endif
