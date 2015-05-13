/*  Explorer folder handling remote files and folders in a directory.

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
*/

#pragma once

#include "SwishFolder.hpp"      // Superclass

#include "swish/CoFactory.hpp"  // CComObject factory
#include "swish/provider/sftp_provider.hpp" // sftp_provider, ISftpConsumer
#include "swish/remote_folder/columns.hpp" // Column
#include "Swish.h" // For CRemoteFolder UUID

#include "swish/atl.hpp"        // Common ATL setup

#include <washer/shell/pidl.hpp> // cpidl_t

#include <comet/ptr.h> // com_ptr

#include <boost/function.hpp> // function

#include <vector>

class ATL_NO_VTABLE CRemoteFolder :
    public swish::shell_folder::folder::CSwishFolder<
        swish::remote_folder::Column>,
    private swish::CCoFactory<CRemoteFolder>
{
public:

    BEGIN_COM_MAP(CRemoteFolder)
        COM_INTERFACE_ENTRY(IShellFolder)
        COM_INTERFACE_ENTRY_CHAIN(CSwishFolder)
    END_COM_MAP()

    /*
    We can assume that the PIDLs contained in this folder (i.e. any PIDL
    relative to it) contain one or more REMOTEPIDLs representing the 
    file-system hierarchy of the target file or folder and may be a child of
    either a HOSTPIDL or another REMOTEPIDL:

        <Relative (HOST|REMOTE)PIDL>/REMOTEPIDL[/REMOTEPIDL]*
    */

    /**
     * Create initialized instance of the CRemoteFolder class.
     *
     * @param pidl  Absolute PIDL at which to root the folder instance (passed
     *              to Initialize).
     * @param consumer_factory  Callable that returns a consumer instance to
     *                          use for a single request.
     *
     * @returns Smart pointer to the CRemoteFolder's IShellFolder interface.
     * @throws  com_error if creation fails.
     */
    static ATL::CComPtr<IShellFolder> Create(
        PCIDLIST_ABSOLUTE pidl,
        boost::function<comet::com_ptr<ISftpConsumer>(HWND)> consumer_factory)
    throw(...)
    {
        ATL::CComPtr<CRemoteFolder> spObject = spObject->CreateCoObject();

        spObject->set_consumer_factory(consumer_factory);
        
        HRESULT hr = spObject->Initialize(pidl);
        ATLENSURE_SUCCEEDED(hr);
        return spObject.p;
    }

protected:

    CLSID clsid() const;

    void validate_pidl(PCUIDLIST_RELATIVE pidl) const;

    ATL::CComPtr<IShellFolder> subfolder(
        const washer::shell::pidl::cpidl_t& pidl);

    comet::variant_t property(
        const washer::shell::property_key& key,
        const washer::shell::pidl::cpidl_t& pidl);

    ATL::CComPtr<IExplorerCommandProvider> command_provider(HWND hwnd);
    ATL::CComPtr<IContextMenu> background_context_menu(HWND hwnd);
    ATL::CComPtr<IExtractIconW> extract_icon_w(
        HWND hwnd, PCUITEMID_CHILD pidl);
    ATL::CComPtr<IQueryAssociations> query_associations(
        HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);
    ATL::CComPtr<IContextMenu> context_menu(
        HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);
    ATL::CComPtr<IDataObject> data_object(
        HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);
    ATL::CComPtr<IDropTarget> drop_target(HWND hwnd);

    ATL::CComPtr<IShellFolderViewCB> folder_view_callback(HWND hwnd);

public:

    // IShellFolder (via folder_error_adapter)
    virtual IEnumIDList* enum_objects(HWND hwnd, SHCONTF flags);
    
    virtual void get_attributes_of(
        UINT pidl_count, PCUITEMID_CHILD_ARRAY pidl_array,
        SFGAOF* flags_inout);

    virtual STRRET get_display_name_of(
        PCUITEMID_CHILD pidl, SHGDNF uFlags);

    virtual PIDLIST_RELATIVE parse_display_name(
        HWND hwnd, IBindCtx* bind_ctx, const wchar_t* display_name,
        ULONG* attributes_inout);

    virtual PITEMID_CHILD set_name_of(
        HWND hwnd, PCUITEMID_CHILD pidl, const wchar_t* name,
        SHGDNF flags);

    // IShellFolder2 (via folder_error_adapter2)
    virtual SHCOLUMNID map_column_to_scid(UINT column_index);

private:
    boost::function<comet::com_ptr<ISftpConsumer>(HWND)> m_consumer_factory;
    comet::com_ptr<ISftpConsumer> m_consumer;

    void set_consumer_factory(
        boost::function<comet::com_ptr<ISftpConsumer>(HWND)> consumer_factory)
    {
        m_consumer_factory = consumer_factory;
    }


    /** @name Default Context Menu event handler */
    // @{
    static HRESULT CALLBACK menu_callback(
        IShellFolder* folder, HWND hwnd_view, IDataObject* selection, 
        UINT message_id, WPARAM wparam, LPARAM lparam);
    HRESULT MenuCallback( HWND hwnd, IDataObject *pdtobj, 
        UINT uMsg, WPARAM wParam, LPARAM lParam );
    // @}

    
    bool CRemoteFolder::show_extension(PCUITEMID_CHILD pidl);

};