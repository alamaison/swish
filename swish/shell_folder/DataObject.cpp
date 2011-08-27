/**
    @file

    Wrapper around shell-created IDataObject adding support for FILECONTENTS.

    @if license

    Copyright (C) 2009, 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "DataObject.h"

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <comet/error.h> // com_error_from_interface
#include <comet/ptr.h> // com_ptr

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

using comet::com_error;
using comet::com_error_from_interface;
using comet::com_ptr;

namespace comet {

template<> struct comtype<IDataObject>
{
	static const IID& uuid() { return IID_IDataObject; }
	typedef ::IUnknown base;
};

}

namespace {

	com_ptr<IDataObject> shell_data_object_from_pidls(
		UINT cPidl, PCUITEMID_CHILD_ARRAY aPidl, 
		PCIDLIST_ABSOLUTE pidlCommonParent)
	{
		// Create the default shell IDataObject implementation which we 
		// are wrapping.
		com_ptr<IDataObject> data_object;
		HRESULT hr = ::CIDLData_CreateFromIDArray(
			pidlCommonParent, cPidl, 
			reinterpret_cast<PCUIDLIST_RELATIVE_ARRAY>(aPidl),
			data_object.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(hr));

		return data_object;
	}

}

/**
 * Construct the DataObject with the top-level PIDLs.
 *
 * These PIDLs represent, for instance, the current group of files and
 * directories which have been selected in an Explorer window.  This list
 * should not include any sub-items of any of the directories.
 *
 * @param cPidl             Number of PIDLs in the selection.
 * @param aPidl             The selected PIDLs.
 * @param pidlCommonParent  PIDL to the common parent of all the PIDLs.
 */
CDataObject::CDataObject(
	UINT cPidl, PCUITEMID_CHILD_ARRAY aPidl, 
	PCIDLIST_ABSOLUTE pidlCommonParent) :
	m_cfFileDescriptor(static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR))),
	m_cfFileContents(static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(CFSTR_FILECONTENTS))),
	m_inner(shell_data_object_from_pidls(cPidl, aPidl, pidlCommonParent))
{
}

/*----------------------------------------------------------------------------*
 * IDataObject methods
 *----------------------------------------------------------------------------*/

STDMETHODIMP CDataObject::GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
	HRESULT hr = S_OK;
	try
	{
		if (pformatetcIn->cfFormat == m_cfFileContents)
		{
			// Validate FORMATETC
			if (!(pformatetcIn->tymed & TYMED_ISTREAM))
				BOOST_THROW_EXCEPTION(com_error(DV_E_TYMED));
			if (pformatetcIn->dwAspect != DVASPECT_CONTENT)
				BOOST_THROW_EXCEPTION(com_error(DV_E_DVASPECT));
			if (pformatetcIn->ptd)
				BOOST_THROW_EXCEPTION(com_error(DV_E_DVTARGETDEVICE));

			long lindex = pformatetcIn->lindex;

			// Handle incorrect lindex if possible
			if (lindex == -1)
			{
				if (m_streams.size() != 1)
					BOOST_THROW_EXCEPTION(com_error(DV_E_LINDEX));
				lindex = 0;
			}

			// Ensure that the item is actually in our (sparse) local store
			if (m_streams.find(lindex) == m_streams.end())
				BOOST_THROW_EXCEPTION(com_error(DV_E_LINDEX));

			// Fill STGMEDIUM with IStream
			pmedium->pUnkForRelease = NULL;
			m_streams[lindex].CopyTo(&(pmedium->pstm));
			pmedium->tymed = TYMED_ISTREAM;
		}
		else
		{
			// Delegate all other requests to the inner IDataObject
			hr = m_inner->GetData(pformatetcIn, pmedium);
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(com_error_from_interface(m_inner, hr));
		}
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();
	return hr;
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
	HRESULT hr = S_OK;
	try
	{
		if (pformatetc->cfFormat == m_cfFileContents)
		{
			// Validate FORMATETC
			if (pformatetc->tymed != TYMED_ISTREAM)
				BOOST_THROW_EXCEPTION(com_error(DV_E_TYMED));
			if (pformatetc->dwAspect != DVASPECT_CONTENT)
				BOOST_THROW_EXCEPTION(com_error(DV_E_DVASPECT));
			if (pformatetc->ptd)
				BOOST_THROW_EXCEPTION(com_error(DV_E_DVTARGETDEVICE));
			if (pformatetc->lindex < 0)
				BOOST_THROW_EXCEPTION(com_error(DV_E_LINDEX));

			// Validate STGMEDIUM
			if (pmedium->tymed != pformatetc->tymed)
				BOOST_THROW_EXCEPTION(com_error(DV_E_TYMED));
			if (!pmedium->pstm)
				BOOST_THROW_EXCEPTION(com_error(DV_E_STGMEDIUM));

			// Add IStream to our local store
			m_streams[pformatetc->lindex] = pmedium->pstm;

			if (fRelease) // Release STGMEDIUM if we own it now
			{
				::ReleaseStgMedium(pmedium);
			}

			// Prod inner IDataObject with empty CFSTR_FILECONTENTS format
			hr = ProdInnerWithFormat(pformatetc->cfFormat, pformatetc->tymed);
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(com_error_from_interface(m_inner, hr));
		}
		else
		{
			// Delegate all other requests to the inner IDataObject
			hr = m_inner->SetData(pformatetc, pmedium, fRelease);
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(com_error_from_interface(m_inner, hr));
		}
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();
	return hr;
}

STDMETHODIMP CDataObject::GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
	return m_inner->GetDataHere(pformatetc, pmedium);
}

STDMETHODIMP CDataObject::QueryGetData(FORMATETC *pformatetc)
{
	return m_inner->QueryGetData(pformatetc);
}

STDMETHODIMP CDataObject::GetCanonicalFormatEtc( 
	FORMATETC *pformatetcIn, FORMATETC *pformatetcOut)
{
	return m_inner->GetCanonicalFormatEtc(pformatetcIn, pformatetcOut);
}
STDMETHODIMP CDataObject::EnumFormatEtc( 
	DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
	return m_inner->EnumFormatEtc(dwDirection, ppenumFormatEtc);
}

STDMETHODIMP CDataObject::DAdvise( 
	FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, 
	DWORD *pdwConnection)
{
	return m_inner->DAdvise(pformatetc, advf, pAdvSink, pdwConnection);
}

STDMETHODIMP CDataObject::DUnadvise(DWORD dwConnection)
{
	return m_inner->DUnadvise(dwConnection);
}

STDMETHODIMP CDataObject::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
	return m_inner->EnumDAdvise(ppenumAdvise);
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
HRESULT CDataObject::ProdInnerWithFormat(CLIPFORMAT nFormat, DWORD tymed)
throw()
{
	CFormatEtc fetc(nFormat, tymed);

	STGMEDIUM stgEmpty;
	::ZeroMemory(&stgEmpty, sizeof stgEmpty);

	return m_inner->SetData(&fetc, &stgEmpty, true);
}