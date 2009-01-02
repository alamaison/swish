/*  Factory which creates IDataObjects from PIDLs.

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

#include "stdafx.h"

#include "DataObject.h"
#include "HostPidl.h"
#include "RemotePidl.h"

#define SHOW_PROGRESS_THRESHOLD 10000 ///< File size threshold after which we
                                      ///< display a progress dialogue

CDataObject::CDataObject() :
	m_cfFileContents(static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(CFSTR_FILECONTENTS)))
{
}

CDataObject::~CDataObject()
{
}

HRESULT CDataObject::FinalConstruct()
{
	ATLENSURE_RETURN_HR(m_cfFileContents, E_UNEXPECTED);
	return S_OK;
}

HRESULT CDataObject::Initialize(
	CConnection& conn, PCIDLIST_ABSOLUTE pidlCommonParent, UINT cPidl,
	PCUITEMID_CHILD_ARRAY aPidl)
{
	ATLENSURE_RETURN_HR(!m_spDoInner, E_UNEXPECTED); // Initialised twice

	// Create the default shell IDataObject implementation which we are wrapping.
	//
	// Typically, aPidl is an array of child IDs and pidlCommonParent is a full
	// pointer to a PIDL for those items. However, pidlCommonParent can be NULL
	// in which case, aPidl can contain absolute PIDLs.
	//
	// For this reason, CIDLData_CreateFromIDArray expects relative PIDLs so we
	// cast the array but, ironically, true relative PIDLs are the only type that
	// would *not* be valid here.
	HRESULT hr = ::CIDLData_CreateFromIDArray(
		pidlCommonParent, cPidl, 
		reinterpret_cast<PCUIDLIST_RELATIVE_ARRAY>(aPidl), &m_spDoInner);
	ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);

	// Make a copy of the PIDLs for use later
	for (UINT i = 0; i < cPidl; i++)
	{
		m_vecPidls.push_back(CAbsolutePidl(pidlCommonParent, aPidl[i]));
	}

	// Create FILEGROUPDESCRIPTOR format which we insert into the default
	// DataObject.  We will create the FILECONTENTS formats on-demand when 
	// requested in GetData().
	if (cPidl > 0)
	{
		CFileGroupDescriptor fgd = _CreateFileGroupDescriptor(cPidl, aPidl);
		ATLASSERT(fgd.GetSize() > 0);

		// Add the descriptor to the DataObject
		CFormatEtc fetcDescriptor(CFSTR_FILEDESCRIPTOR);
		STGMEDIUM stg;
		::ZeroMemory(&stg, sizeof stg);
		stg.tymed = TYMED_HGLOBAL;
		stg.hGlobal = fgd.Detach();
		hr = m_spDoInner->SetData(&fetcDescriptor, &stg, true);
		if (FAILED(hr))
		{
			::ReleaseStgMedium(&stg);
		}
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);

		// Prod the inner DataObject with a FILEDESCRIPTOR format. This empty
		// item just registers the format with the inner DO so that calls to
		// EnumFormatEtc and others return the correct list.
		CFormatEtc fetcContents(CFSTR_FILECONTENTS);
		STGMEDIUM stgEmpty;
		::ZeroMemory(&stgEmpty, sizeof stgEmpty);
		hr = m_spDoInner->SetData(&fetcContents, &stg, true);
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
	}

	m_conn = conn;

	return hr;
}

STDMETHODIMP CDataObject::GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
	ATLENSURE_RETURN_HR(m_spDoInner, E_UNEXPECTED); // Not initialised

	if (pformatetcIn->cfFormat == m_cfFileContents)
	{
		// Validate FORMATETC
		ATLENSURE_RETURN_HR(pformatetcIn->tymed & TYMED_ISTREAM, DV_E_TYMED);
		ATLENSURE_RETURN_HR(
			pformatetcIn->dwAspect == DVASPECT_CONTENT, DV_E_DVASPECT);
		ATLENSURE_RETURN_HR(!pformatetcIn->ptd, DV_E_DVTARGETDEVICE);

		LONG lindex = pformatetcIn->lindex;
		if (lindex == -1) // Handle incorrect lindex
		{
			if (m_vecPidls.size() == 1)
				lindex = 0;
			else
				return DV_E_LINDEX;
		}
		if (m_vecPidls.size() - lindex < 0)
			return DV_E_LINDEX;

		// Fill STGMEDIUM with file contents IStream
		::ZeroMemory(pmedium, sizeof STGMEDIUM);
		pmedium->tymed = TYMED_ISTREAM;

		CComBSTR bstrPath(_ExtractPathFromPIDL(m_vecPidls[lindex]));

		return m_conn.spProvider->GetFile(bstrPath, &(pmedium->pstm));
	}
	else
	{
		return m_spDoInner->GetData(pformatetcIn, pmedium);
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

STDMETHODIMP CDataObject::SetData( 
	FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
	return m_spDoInner->SetData(pformatetc, pmedium, fRelease);
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
 * Private functions
 *----------------------------------------------------------------------------*/

/**
 * Retrieve the full path of the file on the remote system from the given PIDL.
 */
/* static */ CString CDataObject::_ExtractPathFromPIDL(
	PCIDLIST_ABSOLUTE pidl )
{
	CString strPath;

	// Find HOSTPIDL part of pidl and use it to get 'root' path of connection
	// (by root we mean the path specified by the user when they added the
	// connection to Explorer, rather than the root of the server's filesystem)
	CHostItemListHandle pidlHost = 
		CHostItemAbsoluteHandle(pidl).FindHostPidl();
	ATLASSERT(pidlHost.IsValid());

	strPath = pidlHost.GetPath();

	// Walk over RemoteItemIds and append each filename to form the path
	CRemoteItemListHandle pidlRemote = pidlHost.GetNext();
	while (pidlRemote.IsValid())
	{
		strPath += L"/";
		strPath += pidlRemote.GetFilename();
		pidlRemote = pidlRemote.GetNext();
	}

	ATLASSERT( strPath.GetLength() <= MAX_PATH_LEN );

	return strPath;
}

inline DWORD LODWORD(ULONGLONG qwSrc)
{
	return static_cast<DWORD>(qwSrc & 0xFFFFFFFF);
}

inline DWORD HIDWORD(ULONGLONG qwSrc)
{
	return static_cast<DWORD>((qwSrc >> 32) & 0xFFFFFFFF);
}

/**
 * Create FILEGROUPDESCRIPTOR format from array of one or more PIDLs.
 */
/*static*/ CFileGroupDescriptor CDataObject::_CreateFileGroupDescriptor(
	UINT cPidl, PCUITEMID_CHILD_ARRAY aPidl)
throw(...)
{
	CFileGroupDescriptor fgd(cPidl);
	for (UINT i = 0; i < cPidl; i++)
	{
		CRemoteItemListHandle pidl(aPidl[i]);

		FILEDESCRIPTOR fd;
		::ZeroMemory(&fd, sizeof fd);
		::StringCchCopy(
			fd.cFileName, ARRAYSIZE(fd.cFileName), pidl.GetFilename());

		fd.dwFlags = FD_WRITESTIME | FD_FILESIZE | FD_ATTRIBUTES;
		if (pidl.GetFileSize() > SHOW_PROGRESS_THRESHOLD)
			fd.dwFlags |= FD_PROGRESSUI;

		fd.nFileSizeLow = LODWORD(pidl.GetFileSize());
		fd.nFileSizeHigh = HIDWORD(pidl.GetFileSize());

		SYSTEMTIME st;
		ATLVERIFY(pidl.GetDateModified().GetAsSystemTime(st));
		ATLVERIFY(::SystemTimeToFileTime(&st, &fd.ftLastWriteTime));

		if (pidl.IsFolder())
		{
			fd.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
			// TODO: recursively add contents of directory to the DataObject
		}
		if (pidl.GetFilename()[0] == L'.')
			fd.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;

		fgd.SetDescriptor(i, fd);
	}

	ATLASSERT(cPidl == fgd.GetSize());
	return fgd;
}