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

#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer
#include "swish/remote_folder/columns.hpp" // Column
#include "swish/shell_folder/Swish.h" // For CRemoteFolder UUID
#include "swish/shell_folder/RemotePidl.h" // RemoteItemId handling

#include <comet/ptr.h> // com_ptr
#include <comet/server.h> // simple_object

#include <vector>

class CRemoteFolder :
	public comet::simple_object<IShellFolder2,
		swish::shell_folder::folder::CSwishFolder<swish::remote_folder::Column>
	>
{
public:

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
	 *
	 * @returns Smart pointer to the CRemoteFolder's IShellFolder interface.
	 * @throws  com_error if creation fails.
	 */
	static ATL::CComPtr<IShellFolder> Create(__in PCIDLIST_ABSOLUTE pidl)
	throw(...)
	{
		comet::com_ptr<CRemoteFolder> folder = new CRemoteFolder();
		
		HRESULT hr = folder->Initialize(pidl);
		ATLENSURE_SUCCEEDED(hr);
		return folder.get();
	}

protected:

	CLSID clsid() const;

	void validate_pidl(PCUIDLIST_RELATIVE pidl) const;

	ATL::CComPtr<IShellFolder> subfolder(
		const winapi::shell::pidl::apidl_t& pidl) const;

	comet::variant_t property(
		const winapi::shell::property_key& key,
		const winapi::shell::pidl::cpidl_t& pidl);

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
	comet::com_ptr<ISftpConsumer> m_consumer;

	typedef std::vector<CRemoteItem> RemotePidls;

	comet::com_ptr<ISftpProvider> _CreateConnectionForFolder(
		__in_opt  HWND hwndUserInteraction ) throw(...);
	void _Delete( __in_opt HWND hwnd, __in const RemotePidls& vecDeathRow )
		throw(...);
	void _DoDelete(
		__in_opt HWND hwnd, __in const RemotePidls& vecDeathRow ) throw(...);
	bool _ConfirmDelete(
		__in_opt HWND hwnd, __in BSTR bstrName, __in bool fIsFolder ) throw();
	bool _ConfirmMultiDelete( __in_opt HWND hwnd, size_t cItems ) throw();

	/**
	 * Static dispatcher for Default Context Menu callback
	 */
	static HRESULT __callback CALLBACK MenuCallback(
		__in_opt IShellFolder *psf, HWND hwnd, __in_opt IDataObject *pdtobj, 
		UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		ATLENSURE_RETURN(psf);
		return static_cast<CRemoteFolder *>(psf)->OnMenuCallback(
			hwnd, pdtobj, uMsg, wParam, lParam);
	}

	/** @name Default context menu event handlers */
	// @{
	HRESULT OnMenuCallback( HWND hwnd, IDataObject *pdtobj, 
		UINT uMsg, WPARAM wParam, LPARAM lParam );
	HRESULT OnMergeContextMenu(
		HWND hwnd, IDataObject *pDataObj, UINT uFlags, QCMINFO& info );
	HRESULT OnInvokeCommand(
		HWND hwnd, IDataObject *pDataObj, int idCmd, PCWSTR pwszArgs );
	HRESULT OnInvokeCommandEx(
		HWND hwnd, IDataObject *pDataObj, int idCmd, PDFMICS pdfmics );
	// @}

	/** @name Invoked command handlers */
	// @{
	HRESULT OnCmdDelete( HWND hwnd, IDataObject *pDataObj );
	// @}

};