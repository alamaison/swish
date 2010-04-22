/**
    @file

    Base-class implementing functionality common to all Swish folders

    @if licence

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "Folder.h" // Superclass

#include "swish/atl.hpp" // Common ATL setup
#include "swish/catch_com.hpp" // catchCom
#include "swish/debug.hpp" // METHOD_TRACE
#include "swish/exception.hpp"  // com_exception

#include <cassert> // assert

namespace swish {
namespace shell_folder {
namespace folder {

class CSwishFolder : public swish::shell_folder::folder::CFolder
{
public:

	BEGIN_COM_MAP(CSwishFolder)
		COM_INTERFACE_ENTRY(IShellFolder)
		COM_INTERFACE_ENTRY_CHAIN(CFolder)
	END_COM_MAP()

protected:

	/**
	 * Create one of the objects associated with the current folder.
	 *
	 * Currently, only requests for the current objects are displatched to the
	 * subclasses:
	 * - IShellView
	 * - IShellDetails
	 * - IDropTarget
	 */
	ATL::CComPtr<IUnknown> folder_object(HWND hwnd, REFIID riid)
	{
		ATL::CComPtr<IUnknown> object;

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
		else if (riid == __uuidof(IExplorerCommandProvider))
		{
			object = command_provider(hwnd);
		}

		// QueryInterface could fail at any point above and it *doesn't* throw
		// an exception.  We have to check for NULL once we are sure it can't
		// fail again: IUnknown returned as IUnknown shouldn't be able to fail.
		if (!object)
			throw swish::exception::com_exception(E_NOINTERFACE);

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
	ATL::CComPtr<IUnknown> folder_item_object(
		HWND hwnd, REFIID riid, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl)
	{
		assert(cpidl > 0);

		ATL::CComPtr<IUnknown> object;

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
			throw swish::exception::com_exception(E_NOINTERFACE);

		return object;
	}

	/** @name Objects associated with the current folder. */
	// @{

	/**
	 * Caller has requested the IShellView object associated with this folder.
	 */
	virtual ATL::CComPtr<IShellView> folder_view(HWND hwnd)
	{
		TRACE("Request: IShellView");

		SFV_CREATE sfvdata = { sizeof(sfvdata), 0 };

		// Create a pointer to this IShellFolder to pass to view
		ATL::CComPtr<IShellFolder> this_folder = this;
		ATLENSURE_THROW(this_folder, E_NOINTERFACE);
		sfvdata.pshf = this_folder;

		// Get the callback object for this folder view, if any.
		// Must hold reference to it in this CComPtr over the
		// ::SHCreateShellFolderView() call in case folder_view_callback()
		// also creates it (hands back the only pointer to it).
		ATL::CComPtr<IShellFolderViewCB> callback = folder_view_callback(hwnd);
		sfvdata.psfvcb = callback;

		// Create Default Shell Folder View object
		ATL::CComPtr<IShellView> view;
		HRESULT hr = ::SHCreateShellFolderView(&sfvdata, &view);
		if (FAILED(hr))
			throw swish::exception::com_exception(hr);

		return view;
	}

	/**
	 * Caller has requested the IShellDetail object associated with this
	 * folder.
	 *
	 * By default, that is this folder itself.
	 */
	virtual ATL::CComPtr<IShellDetails> shell_details(HWND /*hwnd*/)
	{
		TRACE("Request: IShellDetails");

		return this;
	}

	/** Create a drop target handler for the folder. */
	virtual ATL::CComPtr<IDropTarget> drop_target(HWND /*hwnd*/)
	{
		TRACE("Request: IDropTarget");
		throw swish::exception::com_exception(E_NOINTERFACE);
	}

	/** Create a toolbar command provider for the folder. */
	virtual ATL::CComPtr<IExplorerCommandProvider> command_provider(
		HWND /*hwnd*/)
	{
		TRACE("Request: IExplorerCommandProvider");
		throw swish::exception::com_exception(E_NOINTERFACE);
	}

	// @}


	/** @name Objects associated with items contained in folder. */
	// @{

	/** Create an icon extraction helper object for the selected item. */
	virtual ATL::CComPtr<IExtractIconW> extract_icon_w(
		HWND /*hwnd*/, PCUITEMID_CHILD /*pidl*/)
	{
		TRACE("Request: IExtractIconW");
		throw swish::exception::com_exception(E_NOINTERFACE);
	}

	/**
	 * Create an icon extraction helper object for the selected item. 
	 * This is the ASCII version of the interface and, by default, requests are
	 * delegated to the same object as IExtractIconW.  Override this to
	 * change the behaviour.
	 */
	virtual ATL::CComPtr<IExtractIconA> extract_icon_a(
		HWND hwnd, PCUITEMID_CHILD pidl)
	{
		TRACE("Request: IExtractIconA");
		ATL::CComPtr<IExtractIconA> extractor;
		extractor = extract_icon_w(hwnd, pidl);
		return extractor;
	}

	/** Create a context menu for the selected items. */
	virtual ATL::CComPtr<IContextMenu> context_menu(
		HWND /*hwnd*/, UINT /*cpidl*/, PCUITEMID_CHILD_ARRAY /*apidl*/)
	{
		TRACE("Request: IContextMenu");
		throw swish::exception::com_exception(E_NOINTERFACE);
	}

	/** Create a file association handler for the selected items. */
	virtual ATL::CComPtr<IQueryAssociations> query_associations(
		HWND /*hwnd*/, UINT /*cpidl*/, PCUITEMID_CHILD_ARRAY /*apidl*/)
	{
		TRACE("Request: IQueryAssociations");
		throw swish::exception::com_exception(E_NOINTERFACE);
	}

	/** Create a data object for the selected items. */
	virtual ATL::CComPtr<IDataObject> data_object(
		HWND /*hwnd*/, UINT /*cpidl*/, PCUITEMID_CHILD_ARRAY /*apidl*/)
	{
		TRACE("Request: IDataObject");
		throw swish::exception::com_exception(E_NOINTERFACE);
	}

	/**
	 * Return any folder view callback object that should be used when creating
	 * the default view.
	 */
	virtual ATL::CComPtr<IShellFolderViewCB> folder_view_callback(
		HWND /*hwnd*/)
	{
		return NULL;
	}
	
	// @}
};

}}} // namespace swish::shell_folder::folder
