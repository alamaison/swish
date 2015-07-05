/**
    @file

    Explorer folder that handles remote files and folders.

    @if license

    Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012, 2013
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
#include "swish/drop_target/DropTarget.hpp" // CDropTarget
#include "swish/drop_target/DropUI.hpp" // DropUI
#include "swish/frontend/announce_error.hpp" // announce_last_exception
#include "swish/remote_folder/columns.hpp" // property_key_from_column_index
#include "swish/remote_folder/commands/commands.hpp"
                                           // remote_folder_command_provider
#include "swish/remote_folder/pidl_connection.hpp" // provider_from_pidl
#include "swish/remote_folder/context_menu_callback.hpp"
                                                       // context_menu_callback
#include "swish/remote_folder/properties.hpp" // property_from_pidl
#include "swish/remote_folder/remote_pidl.hpp" // remote_itemid_view
                                               // create_remote_itemid
#include "swish/remote_folder/ViewCallback.hpp" // CViewCallback
#include "swish/shell_folder/SnitchingDataObject.hpp" // CSnitchingDataObject
#include "swish/trace.hpp" // trace
#include "swish/windows_api.hpp" // SHBindToParent

#include <washer/shell/shell.hpp> // string_to_strret
#include <washer/window/window.hpp>

#include <comet/datetime.h> // datetime_t
#include <comet/regkey.h>

#include <boost/bind.hpp> // bind
#include <boost/exception/diagnostic_information.hpp> // diagnostic_information
#include <boost/filesystem/path.hpp> // path
#include <boost/locale.hpp> // translate
#include <boost/make_shared.hpp> // make_shared
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <string>

using swish::drop_target::CDropTarget;
using swish::drop_target::DropUI;
using swish::frontend::announce_last_exception;
using swish::provider::sftp_provider;
using swish::remote_folder::CViewCallback;
using swish::remote_folder::commands::remote_folder_command_provider;
using swish::remote_folder::context_menu_callback;
using swish::remote_folder::create_remote_itemid;
using swish::remote_folder::property_from_pidl;
using swish::remote_folder::property_key_from_column_index;
using swish::remote_folder::provider_from_pidl;
using swish::remote_folder::remote_itemid_view;
using swish::tracing::trace;

using washer::shell::pidl::apidl_t;
using washer::shell::pidl::cpidl_t;
using washer::shell::pidl::pidl_t;
using washer::shell::property_key;
using washer::shell::string_to_strret;
using washer::window::window;
using washer::window::window_handle;

using comet::com_ptr;
using comet::com_error;
using comet::datetime_t;
using comet::regkey;
using comet::throw_com_error;
using comet::variant_t;

using boost::bind;
using boost::filesystem::path;
using boost::locale::translate;
using boost::make_shared;
using boost::optional;
using boost::shared_ptr;

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
                return path(full_name).stem().wstring();
            }
            else
            {
                // File might look something like '.hidden.txt' or it might
                // just be '.hidden'.  In the first case we only want to remove
                // the '.txt' extension.  In the second case we don't want
                // to remove anything.
                wstring bit_after_initial_dot = full_name.substr(1);
                return L'.' + path(bit_after_initial_dot).stem().wstring();
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
        com_ptr<ISftpConsumer> consumer = m_consumer_factory(hwnd);

        // TODO: get the name of the directory and embed in the task name
        shared_ptr<sftp_provider> provider = provider_from_pidl(
            root_pidl(), consumer, translate(
                "Name of a running task", "Reading a directory"));

        // Create directory handler and get listing as PIDL enumeration
        CSftpDirectory directory(root_pidl(), provider);
        return directory.GetEnum(flags).detach();
    }
    catch (...)
    {
        announce_last_exception(
            hwnd, translate(L"Unable to access the directory"),
            translate(L"You might not have permission."));
        throw;
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
        announce_last_exception(
            hwnd, translate(L"Path not recognised"),
            translate(L"Check that the path was entered correctly."));
        throw;
    }
}

namespace {

    bool extension_hiding_disabled_in_registry()
    {
        if (regkey user_settings = regkey(HKEY_CURRENT_USER).open_nothrow(
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
            L"Advanced"))
        {
            regkey::mapped_type extension_setting =
                user_settings[L"HideFileExt"];

            if (extension_setting.exists())
            {
                return extension_setting == 0U;
            }
        }
        
        // We only reach here if the user settings didn't exist, not if
        // they just said "no".  This means the global settings don't 
        // override the user settings, which seems the right way round.
        if (regkey global_settings = regkey(HKEY_LOCAL_MACHINE).open_nothrow(
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
            L"Advanced\\Folder\\HideFileExt"))
        {
            regkey::mapped_type extension_setting =
                global_settings[L"DefaultValue"];

            if (extension_setting.exists())
            {
                return extension_setting == 0U;
            }
        }

        // It's unlikely that neither will be set but we're prepared for it
        // anyway
        return false;
    }

}



bool CRemoteFolder::show_extension(PCUITEMID_CHILD pidl)
{
    if (extension_hiding_disabled_in_registry())
        return true;

    HKEY raw_class_key = NULL;
    com_ptr<IQueryAssociations> associations = query_associations(
        NULL, 1, &pidl);
    HRESULT hr = associations->GetKey(
        0, ASSOCKEY_CLASS, NULL, &raw_class_key);
    regkey class_key(raw_class_key);

    // Failing to find the key indicates an unknown file type.  As the
    // user setting say 'Hide extensions for *known* filetypes' we
    // show the extension if the file is unknown.
    if FAILED(hr)
    {
        return true;
    }
    else
    {
        // In practice, Explorer returns the "Unknown" key for unregistered
        // file types.  But that's ok; it contains an AlwaysShowExt value
        // so we obey that and it all comes out in the wash.
        return class_key[L"AlwaysShowExt"].exists();
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

        if (show_extension(pidl))
        {
            // The table of SHGDN examples on MSDN implies that the
            // presence of the SHGDN_FORPARSING flag means include the file
            // extension and its absence means remove it.
            // But that's not the full story.  The SHGDN_FORPARSING flag
            // indeed means include the file extension, but its absence means
            // do what the user wants.  In other words, remove the extension
            // if their Explorer settings say that's what they want.
            // Checking the Explorer settings is up to the individual
            // namespace extension.

            name = remote_itemid_view(pidl).filename();
        }
        else
        {
            name = filename_without_extension(pidl);
        }
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
        // TODO: embed the name of the file in the task name
        com_ptr<ISftpConsumer> consumer = m_consumer_factory(hwnd);
        shared_ptr<sftp_provider> provider =
            provider_from_pidl(root_pidl(), consumer, translate(
                "Name of a running task", "Renaming a file"));

        // Rename file
        CSftpDirectory directory(root_pidl(), provider);
        bool fOverwritten = directory.Rename(pidl, name, consumer);

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
        announce_last_exception(
            hwnd, translate(L"Unable to rename the item"),
            translate(L"You might not have permission."));
        throw;
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
    apidl_t new_root = root_pidl() + pidl;

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
        hwnd, root_pidl(), bind(&provider_from_pidl, root_pidl(), _1, _2),
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
    
    if (itemid.is_folder())
    {
        // Initialise default assoc provider for Folders
        hr = spAssoc->Init(
            ASSOCF_INIT_DEFAULTTOFOLDER, L"Folder", NULL, hwnd);
        ATLENSURE_SUCCEEDED(hr);
    }
    else
    {
        // Initialise default assoc provider for given file extension
        wstring extension = path(itemid.filename()).extension().wstring();
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
        // TODO: pass a provider factory instead of the provider to the
        // data object and create more specific reservations when needed
        com_ptr<ISftpConsumer> consumer = m_consumer_factory(hwnd);
        shared_ptr<sftp_provider> provider =
            provider_from_pidl(root_pidl(), consumer, translate(
                "Name of a running task", "Accessing files"));

        return new swish::shell_folder::CSnitchingDataObject(
            new CSftpDataObject(
                cpidl, apidl, root_pidl().get(), provider));
    }
    catch (...)
    {
        announce_last_exception(
            hwnd,
            (cpidl > 1) ? translate(L"Unable to access the item") :
                          translate(L"Unable to access the items"),
            translate(L"You might not have permission."));
        throw;
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
        // TODO: pass a provider factory instead of the provider to the
        // drop target and create more specific reservations when needed
        com_ptr<ISftpConsumer> consumer = m_consumer_factory(hwnd);
        shared_ptr<sftp_provider> provider =
            provider_from_pidl(root_pidl(), consumer, translate(
                "Name of a running task", "Copying to directory"));

        optional< window<wchar_t> > owner;
        if (hwnd)
            owner = window<wchar_t>(window_handle::foster_handle(hwnd));

        // HACKish:
        // UI happens via the given owner window given here.  We used to do it
        // via the window of the OLE site instead, but this is incompatible
        // with asynchronous operations because the shell clears the site
        // when Drop returns (at which point the operation is still running
        // and may need an owner window for UI).
        //
        // We could hang on to a copy of the site but that seems .. impolite.
        // After all, the shell presumably cleared the site for a reason.
        //
        // That said, what we're doing now seems pretty naughty too. We use the
        // window we were passed as an owner window when we were created.  This
        // window is probably the one the shell passed to our folder's
        // GetUIObjectOf or CreateViewObject methods.  MSDN documents this
        // window as the 'owner' to be used for UI but doesn't make clear how
        // long the window is guarantted to remain alive: until the
        // GetUIObjectOf/CreateViewObject call returns or for as long as this
        // drop target is in use.  Nevertheless, this seems to work so it's
        // what we're doing for now.
        return new CDropTarget(
            provider, root_pidl(), make_shared<DropUI>(owner));
    }
    catch (...)
    {
        announce_last_exception(
            hwnd, translate(L"Unable to access the folder"),
            translate(L"You might not have permission."));
        throw;
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
        bind(&provider_from_pidl, root_pidl(), _1, _2), m_consumer_factory);
    return callback(hwnd, pdtobj, uMsg, wParam, lParam);
}
