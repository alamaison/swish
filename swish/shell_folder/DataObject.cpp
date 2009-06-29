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

#include "DataObject.h"

#include "swish/catch_com.hpp"  // COM catch block

CDataObject::CDataObject() :
	m_cfFileDescriptor(static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR))),
	m_cfFileContents(static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(CFSTR_FILECONTENTS)))
{
}

CDataObject::~CDataObject()
{
}

HRESULT CDataObject::FinalConstruct()
{
	ATLENSURE_RETURN_HR(m_cfFileDescriptor, E_UNEXPECTED);
	ATLENSURE_RETURN_HR(m_cfFileContents, E_UNEXPECTED);
	return S_OK;
}

/**
 * Initialise the DataObject with the top-level PIDLs.
 *
 * These PIDLs represent, for instance, the current group of files and
 * directories which have been selected in an Explorer window.  This list
 * should not include any sub-items of any of the directories.
 *
 * @param cPidl             Number of PIDLs in the selection.
 * @param aPidl             The selected PIDLs.
 * @param pidlCommonParent  PIDL to the common parent of all the PIDLs.
 *
 * @throws  CAtlException on error.
 */
void CDataObject::Initialize(
	UINT cPidl, PCUITEMID_CHILD_ARRAY aPidl, 
	PCIDLIST_ABSOLUTE pidlCommonParent)
throw(...)
{
	ATLENSURE_THROW(!m_spDoInner, E_UNEXPECTED); // Initialised twice

	// Create the default shell IDataObject implementation which we 
	// are wrapping.
	HRESULT hr = ::CIDLData_CreateFromIDArray(
		pidlCommonParent, cPidl, 
		reinterpret_cast<PCUIDLIST_RELATIVE_ARRAY>(aPidl), &m_spDoInner);
	ATLENSURE_SUCCEEDED(hr);
}


/*----------------------------------------------------------------------------*
 * IDataObject methods
 *----------------------------------------------------------------------------*/

STDMETHODIMP CDataObject::GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
	try
	{
		ATLENSURE_THROW(m_spDoInner, E_UNEXPECTED); // Not initialised

		if (pformatetcIn->cfFormat == m_cfFileContents)
		{
			// Validate FORMATETC
			ATLENSURE_THROW(pformatetcIn->tymed & TYMED_ISTREAM, DV_E_TYMED);
			ATLENSURE_THROW(
				pformatetcIn->dwAspect == DVASPECT_CONTENT, DV_E_DVASPECT);
			ATLENSURE_THROW(!pformatetcIn->ptd, DV_E_DVTARGETDEVICE);

			long lindex = pformatetcIn->lindex;

			// Handle incorrect lindex if possible
			if (lindex == -1)
			{
				ATLENSURE_THROW(m_streams.size() == 1, DV_E_LINDEX);
				lindex = 0;
			}

			// Ensure that the item is actually in our (sparse) local store
			if (m_streams.find(lindex) == m_streams.end())
				AtlThrow(DV_E_LINDEX);

			// Fill STGMEDIUM with IStream
			pmedium->pUnkForRelease = NULL;
			m_streams[lindex].CopyTo(&(pmedium->pstm));
			pmedium->tymed = TYMED_ISTREAM;

			return S_OK;
		}
		else
		{
			// Delegate all other requests to the inner IDataObject
			return m_spDoInner->GetData(pformatetcIn, pmedium);
		}
	}
	catchCom()
}

/**
 * Set a format in the DataObject.
 *
 * Which item to set is specified as a FORMATETC and the item is passed in
 * a STGMEDIUM.  If an item already exists with the specified parameters, it
 * is replaced.
 *
 * @param pformatetc  Pointer to FORMATETC specifying the format type and, for
 *                    CFSTR_FILECONTENTS formats only, the index of the item
 *                    in the DataObject.
 * @param pmedium     Pointer to STGMEDIUM holding the item to be added to the
 *                    DataObject.
 * @param fRelease    Indicates who owns the contents of the STGMEDIUM after a
 *                    call to this method.  If true, this object does.  If 
 *                    false, the caller retains ownership.
 */
STDMETHODIMP CDataObject::SetData( 
	FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
	ATLENSURE_RETURN_HR(m_spDoInner, E_UNEXPECTED); // Not initialised

	if (pformatetc->cfFormat == m_cfFileContents)
	{
		// Validate FORMATETC
		ATLENSURE_RETURN_HR(pformatetc->tymed == TYMED_ISTREAM, DV_E_TYMED);
		ATLENSURE_RETURN_HR(
			pformatetc->dwAspect == DVASPECT_CONTENT, DV_E_DVASPECT);
		ATLENSURE_RETURN_HR(!pformatetc->ptd, DV_E_DVTARGETDEVICE);
		ATLENSURE_RETURN_HR(pformatetc->lindex >= 0, DV_E_LINDEX);

		// Validate STGMEDIUM
		ATLENSURE_RETURN_HR(pmedium->tymed == pformatetc->tymed, DV_E_TYMED);
		ATLENSURE_RETURN_HR(pmedium->pstm, DV_E_STGMEDIUM);

		// Add IStream to our local store
		m_streams[pformatetc->lindex] = pmedium->pstm;

		if (fRelease) // Release STGMEDIUM if we own it now
		{
			::ReleaseStgMedium(pmedium);
		}

		// Prod inner IDataObject with empty CFSTR_FILECONTENTS format
		return ProdInnerWithFormat(pformatetc->cfFormat);
	}
	else
	{
		// Delegate all other requests to the inner IDataObject
		return m_spDoInner->SetData(pformatetc, pmedium, fRelease);
	}
}

STDMETHODIMP CDataObject::GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
	return m_spDoInner->GetDataHere(pformatetc, pmedium);
}

STDMETHODIMP CDataObject::QueryGetData(FORMATETC *pformatetc)
{
	return m_spDoInner->QueryGetData(pformatetc);
}

STDMETHODIMP CDataObject::GetCanonicalFormatEtc( 
	FORMATETC *pformatetcIn, FORMATETC *pformatetcOut)
{
	return m_spDoInner->GetCanonicalFormatEtc(pformatetcIn, pformatetcOut);
}
STDMETHODIMP CDataObject::EnumFormatEtc( 
	DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
	return m_spDoInner->EnumFormatEtc(dwDirection, ppenumFormatEtc);
}

STDMETHODIMP CDataObject::DAdvise( 
	FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, 
	DWORD *pdwConnection)
{
	return m_spDoInner->DAdvise(pformatetc, advf, pAdvSink, pdwConnection);
}

STDMETHODIMP CDataObject::DUnadvise(DWORD dwConnection)
{
	return m_spDoInner->DUnadvise(dwConnection);
}

STDMETHODIMP CDataObject::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
	return m_spDoInner->EnumDAdvise(ppenumAdvise);
}


/*----------------------------------------------------------------------------*
 * Protected methods
 *----------------------------------------------------------------------------*/

/**
 * Prod the inner DataObject with the given format.
 *
 * This sets an empty item in the inner DataObject which causes it to register
 * the existence of the format.  This ensures that calls to QueryGetData() and
 * the IEnumFORMATETC enumeration---both of which are delegated to the inner
 * object---respond correctly.
 */
HRESULT CDataObject::ProdInnerWithFormat(CLIPFORMAT nFormat)
throw()
{
	CFormatEtc fetc(nFormat);

	STGMEDIUM stgEmpty;
	::ZeroMemory(&stgEmpty, sizeof stgEmpty);

	return m_spDoInner->SetData(&fetc, &stgEmpty, true);
}