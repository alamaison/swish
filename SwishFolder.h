// SwishFolder.h : Declaration of the CSwishFolder

#ifndef SWISHFOLDER_H
#define SWISHFOLDER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "resource.h"       // main symbols

#define INITGUID
#include <propkey.h>

#include "PidlManager.h"

#define _ATL_DEBUG_QI

// ISwishFolder
[
	object,
	uuid("b816a839-5022-11dc-9153-0090f5284f85"),
	helpstring("ISwishFolder Interface"),
	pointer_default(unique)
]
__interface ISwishFolder : IUnknown
{
};


// CSwishFolder
[
	coclass,
	default(ISwishFolder),
	threading(apartment),
	vi_progid("Swish.SwishFolder"),
	progid("Swish.SwishFolder.1"),
	version(1.0),
	registration_script("SwishFolder.rgs"),
	uuid("b816a83a-5022-11dc-9153-0090f5284f85"),
	helpstring("SwishFolder Class")
]
class ATL_NO_VTABLE CSwishFolder :
	public ISwishFolder,
	// The IShellFolder2-specific detail-handling methods are not compatible
	// with Win 9x/NT but it supports all those of IShellDetails which are
	public IShellFolder2, 
	// IPersistFolder2 needed for Details expando
    public IPersistFolder2, 
	public IExtractIcon
//	public IShellDetails // This is compatible with 9x/NT unlike IShellFolder2
{
public:
	CSwishFolder() : m_pidlRoot(NULL)
	{
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	void FinalRelease()
	{
	}

	// Init function - call right after constructing a CSwishFolder object
    HRESULT _init ( CSwishFolder* pParentFolder, LPCITEMIDLIST pidl )
    {
        m_pParentFolder = pParentFolder;

        if ( NULL != m_pParentFolder )
            m_pParentFolder->AddRef();

        m_pidl = m_PidlManager.Copy ( pidl );

        return S_OK;
    }

    // IPersist
    STDMETHOD(GetClassID)( CLSID* );

	// IPersistFolder
    STDMETHOD(Initialize)( LPCITEMIDLIST pidl );

	// IPersistFolder2
	STDMETHOD(GetCurFolder)( LPITEMIDLIST *ppidl );

	// IShellFolder
    STDMETHOD(BindToObject)( LPCITEMIDLIST, LPBC, REFIID, void** );
	STDMETHOD(EnumObjects)( HWND, DWORD, LPENUMIDLIST* );
    STDMETHOD(CreateViewObject)( HWND, REFIID, void** );
    STDMETHOD(GetAttributesOf) ( UINT, LPCITEMIDLIST*, LPDWORD );
    STDMETHOD(GetUIObjectOf)
		( HWND, UINT, LPCITEMIDLIST*, REFIID, LPUINT, void** );
	STDMETHOD(CompareIDs)
		( LPARAM, LPCITEMIDLIST, LPCITEMIDLIST );
    STDMETHOD(BindToStorage)( LPCITEMIDLIST, LPBC, REFIID, void** )
        { return E_NOTIMPL; }
    STDMETHOD(GetDisplayNameOf)( PCUITEMID_CHILD, SHGDNF, LPSTRRET );
    STDMETHOD(ParseDisplayName)
		( HWND, LPBC, LPOLESTR, LPDWORD, LPITEMIDLIST*, LPDWORD )
        { return E_NOTIMPL; }
    STDMETHOD(SetNameOf)( HWND, LPCITEMIDLIST, LPCOLESTR, DWORD, LPITEMIDLIST* )
        { return E_NOTIMPL; }

	// IShellFolder2
	STDMETHOD(EnumSearches)( IEnumExtraSearch **ppEnum );
	STDMETHOD(GetDefaultColumn)( DWORD, ULONG *pSort, ULONG *pDisplay );
	STDMETHOD(GetDefaultColumnState)( UINT iColumn, SHCOLSTATEF *pcsFlags );		STDMETHOD(GetDefaultSearchGUID)( GUID *pguid )
		{ return E_NOTIMPL; }
	STDMETHOD(GetDetailsEx)( PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, 
							 VARIANT *pv );
	STDMETHOD(MapColumnToSCID)( UINT iColumn, SHCOLUMNID *pscid );

	// IExtractIcon
	STDMETHOD(Extract)( LPCTSTR pszFile, UINT nIconIndex, HICON *phiconLarge, 
						HICON *phiconSmall, UINT nIconSize );
	STDMETHOD(GetIconLocation)( UINT uFlags, LPTSTR szIconFile, UINT cchMax, 
								int *piIndex, UINT *pwFlags );

	// IShellDetails
	STDMETHOD(GetDetailsOf)( PCUITEMID_CHILD pidl, UINT iColumn, 
							 LPSHELLDETAILS pDetails );
	STDMETHOD(ColumnClick)( UINT iColumn );

private:
    CPidlManager       m_PidlManager;
	LPCITEMIDLIST      m_pidlRoot;
    CSwishFolder*      m_pParentFolder;
    LPITEMIDLIST       m_pidl;
	std::vector<HOSTPIDL> m_vecConnData;

	CString _GetLongNameFromPIDL( PCUITEMID_CHILD pidl, BOOL fCanonical );
	CString _GetLabelFromPIDL( PCUITEMID_CHILD pidl );
	HRESULT _FillDetailsVariant( PCWSTR szDetail, VARIANT *pv );


	// PROPERTYKEY fields for GetDetailsEx and MapColumnToSCID



};

// Host column property IDs
enum PID_SWISH_HOST {
	PID_SWISH_HOST_USER = PID_FIRST_USABLE,
	PID_SWISH_HOST_PORT
};

// PKEYs for custom swish details/properties
// Swish Host FMTID GUID {b816a850-5022-11dc-9153-0090f5284f85}
DEFINE_PROPERTYKEY(PKEY_SwishHostUser, 0xb816a850, 0x5022, 0x11dc, \
				   0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, 0x4f, 0x85, \
				   PID_SWISH_HOST_USER);
DEFINE_PROPERTYKEY(PKEY_SwishHostPort, 0xb816a850, 0x5022, 0x11dc, \
				   0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, 0x4f, 0x85, \
				   PID_SWISH_HOST_PORT);

#endif // SWISHFOLDER_H
