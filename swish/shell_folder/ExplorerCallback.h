/*  Handler for the Shell Folder View's interaction with Explorer.

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/CoFactory.hpp"  // CComObject factory

#include "swish/atl.hpp"  // Common ATL setup

#include "HostPidl.h"

#define STRICT_TYPED_ITEMIDS ///< Better type safety for PIDLs (must be 
                             ///< before <shtypes.h> or <shlobj.h>).
#include <shlobj.h>  // Windows Shell API

class ATL_NO_VTABLE CExplorerCallback :
	public IShellFolderViewCB,
	public ATL::CComObjectRoot,
	public ATL::IObjectWithSiteImpl<CExplorerCallback>,
	private swish::CCoFactory<CExplorerCallback>
{
public:

	BEGIN_COM_MAP(CExplorerCallback)
		COM_INTERFACE_ENTRY(IShellFolderViewCB)
		COM_INTERFACE_ENTRY(IObjectWithSite)
	END_COM_MAP()

	CExplorerCallback() : 
		m_hwndView(NULL), m_hToolsMenu(NULL), m_idCmdFirst(0), m_pidl(NULL) {}
	~CExplorerCallback()
	{
		if (m_pidl)
			::ILFree(m_pidl);
	}

	HRESULT Initialize( PCIDLIST_ABSOLUTE pidl );

	/**
	 * Create and initialise an instance of the CExplorerCallback class.
	 *
	 * @param[in] pidl  An absolute PIDL to the folder for whom we are
	 *                  creating this callback object.
	 *
	 * @returns  Pointer to the object's IShellFolderViewCB interface.
	 * @throws   com_exception if creation fails.
	 */
	static ATL::CComPtr<IShellFolderViewCB> Create(__in PCIDLIST_ABSOLUTE pidl)
	throw(...)
	{
		ATL::CComPtr<CExplorerCallback> spCB = spCB->CreateCoObject();
		HRESULT hr = spCB->Initialize(pidl);
		ATLENSURE_SUCCEEDED(hr);

		return ATL::CComPtr<IShellFolderViewCB>(spCB);
	}

public: // IShellFolderViewCB

	IFACEMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:

	__checkReturn HMENU _GetToolsMenu(HMENU hParentMenu);

	bool _ShouldEnableRemove();

	ATL::CComPtr<IShellBrowser> _GetShellBrowser();
	ATL::CComPtr<IShellView> _GetShellView();
	ATL::CComPtr<IDataObject> _GetSelectionDataObject();
	CAbsolutePidl _GetSelectedItem();

	// Menu command ID offsets for Explorer Tools menu
	enum MENUOFFSET
	{
		MENUIDOFFSET_FIRST = 0,
		MENUIDOFFSET_ADD = MENUIDOFFSET_FIRST,
		MENUIDOFFSET_REMOVE,
		MENUIDOFFSET_LAST = MENUIDOFFSET_REMOVE
	};

	HWND m_hwndView;          ///< Handle to folder view window
	HMENU m_hToolsMenu;       ///< Handle to the Explorer 'Tools' menu
	UINT m_idCmdFirst;        ///< Start of our tools menu ID range
	PIDLIST_ABSOLUTE m_pidl;  ///< Our copy of pidl to owning folder
};