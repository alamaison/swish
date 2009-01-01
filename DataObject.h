/*  Wrapper around shell-created IDataObject adding support for FILECONTENTS.

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
*/

#pragma once

#include <objidl.h>

#include "CoFactory.h"
#include "Connection.h"
#include "Pidl.h"

#include <vector>
using std::vector;

/**
 * Pseudo-subclass of IDataObject created by CIDLData_CreateFromIDArray().
 * 
 * The shell-created DataObject is lacking in one respect: it doesn't
 * allow the storage of more than one item with the same format but
 * different lIndex value.  This rules out using it as-is for the common
 * shell scenario where the contents of a number of selected files
 * are stored in the same DataObject: only the last file is stored regardless
 * of the value of lItem passed in the FORMATETC into SetData().
 *
 * This class works around the problem by intercepting calls to the
 * shell DataObject (stored in @p m_spDoInner) and performing custom 
 * processing for CFSTR_FILECONTENTS formats.  All other requests are simply 
 * forwarded to the inner DataObject.
 *
 * Additionally, this class creates the CFSTR_FILECONTENTS IStreams (this 
 * DataObject) and CFSTR_FILEDESCRIPTOR HGLOBAL data (inner DataObject) from 
 * a list of PIDLs passed to Initialize().  This requires the class to
 * contact the server via an ISftpProvider.
 */
class CDataObject :
	public CComObjectRoot,
	public IDataObject,
	private CCoFactory<CDataObject>
{
public:

	BEGIN_COM_MAP(CDataObject)
		COM_INTERFACE_ENTRY(IDataObject)
	END_COM_MAP()

	static CComPtr<IDataObject> Create(
		__in CConnection& conn, __in PCIDLIST_ABSOLUTE pidlCommonParent,
		UINT cPidl, __in_ecount_opt(cPidl) PCUITEMID_CHILD_ARRAY aPidl)
	throw(...)
	{
		CComPtr<CDataObject> spObject = spObject->CreateCoObject();
		
		HRESULT hr = spObject->Initialize(
			conn, pidlCommonParent, cPidl, aPidl);
		ATLENSURE_SUCCEEDED(hr);

		return spObject.p;
	}

	CDataObject();
	~CDataObject();

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	HRESULT FinalConstruct();

	HRESULT Initialize(
		__in CConnection& conn, __in PCIDLIST_ABSOLUTE pidlCommonParent,
		UINT cPidl, __in_ecount_opt(cPidl) PCUITEMID_CHILD_ARRAY aPidl);

public: // IDataObject methods

	IFACEMETHODIMP GetData( 
		__in FORMATETC *pformatetcIn,
		__out STGMEDIUM *pmedium);

	IFACEMETHODIMP GetDataHere( 
		__in FORMATETC *pformatetc,
		__inout STGMEDIUM *pmedium);

	IFACEMETHODIMP QueryGetData( 
		__in_opt FORMATETC *pformatetc);

	IFACEMETHODIMP GetCanonicalFormatEtc( 
		__in_opt FORMATETC *pformatetcIn,
		__out FORMATETC *pformatetcOut);

	IFACEMETHODIMP SetData( 
		__in FORMATETC *pformatetc,
		__in STGMEDIUM *pmedium,
		__in BOOL fRelease);

	IFACEMETHODIMP EnumFormatEtc( 
		__in DWORD dwDirection,
		__deref_out_opt IEnumFORMATETC **ppenumFormatEtc);

	IFACEMETHODIMP DAdvise( 
		__in FORMATETC *pformatetc,
		__in DWORD advf,
		__in_opt IAdviseSink *pAdvSink,
		__out DWORD *pdwConnection);

	IFACEMETHODIMP DUnadvise( 
		__in DWORD dwConnection);

	IFACEMETHODIMP EnumDAdvise( 
		__deref_out_opt IEnumSTATDATA **ppenumAdvise);

private:
	CComPtr<IDataObject> m_spDoInner;       ///< Wrapped inner DataObject
	CConnection m_conn;
	CLIPFORMAT m_cfFileContents;       ///< CFSTR_FILECONTENTS
	vector<CAbsolutePidl> m_vecPidls;

	static CString _ExtractPathFromPIDL( __in PCIDLIST_ABSOLUTE pidl );
};


class CStorageMedium : public STGMEDIUM
{
public:
	~CStorageMedium() throw()
	{
		::ReleaseStgMedium(this);
	}
};

class CFormatEtc : public FORMATETC
{
public:
	CFormatEtc(
		CLIPFORMAT cfFormat, DWORD tymed = TYMED_HGLOBAL, LONG lIndex = -1, 
		DWORD dwAspect = DVASPECT_CONTENT, DVTARGETDEVICE *ptd = NULL)
		throw()
	{
		_Construct(cfFormat, tymed, lIndex, dwAspect, ptd);
	}

	CFormatEtc(
		UINT nFormat, DWORD tymed = TYMED_HGLOBAL, LONG lIndex = -1, 
		DWORD dwAspect = DVASPECT_CONTENT, DVTARGETDEVICE *ptd = NULL)
		throw()
	{
		_Construct(static_cast<CLIPFORMAT>(nFormat), 
			tymed, lIndex, dwAspect, ptd);
	}

	CFormatEtc(
		PCWSTR pszFormat, DWORD tymed = TYMED_HGLOBAL, LONG lIndex = -1, 
		DWORD dwAspect = DVASPECT_CONTENT, DVTARGETDEVICE *ptd = NULL)
		throw(...)
	{
		UINT nFormat = ::RegisterClipboardFormat(pszFormat);
		ATLENSURE_THROW(nFormat, E_INVALIDARG);

		_Construct(static_cast<CLIPFORMAT>(nFormat), 
			tymed, lIndex, dwAspect, ptd);
	}

private:
	inline void _Construct(
		CLIPFORMAT cfFormat, DWORD tymed, LONG lIndex, DWORD dwAspect, 
		DVTARGETDEVICE *ptd) throw()
	{
		this->cfFormat = cfFormat;
		this->tymed = tymed;
		this->lindex = lIndex;
		this->dwAspect = dwAspect;
		this->ptd = ptd;
	}
};