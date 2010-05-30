/**
    @file

    SFTP connections Explorer folder implementation.

    @if licence

    Copyright (C) 2007, 2008, 2009, 2010
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

#include "HostFolder.h"

#include "RemoteFolder.h"
#include "ExplorerCallback.h"     // For interaction with Explorer window
#include "Registry.h"             // For saved connection details
#include "host_management.hpp"
#include "swish/catch_com.hpp" // catchCom
#include "swish/debug.hpp"
#include "swish/host_folder/properties.hpp" // property_from_pidl
#include "swish/host_folder/columns.hpp" // property_key_from_column_index
#include "swish/remotelimits.h"   // Text field limits
#include "swish/exception.hpp"    // com_exception
#include "swish/windows_api.hpp" // SHBindToParent
#include "swish/shell_folder/commands/host/host.hpp" // host_folder_commands
#include "swish/trace.hpp" // trace

#include <winapi/shell/shell.hpp> // strret_to_string

#include <comet/enum.h> // stl_enumeration

#include <strsafe.h>  // For StringCchCopy

#include <boost/locale.hpp> // translate
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <cstring> // memset
#include <string>
#include <vector>

using ATL::CComPtr;
using ATL::CComObject;

using comet::com_error;
using comet::com_ptr;
using comet::stl_enumeration_t;
using comet::throw_com_error;
using comet::variant_t;

using boost::locale::translate;
using boost::shared_ptr;

using std::vector;
using std::wstring;

using swish::host_management::LoadConnectionsFromRegistry;
using swish::exception::com_exception;
using swish::host_folder::property_from_pidl;
using swish::host_folder::property_key_from_column_index;
using swish::shell_folder::commands::host::host_folder_command_provider;
using swish::tracing::trace;

using winapi::shell::pidl::cpidl_t;
using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::pidl_t;
using winapi::shell::property_key;
using winapi::shell::strret_to_string;
using winapi::shell::string_to_strret;

/**
 * Comet IID lookup for IEnumIDList.
 */
template<> struct ::comet::comtype<::IEnumIDList>
{
	static const ::IID& uuid() throw() { return ::IID_IEnumIDList; }
	typedef ::IUnknown base;
};

/**
 * Copy policy used to create IEnumIDList from CHostItems.
 */
template<> struct ::comet::impl::type_policy<PITEMID_CHILD>
{
	static void init(PITEMID_CHILD& raw_pidl, const CHostItem& pidl) 
	{
		assert(pidl.IsValid());
		raw_pidl = pidl.CopyTo();
	}

	static void clear(PITEMID_CHILD& raw_pidl) { ::CoTaskMemFree(raw_pidl); }	
};

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
 *
 * @retval S_FALSE if the are no matching items to enumerate.
 */
IEnumIDList* CHostFolder::enum_objects(HWND hwnd, SHCONTF flags)
{
	UNREFERENCED_PARAMETER(hwnd); // No UI required to access registry

	// This folder only contains folders
	if (!(flags & SHCONTF_FOLDERS) ||
		(flags & (SHCONTF_NETPRINTERSRCH | SHCONTF_SHAREABLE)))
		return NULL;

	// Load connections from HKCU\Software\Swish\Connections
	m_vecConnData = LoadConnectionsFromRegistry();

    // Create an enumerator from the member vector.  We pass in a pointer to
	// this object so that the vector remains valid as long as the enumerator
	// is in use.
	return new stl_enumeration_t<
		IEnumIDList, vector<CHostItem>, PITEMID_CHILD>(
		m_vecConnData, GetUnknown());
}

/**
 * Convert path string relative to this folder into a PIDL to the item.
 *
 * @implementing folder_error_adapter
 *
 * @todo  Handle the attributes parameter.  Should just return
 * GetAttributesOf() the PIDL we create but it is a bit hazy where the
 * host PIDL's responsibilities end and the remote PIDL's start because
 * of the path embedded in the host PIDL.
 */
PIDLIST_RELATIVE CHostFolder::parse_display_name(
	HWND hwnd, IBindCtx* bind_ctx, const wchar_t* display_name,
	ULONG* attributes_inout)
{
	trace(__FUNCTION__" called (display_name=%s)") % display_name;

	// The string we are trying to parse should be of the form:
	//    sftp://username@hostname:port/path

	wstring strDisplayName(display_name);
	if (strDisplayName.empty())
	{
		PIDLIST_RELATIVE pidl;
		root_pidl().copy_to(pidl);
		return pidl;
	}

	// Must start with sftp://
	if (strDisplayName.substr(0, 7) != L"sftp://")
		BOOST_THROW_EXCEPTION(com_error(E_FAIL));

	// Must have @ to separate username from hostname
	wstring::size_type nAt = strDisplayName.find_first_of(L'@', 7);
	if (nAt == wstring::npos)
		BOOST_THROW_EXCEPTION(com_error(E_FAIL));

	// Must have : to separate hostname from port number
	wstring::size_type nColon = strDisplayName.find_first_of(L':', 7);
	if (nAt == wstring::npos)
		BOOST_THROW_EXCEPTION(com_error(E_FAIL));
	if (nColon <= nAt)
		BOOST_THROW_EXCEPTION(com_error(E_FAIL));

	// Must have / to separate port number from path
	wstring::size_type nSlash = strDisplayName.find_first_of(L'/', 7);
	if (nAt == wstring::npos)
		BOOST_THROW_EXCEPTION(com_error(E_FAIL));
	if (nColon <= nAt)
		BOOST_THROW_EXCEPTION(com_error(E_FAIL));

	wstring strUser = strDisplayName.substr(7, nAt - 7);
	wstring strHost = strDisplayName.substr(nAt+1, nColon - (nAt+1));
	wstring strPort = strDisplayName.substr(nColon+1, nAt - (nSlash+1));
	wstring strPath = strDisplayName.substr(nSlash+1);
	if (strUser.empty() || strHost.empty() || strPort.empty() ||
		strPath.empty())
		BOOST_THROW_EXCEPTION(com_error(E_FAIL));

	int nPort = _wtoi(strPort.c_str());
	if (nPort < MIN_PORT || nPort > MAX_PORT)
		BOOST_THROW_EXCEPTION(com_error(E_FAIL));

	// Create child PIDL for this path segment
	CHostItem pidl(
		strUser.c_str(), strHost.c_str(), strPath.c_str(),
		static_cast<USHORT>(nPort));

	com_ptr<IShellFolder> subfolder;
	bind_to_object(
		pidl, bind_ctx, subfolder.iid(),
		reinterpret_cast<void**>(subfolder.out()));

	wchar_t wszPath[MAX_PATH];
	::StringCchCopyW(wszPath, ARRAYSIZE(wszPath), strPath.c_str());

	pidl_t pidl_path;
	HRESULT hr = subfolder->ParseDisplayName(
		hwnd, bind_ctx, wszPath, NULL, pidl_path.out(), attributes_inout);
	if (FAILED(hr))
		throw_com_error(subfolder.get(), hr);

	pidl_t pidl_out = root_pidl() + pidl_path;
	return pidl_out.detach();
}

/**
 * Retrieve the display name for the specified file object or subfolder.
 *
 * @implementing folder_error_adapter
 */
STRRET CHostFolder::get_display_name_of(PCUITEMID_CHILD pidl, SHGDNF flags)
{
	if (::ILIsEmpty(pidl))
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

	wstring name;
	CHostItem hpidl(pidl);

	if (flags & SHGDN_FORPARSING)
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

			name = strret_to_string<wchar_t>(
				strret, pidlThisFolder) + L'\\';
		}

		name += hpidl.GetLongName(true);
	}
	else if (flags == SHGDN_NORMAL || flags & SHGDN_FORADDRESSBAR)
	{
		name = hpidl.GetLongName(false);
	}
	else if (flags == SHGDN_INFOLDER || flags & SHGDN_FOREDITING)
	{
		name = hpidl.GetLabel();
	}
	else
	{
		UNREACHABLE;
		BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));
	}

	return string_to_strret(name);
}

/**
 * Rename item.
 *
 * @todo  Support renaming host items.
 */
PITEMID_CHILD CHostFolder::set_name_of(
	HWND /*hwnd*/, PCUITEMID_CHILD /*pidl*/, const wchar_t* /*name*/,
	SHGDNF /*flags*/)
{
	BOOST_THROW_EXCEPTION(com_error(E_NOTIMPL));
}

/**
 * Returns the attributes for the items whose PIDLs are passed in.
 *
 * @implementing folder_error_adapter
 */
void CHostFolder::get_attributes_of(
	UINT pidl_count, PCUITEMID_CHILD_ARRAY pidl_array,
	SFGAOF* attributes_inout)
{
	(void)pidl_array; // All items are folders. No need to check PIDL.
	(void)pidl_count;

	DWORD dwAttribs = 0;
    dwAttribs |= SFGAO_FOLDER;
    dwAttribs |= SFGAO_HASSUBFOLDER;
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
SHCOLUMNID CHostFolder::map_column_to_scid(UINT column_index)
{
	return property_key_from_column_index(column_index).get();
}

/*--------------------------------------------------------------------------*/
/*                    Functions implementing IExtractIcon                   */
/*--------------------------------------------------------------------------*/

/**
 * Extract an icon bitmap given the information passed.
 *
 * @implementing IExtractIconW
 *
 * We return S_FALSE to tell the shell to extract the icons itself.
 */
STDMETHODIMP CHostFolder::Extract( LPCTSTR, UINT, HICON *, HICON *, UINT )
{
	ATLTRACE("CHostFolder::Extract called\n");
	return S_FALSE;
}

/**
 * Retrieve the location of the appropriate icon.
 *
 * @implementing IExtractIconW
 *
 * We set all SFTP hosts to have the icon from shell32.dll.
 */
STDMETHODIMP CHostFolder::GetIconLocation(
	__in UINT uFlags, __out_ecount(cchMax) LPTSTR szIconFile, 
	__in UINT cchMax, __out int *piIndex, __out UINT *pwFlags )
{
	ATLTRACE("CHostFolder::GetIconLocation called\n");
	(void)uFlags; // type of use is ignored for host folder

	// Set host to have the ICS host icon
	StringCchCopy(szIconFile, cchMax, L"shell32.dll");
	*piIndex = 17;
	*pwFlags = GIL_DONTCACHE;

	return S_OK;
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
CLSID CHostFolder::clsid() const
{
	return __uuidof(this);
}

/**
 * Sniff PIDLs to determine if they are of our type.  Throw if not.
 *
 * @implementing CFolder
 */
void CHostFolder::validate_pidl(PCUIDLIST_RELATIVE pidl) const
{
	if (pidl == NULL)
		throw com_exception(E_POINTER);

	if (!CHostItemList::IsValid(pidl))
		throw com_exception(E_INVALIDARG);
}

/**
 * Create and initialise new folder object for subfolder.
 *
 * @implementing CFolder
 *
 * Create CRemoteFolder initialised with its root PIDL.  CHostFolders
 * don't have any other types of subfolder.
 */
CComPtr<IShellFolder> CHostFolder::subfolder(const apidl_t& pidl) const
{
	CComPtr<IShellFolder> folder = CRemoteFolder::Create(pidl.get());
	ATLENSURE_THROW(folder, E_NOINTERFACE);

	return folder;
}

/**
 * Return a property, specified by PROERTYKEY, of an item in this folder.
 */
variant_t CHostFolder::property(const property_key& key, const cpidl_t& pidl)
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
CComPtr<IExplorerCommandProvider> CHostFolder::command_provider(
	HWND hwnd)
{
	TRACE("Request: IExplorerCommandProvider");
	return host_folder_command_provider(hwnd, root_pidl()).get();
}

/**
 * Create an icon extraction helper object for the selected item.
 *
 * @implementing CSwishFolder
 *
 * For host folders, the extraction object happens to be the folder
 * itself. We don't need to look at the PIDLs as all host items are the same.
 */
CComPtr<IExtractIconW> CHostFolder::extract_icon_w(
	HWND /*hwnd*/, PCUITEMID_CHILD /*pidl*/)
{
	TRACE("Request: IExtractIconW");

	return this;
}

/**
 * Create a file association handler for host items.
 *
 * @implementing CSwishFolder
 *
 * We don't need to look at the PIDLs as all host items are the same.
 */
CComPtr<IQueryAssociations> CHostFolder::query_associations(
	HWND /*hwnd*/, UINT /*cpidl*/, PCUITEMID_CHILD_ARRAY /*apidl*/)
{
	TRACE("Request: IQueryAssociations");

	CComPtr<IQueryAssociations> spAssoc;
	HRESULT hr = ::AssocCreate(
		CLSID_QueryAssociations, IID_PPV_ARGS(&spAssoc));
	ATLENSURE_SUCCEEDED(hr);

	// Get CLSID in {DWORD-WORD-WORD-WORD-WORD.DWORD} form
	LPOLESTR posz;
	::StringFromCLSID(__uuidof(CHostFolder), &posz);
	shared_ptr<OLECHAR> clsid(posz, ::CoTaskMemFree);
	posz = NULL;

	// Initialise default assoc provider to use Swish CLSID key for data.
	// This is necessary to pick up properties and TileInfo etc.
	hr = spAssoc->Init(0, clsid.get(), NULL, NULL);
	if (FAILED(hr))
		throw com_exception(hr);

	return spAssoc;
}

/**
 * Create a context menu for the selected items.
 *
 * @implementing CSwishFolder
 */
CComPtr<IContextMenu> CHostFolder::context_menu(
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
	HKEY *akeys; UINT ckeys;
	ATLENSURE_THROW(SUCCEEDED(
		CRegistry::GetHostFolderAssocKeys(&ckeys, &akeys)),
		E_UNEXPECTED  // Might fail if registry is corrupted
	);

	CComPtr<IShellFolder> spThisFolder = this;
	ATLENSURE_THROW(spThisFolder, E_OUTOFMEMORY);

	// Create default context menu from list of PIDLs
	CComPtr<IContextMenu> spMenu;
	HRESULT hr = ::CDefFolderMenu_Create2(
		root_pidl().get(), hwnd, cpidl, apidl, spThisFolder, 
		MenuCallback, ckeys, akeys, &spMenu);
	if (FAILED(hr))
		throw com_exception(hr);

	return spMenu;
}

/**
 * Create a data object for the selected items.
 *
 * @implementing CSwishFolder
 */
CComPtr<IDataObject> CHostFolder::data_object(
	HWND /*hwnd*/, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl)
{
	TRACE("Request: IDataObject");
	assert(cpidl > 0);

	// A DataObject is required in order for the call to 
	// CDefFolderMenu_Create2 (above) to succeed on versions of Windows
	// earlier than Vista

	CComPtr<IDataObject> spdo;
	HRESULT hr = ::CIDLData_CreateFromIDArray(
		root_pidl().get(), cpidl, 
		reinterpret_cast<PCUIDLIST_RELATIVE_ARRAY>(apidl), &spdo);
	if (FAILED(hr))
		throw com_exception(hr);

	return spdo;
}

/**
 * Create an instance of our Shell Folder View callback handler.
 *
 * @implementing CSwishFolder
 */
CComPtr<IShellFolderViewCB> CHostFolder::folder_view_callback(HWND /*hwnd*/)
{
	return CExplorerCallback::Create(root_pidl().get());
}


/*--------------------------------------------------------------------------*/
/*                         Context menu handlers.                           */
/*--------------------------------------------------------------------------*/

/**
 * Cracks open the @c DFM_* callback messages and dispatched them to handlers.
 */
HRESULT CHostFolder::OnMenuCallback(
	HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	METHOD_TRACE("(uMsg=%d)\n", uMsg);
	UNREFERENCED_PARAMETER(hwnd);

	switch (uMsg)
	{
	case DFM_MERGECONTEXTMENU:
		// Must return S_OK even if we do nothing else or Vista and later
		// won't add standard verbs
		return S_OK;
	case DFM_INVOKECOMMAND: // Required to invoke default action
	case DFM_INVOKECOMMANDEX:
	case DFM_GETDEFSTATICID: // Required for Windows 7 to pick a default
		return S_FALSE; 
	default:
		return E_NOTIMPL; // Required for Windows 7 to show any menu at all
	}
}
