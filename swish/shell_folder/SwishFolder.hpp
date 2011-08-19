/**
    @file

    Base-class implementing functionality common to all Swish folders

    @if license

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
#include "swish/debug.hpp" // METHOD_TRACE

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <comet/error.h> // com_error

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert

namespace swish {
namespace shell_folder {
namespace folder {

template<typename ColumnType>
class CSwishFolder : public swish::shell_folder::folder::CFolder<ColumnType>
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
	 * Currently, only requests for the current objects are dispatched to the
	 * subclasses:
	 * - IShellView
	 * - IShellDetails
	 * - IDropTarget
	 * - IExplorerCommandProvider
	 * - IContextMenu
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
		else if (riid == __uuidof(IContextMenu))
		{
			object = background_context_menu(hwnd);
		}
		else if (riid == __uuidof(IResolveShellLink))
		{
			assert(false);
		}

		// QueryInterface could fail at any point above and it *doesn't* throw
		// an exception.  We have to check for NULL once we are sure it can't
		// fail again: IUnknown returned as IUnknown shouldn't be able to fail.
		if (!object)
			BOOST_THROW_EXCEPTION(comet::com_error(E_NOINTERFACE));

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
		else if (riid == __uuidof(IShellLinkW))
		{
			assert(cpidl == 1);
			if (cpidl == 1)
				object = shell_link_w(hwnd, apidl[0]);
		}
		else if (riid == __uuidof(IShellLinkA))
		{
			assert(cpidl == 1);
			if (cpidl == 1)
				object = shell_link_a(hwnd, apidl[0]);
		}
		else if (riid == __uuidof(IResolveShellLink))
		{
			assert(false);
		}
		
		// QueryInterface could fail at any point above and it *doesn't* throw
		// an exception.  We have to check for NULL once we are sure it can't
		// fail again: IUnknown returned as IUnknown shouldn't be able to fail.
		if (!object)
			BOOST_THROW_EXCEPTION(comet::com_error(E_NOINTERFACE));

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
			BOOST_THROW_EXCEPTION(comet::com_error(hr));

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
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOINTERFACE));
		return NULL;
	}

	/** Create a toolbar command provider for the folder. */
	virtual ATL::CComPtr<IExplorerCommandProvider> command_provider(
		HWND /*hwnd*/)
	{
		TRACE("Request: IExplorerCommandProvider");
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOINTERFACE));
		return NULL;
	}

	/**
	 * Create a context menu for the folder background.
	 * Pasting into a Swish window requires this.
	 */
	virtual ATL::CComPtr<IContextMenu> background_context_menu(HWND /*hwnd*/)
	{
		TRACE("Request: IContextMenu");
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOINTERFACE));
		return NULL;
	}

	// @}


	/** @name Objects associated with items contained in folder. */
	// @{

	/** Create an icon extraction helper object for the selected item. */
	virtual ATL::CComPtr<IExtractIconW> extract_icon_w(
		HWND /*hwnd*/, PCUITEMID_CHILD /*pidl*/)
	{
		TRACE("Request: IExtractIconW");
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOINTERFACE));
		return NULL;
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

	/** Create a link resolver for the given item. */
	virtual ATL::CComPtr<IShellLinkW> shell_link_w(
		HWND /*hwnd*/, PCUITEMID_CHILD /*pidl*/)
	{
		TRACE("Request: IShellLinkW");
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOINTERFACE));
		return NULL;
	}

	/** Create a link resolver (ANSI) for the given item. */
	virtual ATL::CComPtr<IShellLinkA> shell_link_a(
		HWND /*hwnd*/, PCUITEMID_CHILD /*pidl*/)
	{
		TRACE("Request: IShellLinkA");
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOINTERFACE));
		return NULL;
	}

	/** Create a context menu for the selected items. */
	virtual ATL::CComPtr<IContextMenu> context_menu(
		HWND /*hwnd*/, UINT /*cpidl*/, PCUITEMID_CHILD_ARRAY /*apidl*/)
	{
		TRACE("Request: IContextMenu");
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOINTERFACE));
		return NULL;
	}

	/** Create a file association handler for the selected items. */
	virtual ATL::CComPtr<IQueryAssociations> query_associations(
		HWND /*hwnd*/, UINT /*cpidl*/, PCUITEMID_CHILD_ARRAY /*apidl*/)
	{
		TRACE("Request: IQueryAssociations");
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOINTERFACE));
		return NULL;
	}

	/** Create a data object for the selected items. */
	virtual ATL::CComPtr<IDataObject> data_object(
		HWND /*hwnd*/, UINT /*cpidl*/, PCUITEMID_CHILD_ARRAY /*apidl*/)
	{
		TRACE("Request: IDataObject");
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOINTERFACE));
		return NULL;
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
