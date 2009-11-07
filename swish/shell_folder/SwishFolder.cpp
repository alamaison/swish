/**
    @file

    Base-class implementing functionality common to all Swish folders.

    @if licence

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "SwishFolder.hpp"

#include "swish/debug.hpp"
#include "swish/catch_com.hpp"
#include "swish/exception.hpp"

using ATL::CComPtr;

using swish::exception::com_exception;

namespace swish {
namespace shell_folder {
namespace folder {

/**
 * Create one of the objects associated with the current folder.
 *
 * Currently, only requests for the current objects are displatched to the
 * subclasses:
 * - IShellView
 * - IShellDetails
 * - IDropTarget
 */
CComPtr<IUnknown> CSwishFolder::folder_object(HWND hwnd, REFIID riid)
{
	CComPtr<IUnknown> object;

	if (riid == __uuidof(IShellView))
	{
		object = folder_view(hwnd);
	}
	else if (riid == __uuidof(IShellDetails))
	{
		object = shell_details(hwnd);
	}
	else if (riid == __uuidof(IDropTarget))
	{
		object = drop_target(hwnd);
	}

	// QueryInterface could fail at any point above and it *doesn't* throw
	// an exception.  We have to check for NULL once we are sure it can't
	// fail again: IUnknown returned as IUnknown shouldn't be able to fail.
	if (!object)
		throw com_exception(E_NOINTERFACE);

	return object;
}

/**
 * Create one of the objects associated with an item in the current folder.
 *
 * Currently, only requests for the current objects are displatched to the
 * subclasses:
 * - IContextMenu
 * - IDataObject
 * - IQueryAssociations
 * - IExtractIconW/IExtractIconA
 */
CComPtr<IUnknown> CSwishFolder::folder_item_object(
	HWND hwnd, REFIID riid, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl)
{
	assert(cpidl > 0);

	CComPtr<IUnknown> object;

	if (riid == __uuidof(IContextMenu))
	{
		object = context_menu(hwnd, cpidl, apidl);
	}
	else if (riid == __uuidof(IDataObject))
	{
		object = data_object(hwnd, cpidl, apidl);
	}
	else if (riid == __uuidof(IQueryAssociations))
	{
		object = query_associations(hwnd, cpidl, apidl);
	}
	else if (riid == __uuidof(IExtractIconW))
	{
		assert(cpidl == 1);
		if (cpidl == 1)
			object = extract_icon_w(hwnd, apidl[0]);
	}
	else if (riid == __uuidof(IExtractIconA))
	{
		assert(cpidl == 1);
		if (cpidl == 1)
			object = extract_icon_a(hwnd, apidl[0]);
	}
	
	// QueryInterface could fail at any point above and it *doesn't* throw
	// an exception.  We have to check for NULL once we are sure it can't
	// fail again: IUnknown returned as IUnknown shouldn't be able to fail.
	if (!object)
		throw com_exception(E_NOINTERFACE);

	return object;
}

/**
 * Caller has requested the IShellView object associated with this folder.
 */
CComPtr<IShellView> CSwishFolder::folder_view(HWND hwnd)
{
	TRACE("Request: IShellView");

	SFV_CREATE sfvdata = { sizeof(sfvdata), 0 };

	// Create a pointer to this IShellFolder to pass to view
	CComPtr<IShellFolder> this_folder = this;
	ATLENSURE_THROW(this_folder, E_NOINTERFACE);
	sfvdata.pshf = this_folder;

	// Get the callback object for this folder view, if any.
	// Must hold reference to it in this CComPtr over the
	// ::SHCreateShellFolderView() call in case folder_view_callback()
	// also creates it (hands back the only pointer to it).
	CComPtr<IShellFolderViewCB> callback = folder_view_callback(hwnd);
	sfvdata.psfvcb = callback;

	// Create Default Shell Folder View object
	CComPtr<IShellView> view;
	HRESULT hr = ::SHCreateShellFolderView(&sfvdata, &view);
	if (FAILED(hr))
		throw com_exception(hr);

	return view;
}

/**
 * Caller has requested the IShellDetail object associated with this folder.
 *
 * By default, that is this folder itself.
 */
CComPtr<IShellDetails> CSwishFolder::shell_details(HWND /*hwnd*/)
{
	TRACE("Request: IShellDetails");

	return this;
}

/** Create a drop target handler for the folder. */
CComPtr<IDropTarget> CSwishFolder::drop_target(HWND /*hwnd*/)
{
	TRACE("Request: IDropTarget");
	throw com_exception(E_NOINTERFACE);
}

/** Create an icon extraction helper object for the selected item. */
CComPtr<IExtractIconW> CSwishFolder::extract_icon_w(
	HWND /*hwnd*/, PCUITEMID_CHILD /*pidl*/)
{
	TRACE("Request: IExtractIconW");
	throw com_exception(E_NOINTERFACE);
}

/**
 * Create an icon extraction helper object for the selected item. 
 * This is the ASCII version of the interface and, by default, requests are
 * delegated to the same object as IExtractIconW.  Override this to
 * change the behaviour.
 */
CComPtr<IExtractIconA> CSwishFolder::extract_icon_a(
	HWND hwnd, PCUITEMID_CHILD pidl)
{
	TRACE("Request: IExtractIconA");
	CComPtr<IExtractIconA> extractor;
	extractor = extract_icon_w(hwnd, pidl);
	return extractor;
}

/** Create a context menu for the selected items. */
CComPtr<IContextMenu> CSwishFolder::context_menu(
	HWND /*hwnd*/, UINT /*cpidl*/, PCUITEMID_CHILD_ARRAY /*apidl*/)
{
	TRACE("Request: IContextMenu");
	throw com_exception(E_NOINTERFACE);
}

/** Create a file association handler for the selected items. */
CComPtr<IQueryAssociations> CSwishFolder::query_associations(
	HWND /*hwnd*/, UINT /*cpidl*/, PCUITEMID_CHILD_ARRAY /*apidl*/)
{
	TRACE("Request: IQueryAssociations");
	throw com_exception(E_NOINTERFACE);
}

/** Create a data object for the selected items. */
CComPtr<IDataObject> CSwishFolder::data_object(
	HWND /*hwnd*/, UINT /*cpidl*/, PCUITEMID_CHILD_ARRAY /*apidl*/)
{
	TRACE("Request: IDataObject");
	throw com_exception(E_NOINTERFACE);
}

/**
 * Return any folder view callback object that should be used when creating
 * the default view.
 */
CComPtr<IShellFolderViewCB> CSwishFolder::folder_view_callback(HWND /*hwnd*/)
{
	return NULL;
}

}}} // namespace swish::shell_folder::folder
