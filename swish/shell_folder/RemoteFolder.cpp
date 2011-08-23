/**
    @file

    Explorer folder that handles remote files and folders.

    @if license

    Copyright (C) 2007, 2008, 2009, 2010, 2011
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

#include "RemoteFolder.h"

#include "SftpDirectory.h"
#include "SftpDataObject.h"
#include "IconExtractor.h"
#include "Registry.h"
#include "swish/debug.hpp"
#include "swish/drop_target/SnitchingDropTarget.hpp" // CSnitchingDropTarget
#include "swish/drop_target/DropUI.hpp" // DropUI
#include "swish/frontend/announce_error.hpp" // rethrow_and_announce
#include "swish/remote_folder/columns.hpp" // property_key_from_column_index
#include "swish/remote_folder/commands/commands.hpp"
                                           // remote_folder_command_provider
#include "swish/remote_folder/connection.hpp" // connection_from_pidl
#include "swish/remote_folder/context_menu_callback.hpp"
                                                       // context_menu_callback
#include "swish/remote_folder/properties.hpp" // property_from_pidl
#include "swish/remote_folder/remote_pidl.hpp" // remote_itemid_view
                                               // create_remote_itemid
#include "swish/remote_folder/symlink.hpp" // pidl_to_shell_link
#include "swish/remote_folder/ViewCallback.hpp" // CViewCallback
#include "swish/remote_folder/swish_pidl.hpp" // absolute_path_from_swish_pidl
#include "swish/trace.hpp" // trace
#include "swish/windows_api.hpp" // SHBindToParent

#include <winapi/shell/shell.hpp> // string_to_strret

#include <comet/datetime.h> // datetime_t

#include <boost/bind.hpp> // bind
#include <boost/exception/diagnostic_information.hpp> // diagnostic_information
#include <boost/filesystem/path.hpp> // wpath
#include <boost/locale.hpp> // translate
#include <boost/make_shared.hpp> // make_shared
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <string>

using swish::drop_target::CSnitchingDropTarget;
using swish::drop_target::DropUI;
using swish::frontend::rethrow_and_announce;
using swish::remote_folder::absolute_path_from_swish_pidl;
using swish::remote_folder::commands::remote_folder_command_provider;
using swish::remote_folder::connection_from_pidl;
using swish::remote_folder::context_menu_callback;
using swish::remote_folder::create_remote_itemid;
using swish::remote_folder::CViewCallback;
using swish::remote_folder::pidl_to_shell_link;
using swish::remote_folder::property_from_pidl;
using swish::remote_folder::property_key_from_column_index;
using swish::remote_folder::remote_itemid_view;
using swish::tracing::trace;

using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;
using winapi::shell::pidl::pidl_t;
using winapi::shell::property_key;
using winapi::shell::string_to_strret;

using comet::com_ptr;
using comet::com_error;
using comet::datetime_t;
using comet::throw_com_error;
using comet::variant_t;

using boost::bind;
using boost::filesystem::wpath;
using boost::locale::translate;
using boost::make_shared;

using ATL::CComPtr;

using std::string;
using std::wstring;


namespace comet {

template<> struct comtype<::IContextMenu>
{
	static const ::IID& uuid() throw() { return ::IID_IContextMenu; }
	typedef ::IUnknown base;
};

}


namespace {

	cpidl_t create_filename_only_pidl(const wstring& filename)
	{
		return create_remote_itemid(
			filename, false, false, L"", L"", 0, 0, 0, 0, datetime_t(),
			datetime_t());
	}

	/**
	 * Remove the extension from the remote item's filename *if appropriate*.
	 */
	wstring filename_without_extension(const cpidl_t remote_item)
	{
		remote_itemid_view itemid(remote_item);
		wstring full_name = itemid.filename();

		if (full_name.empty() || itemid.is_folder())
		{
			return full_name;
		}
		else
		{
			if (full_name[0] != L'.')
			{
				return wpath(full_name).stem();
			}
			else
			{
				// File might look something like '.hidden.txt' or it might
				// just be '.hidden'.  In the first case we only want to remove
				// the '.txt' extension.  In the second case we don't want
				// to remove anything.
				wstring bit_after_initial_dot = full_name.substr(1);
				return L'.' + wpath(bit_after_initial_dot).stem();
			}
		}
	}
}

/*--------------------------------------------------------------------------*/
/*      Functions implementing IShellFolder via folder_error_adapter.       */
/*--------------------------------------------------------------------------*/

/**
 * Create an IEnumIDList which enumerates the items in this folder.
 *
 * @implementing folder_error_adapter
 *
 * @param hwnd   Optional window handle used if enumeration requires user
 *               input.
 * @param flags  Flags specifying which types of items to include in the
 *               enumeration. Possible flags are from the @c SHCONT enum.
 */
IEnumIDList* CRemoteFolder::enum_objects(HWND hwnd, SHCONTF flags)
{
	try
	{
		com_ptr<ISftpProvider> provider =
			connection_from_pidl(root_pidl(), hwnd);
		com_ptr<ISftpConsumer> consumer = m_consumer_factory(hwnd);

		// Create directory handler and get listing as PIDL enumeration
		CSftpDirectory directory(root_pidl(), provider, consumer);
		return directory.GetEnum(flags).detach();
	}
	catch (...)
	{
		rethrow_and_announce(
			hwnd, translate("Unable to access the directory"),
			translate("You might not have permission."));
	}
}

/**
 * Convert path string relative to this folder into a PIDL to the item.
 *
 * @implementing folder_error_adapter
 *
 * @todo  Handle the attributes parameter.  Will need to contact server
 * as the PIDL we create is fake and will not have correct folderness, etc.
 */
PIDLIST_RELATIVE CRemoteFolder::parse_display_name(
	HWND hwnd, IBindCtx* bind_ctx, const wchar_t* display_name,
	ULONG* attributes_inout)
{
	try
	{
		trace(__FUNCTION__" called (display_name=%s)") % display_name;
		if (*display_name == L'\0')
			BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

		// The string we are trying to parse should be of the form:
		//    directory/directory/filename
		// or 
		//    filename
		wstring strDisplayName(display_name);

		// May have / to separate path segments
		wstring::size_type nSlash = strDisplayName.find_first_of(L'/');
		wstring strSegment;
		if (nSlash == 0) // Unix machine - starts with folder called /
		{
			strSegment = strDisplayName.substr(0, 1);
		}
		else
		{
			strSegment = strDisplayName.substr(0, nSlash);
		}

		// Create child PIDL for this path segment
		cpidl_t pidl = create_filename_only_pidl(strSegment);

		// Bind to subfolder and recurse if there were other path segments
		if (nSlash != wstring::npos)
		{
			wstring strRest = strDisplayName.substr(nSlash+1);

			com_ptr<IShellFolder> subfolder;
			bind_to_object(
				pidl.get(), bind_ctx, subfolder.iid(),
				reinterpret_cast<void**>(subfolder.out()));

			wchar_t wszRest[MAX_PATH];
			::wcscpy_s(wszRest, ARRAYSIZE(wszRest), strRest.c_str());

			pidl_t rest;
			HRESULT hr = subfolder->ParseDisplayName(
				hwnd, bind_ctx, wszRest, NULL, rest.out(), attributes_inout);
			if (FAILED(hr))
				throw_com_error(subfolder.get(), hr);

			return (pidl + rest).detach();
		}
		else
		{
			return pidl.detach();
		}
	}
	catch (...)
	{
		rethrow_and_announce(
			hwnd, translate("Path not recognised"),
			translate("Check that the path was entered correctly."));
	}
}

/**
 * Retrieve the display name for the specified file object or subfolder.
 *
 * @implementing folder_error_adapter
 */
STRRET CRemoteFolder::get_display_name_of(PCUITEMID_CHILD pidl, SHGDNF flags)
{
	if (::ILIsEmpty(pidl))
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

	wstring name;

	bool fForParsing = (flags & SHGDN_FORPARSING) != 0;

	if (fForParsing || (flags & SHGDN_FORADDRESSBAR))
	{
		if (!(flags & SHGDN_INFOLDER))
		{
			// Bind to parent
			com_ptr<IShellFolder> parent;
			PCUITEMID_CHILD pidlThisFolder = NULL;
			HRESULT hr = swish::windows_api::SHBindToParent(
				root_pidl().get(), parent.iid(),
				reinterpret_cast<void**>(parent.out()), &pidlThisFolder);
			
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(com_error(hr));

			STRRET strret;
			std::memset(&strret, 0, sizeof(strret));
			hr = parent->GetDisplayNameOf(pidlThisFolder, flags, &strret);
			if (FAILED(hr))
				throw_com_error(parent.get(), hr);

			ATLASSERT(strret.uType == STRRET_WSTR);

			name += strret.pOleStr;
			name += L'/';
		}

		// Add child path - include extension if FORPARSING
		if (fForParsing)
			name += remote_itemid_view(pidl).filename();
		else
			name += filename_without_extension(pidl);

	}
	else if (flags & SHGDN_FOREDITING)
	{
		name = remote_itemid_view(pidl).filename();
	}
	else
	{
		ATLASSERT(flags == SHGDN_NORMAL || flags == SHGDN_INFOLDER);

		name = filename_without_extension(pidl);
	}

	return string_to_strret(name);
}

/**
 * Rename item.
 *
 * @implementing folder_error_adapter
 */
PITEMID_CHILD CRemoteFolder::set_name_of(
	HWND hwnd, PCUITEMID_CHILD pidl, const wchar_t* name, SHGDNF /*flags*/)
{
	try
	{
		com_ptr<ISftpProvider> provider =
			connection_from_pidl(root_pidl(), hwnd);
		com_ptr<ISftpConsumer> consumer = m_consumer_factory(hwnd);

		// Rename file
		CSftpDirectory directory(root_pidl(), provider, consumer);
		bool fOverwritten = directory.Rename(pidl, name);

		// Create new PIDL from old one with new filename
		remote_itemid_view itemid(pidl);
		cpidl_t new_file = create_remote_itemid(
			name, itemid.is_folder(), itemid.is_link(),
			itemid.owner(), itemid.group(), itemid.owner_id(),
			itemid.group_id(), itemid.permissions(), itemid.size(),
			itemid.date_modified(), itemid.date_accessed());

		// A failure to notify the shell shouldn't prevent us returning the PIDL
		try
		{
			// Make PIDLs absolute
			apidl_t old_pidl = root_pidl() + pidl;
			apidl_t new_pidl = root_pidl() + new_file;

			// Update the shell by passing both PIDLs
			if (fOverwritten)
			{
				::SHChangeNotify(
					SHCNE_DELETE, SHCNF_IDLIST | SHCNF_FLUSH, new_pidl.get(),
					NULL);
			}
			::SHChangeNotify(
				(remote_itemid_view(pidl).is_folder()) ?
					SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM,
				SHCNF_IDLIST | SHCNF_FLUSH, old_pidl.get(), new_pidl.get());
		}
		catch (const std::exception& e)
		{
			trace("Exception thrown while notifying shell of rename:");
			trace("%s") % boost::diagnostic_information(e);
		}

		return new_file.detach();
	}
	catch (...)
	{
		rethrow_and_announce(
			hwnd, translate("Unable to rename the item"),
			translate("You might not have permission."));
	}
}

/**
 * Returns the attributes for the items whose PIDLs are passed in.
 *
 * @implementing folder_error_adapter
 */
void CRemoteFolder::get_attributes_of(
	UINT pidl_count, PCUITEMID_CHILD_ARRAY pidl_array,
	SFGAOF* attributes_inout)
{
	// Search through all PIDLs and check if they are all folders
	bool fAllAreFolders = true;
	for (UINT i = 0; i < pidl_count; i++)
	{
		if (!remote_itemid_view(pidl_array[i]).is_folder())
		{
			fAllAreFolders = false;
			break;
		}
	}

	// Search through all PIDLs and check if they are all links
	bool fAreAllLinks = true;
	for (UINT i = 0; i < pidl_count; i++)
	{
		if (!remote_itemid_view(pidl_array[i]).is_link())
		{
			fAreAllLinks = false;
			break;
		}
	}

	// Search through all PIDLs and check if they are all 'dot' files
	bool fAllAreDotFiles = true;
	for (UINT i = 0; i < pidl_count; i++)
	{
		wstring filename = remote_itemid_view(pidl_array[i]).filename();
		if (filename[0] != L'.')
		{
			fAllAreDotFiles = false;
			break;
		}
	}

	DWORD dwAttribs = 0;
	if (fAllAreFolders)
	{
		dwAttribs |= SFGAO_FOLDER;
		dwAttribs |= SFGAO_HASSUBFOLDER;
		dwAttribs |= SFGAO_DROPTARGET;
	}
	if (fAllAreDotFiles)
	{
		dwAttribs |= SFGAO_GHOSTED;
		dwAttribs |= SFGAO_HIDDEN;
	}
	if (fAreAllLinks)
	{
		dwAttribs |= SFGAO_FOLDER;
		dwAttribs |= SFGAO_LINK;
	}
	dwAttribs |= SFGAO_CANRENAME;
	dwAttribs |= SFGAO_CANDELETE;
	dwAttribs |= SFGAO_CANCOPY;

    *attributes_inout &= dwAttribs;
}

/*--------------------------------------------------------------------------*/
/*     Functions implementing IShellFolder2 via folder2_error_adapter.      */
/*--------------------------------------------------------------------------*/

/**
 * Convert column index to matching PROPERTYKEY, if any.
 *
 * @implementing folder2_error_adapter
 */
SHCOLUMNID CRemoteFolder::map_column_to_scid(UINT column_index)
{
	return property_key_from_column_index(column_index).get();
}

/*--------------------------------------------------------------------------*/
/*                     CFolder NVI internal interface.                      */
/* These method implement the internal interface of the CFolder abstract    */
/* class                                                                    */
/*--------------------------------------------------------------------------*/

/**
 * Return the folder's registered CLSID
 *
 * @implementing CFolder
 */
CLSID CRemoteFolder::clsid() const
{
	return __uuidof(this);
}

/**
 * Sniff PIDLs to determine if they are of our type.  Throw if not.
 *
 * @implementing CFolder
 */
void CRemoteFolder::validate_pidl(PCUIDLIST_RELATIVE pidl) const
{
	if (pidl == NULL)
		BOOST_THROW_EXCEPTION(com_error(E_POINTER));

	if (!remote_itemid_view(pidl).valid())
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));
}

/**
 * Create and initialise new folder object for subfolder.
 *
 * @implementing CFolder
 *
 * Create new CRemoteFolder initialised with its root PIDL.  CRemoteFolder
 * only have instances of themselves as subfolders.
 */
CComPtr<IShellFolder> CRemoteFolder::subfolder(const cpidl_t& pidl)
{
	apidl_t new_root;

	//if (!remote_itemid_view(pidl).is_link())
	{
		new_root = root_pidl() + pidl;
	}
	/*else
	{
		com_ptr<ISftpProvider> provider = _CreateConnectionForFolder(NULL);
		CSftpDirectory directory(root_pidl(), provider, m_consumer);
		new_root = directory.ResolveLink(pidl);
	}*/

	// Create CRemoteFolder initialised with its root PIDL
	CComPtr<IShellFolder> folder = CRemoteFolder::Create(
		new_root.get(), m_consumer_factory);
	ATLENSURE_THROW(folder, E_NOINTERFACE);

	return folder;
}

/**
 * Return a property, specified by PROERTYKEY, of an item in this folder.
 */
variant_t CRemoteFolder::property(const property_key& key, const cpidl_t& pidl)
{
	return property_from_pidl(pidl, key);
}

/*--------------------------------------------------------------------------*/
/*                    CSwishFolder internal interface.                      */
/* These method override the (usually no-op) implementations of some        */
/* in the CSwishFolder base class                                           */
/*--------------------------------------------------------------------------*/

/**
 * Create a toolbar command provider for the folder.
 */
CComPtr<IExplorerCommandProvider> CRemoteFolder::command_provider(HWND hwnd)
{
	TRACE("Request: IExplorerCommandProvider");
	return remote_folder_command_provider(
		hwnd, root_pidl(), bind(&connection_from_pidl, root_pidl(), hwnd),
		bind(m_consumer_factory, hwnd)).get();
}

/**
 * Create an icon extraction helper object for the selected item.
 *
 * @implementing CSwishFolder
 */
CComPtr<IExtractIconW> CRemoteFolder::extract_icon_w(
	HWND /*hwnd*/, PCUITEMID_CHILD pidl)
{
	TRACE("Request: IExtractIconW");

	remote_itemid_view itemid(pidl);
	return CIconExtractor::Create(
		itemid.filename().c_str(), itemid.is_folder());
}

/**
 * @implementing CSwishFolder
 */
CComPtr<IShellLinkW> CRemoteFolder::shell_link_w(
	HWND hwnd, PCUITEMID_CHILD pidl)
{
	// When navigating into a folder it seems that explorer requests the
	// folder's IShellLinkW interface even if we didn't say that it was a link
	// in its attributes.  Therefore we don't assert there but throw instead.
	if (!remote_itemid_view(pidl).is_link())
		BOOST_THROW_EXCEPTION(com_error(E_NOINTERFACE));

	com_ptr<ISftpProvider> provider =
		connection_from_pidl(root_pidl(), hwnd);
	com_ptr<ISftpConsumer> consumer = m_consumer_factory(hwnd);

	return pidl_to_shell_link(root_pidl(), pidl, provider, consumer).get();
}

/**
 * Create a file association handler for the selected items.
 *
 * @implementing CSwishFolder
 */
CComPtr<IQueryAssociations> CRemoteFolder::query_associations(
	HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl)
{
	TRACE("Request: IQueryAssociations");
	ATLENSURE(cpidl > 0);

	CComPtr<IQueryAssociations> spAssoc;
	HRESULT hr = ::AssocCreate(
		CLSID_QueryAssociations, IID_PPV_ARGS(&spAssoc));
	ATLENSURE_SUCCEEDED(hr);

	remote_itemid_view itemid(apidl[0]);
	
	if (itemid.is_link() || itemid.is_folder())
	{
		// Initialise default assoc provider for Folders
		hr = spAssoc->Init(
			ASSOCF_INIT_DEFAULTTOFOLDER, L"Folder", NULL, hwnd);
		ATLENSURE_SUCCEEDED(hr);
	}
	else
	{
		// Initialise default assoc provider for given file extension
		wstring extension = wpath(itemid.filename()).extension();
		if (extension.empty())
			extension = L".";
		hr = spAssoc->Init(
			ASSOCF_INIT_DEFAULTTOSTAR, extension.c_str(), NULL, hwnd);
		ATLENSURE_SUCCEEDED(hr);
	}

	return spAssoc;
}

HRESULT CALLBACK CRemoteFolder::menu_callback(
	IShellFolder* folder, HWND hwnd_view, IDataObject* selection, 
	UINT message_id, WPARAM wparam, LPARAM lparam)
{
	assert(folder);
	if (!folder)
		return E_POINTER;

	return static_cast<CRemoteFolder*>(folder)->MenuCallback(
		hwnd_view, selection, message_id, wparam, lparam);
}

/**
 * Create a context menu for the selected items.
 *
 * @implementing CSwishFolder
 */
CComPtr<IContextMenu> CRemoteFolder::context_menu(
	HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl)
{
	TRACE("Request: IContextMenu");
	assert(cpidl > 0);

	// Get keys associated with filetype from registry.
	// We only take into account the item that was right-clicked on 
	// (the first array element) even if this was a multi-selection.
	//
	// This article says that we don't need to specify the keys:
	// http://groups.google.com/group/microsoft.public.platformsdk.shell/
	// browse_thread/thread/6f07525eaddea29d/
	// but we do for the context menu to appear in versions of Windows 
	// earlier than Vista.
	HKEY *akeys = NULL;
	UINT ckeys = 0;
	if (cpidl > 0)
	{
		ATLENSURE_THROW(SUCCEEDED(
			CRegistry::GetRemoteFolderAssocKeys(
				remote_itemid_view(apidl[0]), &ckeys, &akeys)),
			E_UNEXPECTED  // Might fail if registry is corrupted
		);
	}

	CComPtr<IShellFolder> spThisFolder = this;
	ATLENSURE_THROW(spThisFolder, E_OUTOFMEMORY);

	// Create default context menu from list of PIDLs
	CComPtr<IContextMenu> spMenu;
	HRESULT hr = ::CDefFolderMenu_Create2(
		root_pidl().get(), hwnd, cpidl, apidl, spThisFolder, 
		menu_callback, ckeys, akeys, &spMenu);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_error(hr));

	return spMenu;
}

CComPtr<IContextMenu> CRemoteFolder::background_context_menu(HWND hwnd)
{
	TRACE("Request: IContextMenu");

	// Get keys associated with directory background menus from registry.
	//
	// This article says that we don't need to specify the keys:
	// http://groups.google.com/group/microsoft.public.platformsdk.shell/
	// browse_thread/thread/6f07525eaddea29d/
	// but we do for the context menu to appear in versions of Windows 
	// earlier than Vista.
	HKEY *akeys; UINT ckeys;
	ATLENSURE_THROW(SUCCEEDED(
		CRegistry::GetRemoteFolderBackgroundAssocKeys(&ckeys, &akeys)),
		E_UNEXPECTED  // Might fail if registry is corrupted
		);

	CComPtr<IShellFolder> spThisFolder = this;
	ATLENSURE_THROW(spThisFolder, E_OUTOFMEMORY);

	// Create default context menu from list of PIDLs
	CComPtr<IContextMenu> spMenu;
	HRESULT hr = ::CDefFolderMenu_Create2(
		root_pidl().get(), hwnd, 0, NULL, spThisFolder, 
		menu_callback, ckeys, akeys, &spMenu);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_error(hr));

	return spMenu;
}

/**
 * Create a data object for the selected items.
 *
 * @implementing CSwishFolder
 */
CComPtr<IDataObject> CRemoteFolder::data_object(
	HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl)
{
	TRACE("Request: IDataObject");
	assert(cpidl > 0);

	try
	{
		com_ptr<ISftpProvider> provider =
			connection_from_pidl(root_pidl(), hwnd);
		com_ptr<ISftpConsumer> consumer = m_consumer_factory(hwnd);

		return CSftpDataObject::Create(
			cpidl, apidl, root_pidl().get(), provider.get(), consumer.get());
	}
	catch (...)
	{
		rethrow_and_announce(
			hwnd,
			(cpidl > 1) ? translate("Unable to access the item") :
			              translate("Unable to access the items"),
			translate("You might not have permission."));
	}
}

/**
 * Create a drop target handler for the folder.
 *
 * @implementing CSwishFolder
 */
CComPtr<IDropTarget> CRemoteFolder::drop_target(HWND hwnd)
{
	TRACE("Request: IDropTarget");

	try
	{
		com_ptr<ISftpProvider> provider =
			connection_from_pidl(root_pidl(), hwnd);
		com_ptr<ISftpConsumer> consumer = m_consumer_factory(hwnd);

		return new CSnitchingDropTarget(
			hwnd, provider, consumer,
			absolute_path_from_swish_pidl(root_pidl()),
			make_shared<DropUI>(hwnd));
	}
	catch (...)
	{
		rethrow_and_announce(
			hwnd, translate("Unable to access the folder"),
			translate("You might not have permission."));
	}
}

/**
 * Create an instance of our Shell Folder View callback handler.
 */
CComPtr<IShellFolderViewCB> CRemoteFolder::folder_view_callback(HWND /*hwnd*/)
{
	return new CViewCallback(root_pidl());
}

HRESULT CRemoteFolder::MenuCallback(
	HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	context_menu_callback callback(
		bind(&connection_from_pidl, root_pidl(), _1), m_consumer_factory);
	return callback(hwnd, pdtobj, uMsg, wParam, lParam);
}
