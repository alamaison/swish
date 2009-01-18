/*  DataObject creating FILE_DESCRIPTOR/FILE_CONTENTS formats from remote data.

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
#include "SftpDataObject.h"

#include "HostPidl.h"
#include "RemotePidl.h"
#include "SftpDirectory.h"

CSftpDataObject::CSftpDataObject() :
	m_fExpandedPidlList(false),
	m_fRenderedDescriptor(false),
	m_cfPreferredDropEffect(static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT))),
	m_cfFileDescriptor(static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR))),
	m_cfFileContents(static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat(CFSTR_FILECONTENTS)))
{
}

CSftpDataObject::~CSftpDataObject()
{
}

HRESULT CSftpDataObject::FinalConstruct()
{
	ATLENSURE_RETURN_HR(m_cfPreferredDropEffect, E_UNEXPECTED);
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
 * @param conn              SFTP connection to communicate with remote server.
 *
 * @throws  CAtlException on error.
 */
void CSftpDataObject::Initialize(
	UINT cPidl, PCUITEMID_CHILD_ARRAY aPidl, 
	PCIDLIST_ABSOLUTE pidlCommonParent, CConnection& conn)
throw(...)
{
	ATLENSURE_THROW(!m_pidlCommonParent, E_UNEXPECTED); // Initialised twice

	// Initialise superclass which will create inner IDataObject
	__super::Initialize(cPidl, aPidl, pidlCommonParent);

	// Make a copy of the PIDLs.  These are used to delay-render the 
	// CFSTR_FILEDESCRIPTOR and CFSTR_FILECONTENTS format in GetData().
	m_pidlCommonParent = pidlCommonParent;
	std::copy(aPidl, aPidl + cPidl, std::back_inserter(m_pidls));

	// Prod the inner object with the formats whose data we will delay-
	// render in GetData()
	if (cPidl > 0)
	{
		HRESULT hr;
		hr = ProdInnerWithFormat(m_cfFileDescriptor);
		ATLENSURE_SUCCEEDED(hr);
		hr = ProdInnerWithFormat(m_cfFileContents);
		ATLENSURE_SUCCEEDED(hr);
	}

	// Set preferred drop effect.  This prevents any calls to GetData of FGD or
	// FILECONTENTS until drag is complete, thereby preventing interruptions
	// caused by delay-rendering.
	_RenderCfPreferredDropEffect();

	// Save connection
	m_conn = conn;
}

/*----------------------------------------------------------------------------*
 * IDataObject methods
 *----------------------------------------------------------------------------*/

STDMETHODIMP CSftpDataObject::GetData(
	FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
	::ZeroMemory(pmedium, sizeof(pmedium));

	// Delay-render data if necessary
	try
	{
		if (pformatetcIn->cfFormat == m_cfFileDescriptor)
		{
			// Delay-render CFSTR_FILEDESCRIPTOR format into this IDataObject
			_DelayRenderCfFileGroupDescriptor();
		}
		else if (pformatetcIn->cfFormat == m_cfFileContents)
		{
			// Delay-render CFSTR_FILECONTENTS format directly.  Do not store.
			*pmedium = _DelayRenderCfFileContents(pformatetcIn->lindex);
			return S_OK;
		}

		// Delegate all non-FILECONTENTS requests to the superclass
		return __super::GetData(pformatetcIn, pmedium);
	}
	catchCom()
}

/*----------------------------------------------------------------------------*
 * Private methods
 *----------------------------------------------------------------------------*/

void CSftpDataObject::_RenderCfPreferredDropEffect()
throw(...)
{
	// Create DROPEFFECT_COPY in global memory
	HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
	CGlobalLock glock(hGlobal);
	DWORD &dwDropEffect = glock.GetDword();
	dwDropEffect = DROPEFFECT_COPY;

	// Save in IDataObject
	CFormatEtc fetc(m_cfPreferredDropEffect);
	STGMEDIUM stg;
	stg.tymed = TYMED_HGLOBAL;
	stg.hGlobal = glock.Detach();
	stg.pUnkForRelease = NULL;
	HRESULT hr = SetData(&fetc, &stg, true);
	if (FAILED(hr))
	{
		::ReleaseStgMedium(&stg);
	}
	ATLENSURE_SUCCEEDED(hr);	
}

/**
 * Delay render CFSTR_FILEDESCRIPTOR format for PIDLs passed to Initialize().
 *
 * Unlike the CFSTR_SHELLIDLIST format, the file group descriptor should
 * include not only the top-level items but also any subitems within and
 * directories.  This enables Explorer to copy or move an entire directory
 * tree.
 *
 * As this operation can be very expensive when the directory tree is deep,
 * it isn't appropriate to do this when the IDataObject is created.  This
 * would lead to large delays when simply opening a directory---an operation
 * that also requires an IDataObject.  Instead, this format is delay-rendered
 * from the list of PIDLs cached during Initialize() the first time it is 
 * requested.
 *
 * @see _DelayRenderCfFileContents()
 *
 * @throws  CAtlException on error.
 */
void CSftpDataObject::_DelayRenderCfFileGroupDescriptor()
throw(...)
{
	if (!m_fRenderedDescriptor && !m_pidls.empty())
	{
		// Create FILEGROUPDESCRIPTOR format from the cached PIDL list
		CFileGroupDescriptor fgd = _CreateFileGroupDescriptor();
		ATLASSERT(fgd.GetSize() > 0);

		// Insert the descriptor into the IDataObject
		CFormatEtc fetc(m_cfFileDescriptor);
		STGMEDIUM stg;
		stg.tymed = TYMED_HGLOBAL;
		stg.hGlobal = fgd.Detach();
		stg.pUnkForRelease = NULL;
		HRESULT hr = SetData(&fetc, &stg, true);
		if (FAILED(hr))
		{
			::ReleaseStgMedium(&stg);
		}
		ATLENSURE_SUCCEEDED(hr);

		m_fRenderedDescriptor = true;
	}
}

/**
 * Delay-render a CFSTR_FILECONTENTS format for a PIDL passed to Initialize().
 *
 * Unlike the CFSTR_SHELLIDLIST format, the file contents formats should
 * include not only the top-level items but also any subitems within any
 * directories.  This enables Explorer to copy or move an entire directory
 * tree.
 *
 * As this operation can be very expensive when the directory tree is deep,
 * it isn't appropriate to do this when the IDataObject is created.  This
 * would lead to large delays when simply opening a directory---an operation
 * that also requires an IDataObject.  Instead, these formats are individually
 * delay-rendered from the list of PIDLs cached during Initialize() each time 
 * one is requested.
 *
 * @see _DelayRenderCfFileGroupDescriptor()
 *
 * @throws  CAtlException on error.
 */
STGMEDIUM CSftpDataObject::_DelayRenderCfFileContents(long lindex)
throw(...)
{
	STGMEDIUM stg;
	::ZeroMemory(&stg, sizeof(STGMEDIUM));

	if (!m_pidls.empty())
	{
		// Create an IStream from the cached PIDL list
		CComPtr<IStream> stream = _CreateFileContentsStream(lindex);
		ATLENSURE(stream);

		// Pack into a STGMEDIUM which will be returned to the client
		stg.tymed = TYMED_ISTREAM;
		stg.pstm = stream.Detach();
	}
	else
	{
		AtlThrow(DV_E_LINDEX);
	}

	return stg;
}

/**
 * Create CFSTR_FILEDESCRIPTOR format from cached PIDLs.
 */
CFileGroupDescriptor CSftpDataObject::_CreateFileGroupDescriptor()
throw(...)
{
	_ExpandPidls();

	CFileGroupDescriptor fgd(static_cast<UINT>(m_expandedPidls.size()));

	for (UINT i = 0; i < m_expandedPidls.size(); i++)
	{
		CFileDescriptor fd(m_expandedPidls[i], m_expandedPidls.size() > 1);
		fgd.SetDescriptor(i, fd);
	}

	ATLASSERT(m_expandedPidls.size() == fgd.GetSize());
	return fgd;
}

/**
 * Create IStream to the file represented by one of our cached expanded PIDLs.
 *
 * The PIDL to use is give by @p lindex and this must correspond the item in
 * the File Group Descriptor with the same index (although we do not check 
 * this).  The lindex is also the index at which this will be inserted into
 * the DataObject as a FILECONTENTS format.
 *
 * Asking for an IStream to folder may not break (libssh2 can do this) but it
 * is a waste of effort. Explorer won't use it, nor should it.
 */
CComPtr<IStream> CSftpDataObject::_CreateFileContentsStream(long lindex)
throw(...)
{
	_ExpandPidls(); // This should noop

	const ExpandedPidl& pidl = m_expandedPidls[lindex];
	CRemoteItemHandle pidlItem = pidl.GetLast(); // Our item in question

	// Create an absolute PIDL to our PIDL's parent. For top-level items this
	// is just m_pidlCommonParent but not so when pidl is not a child.
	CRelativePidl pidlParent;
	pidlParent.Attach(pidl.CopyParent());
	CAbsolutePidl pidlParentAbs(m_pidlCommonParent, pidlParent);

	CSftpDirectory directory(pidlParentAbs, m_conn);
	CComPtr<IStream> spStream = directory.GetFile(pidlItem);

	return spStream;
}

/**
 * Expand all top-level PIDLs and cache in m_expandedPidls.
 *
 * Once expanded, this should not need to be done again for this DataObject.
 * All delay-rendering will use this same expanded list.
 */
void CSftpDataObject::_ExpandPidls() throw(...)
{
	if (!m_fExpandedPidlList)
	{
		ATLASSERT(m_expandedPidls.empty());
		m_expandedPidls.clear(); // Just in case

		for (UINT i = 0; i < m_pidls.size(); i++)
		{
			ExpandedList pidls = _ExpandTopLevelPidl(m_pidls[i]);
			m_expandedPidls.insert(
				m_expandedPidls.end(), pidls.begin(), pidls.end());
		}

		m_fExpandedPidlList = true;
	}
}

/**
 * Expand one of the selected PIDLs to include any descendents.
 *
 * If the given PIDL is a simple item, the returned list just contains this
 * PIDL.  However, if it a directory it will contain the PIDL followed by 
 * all the items in and below the directory.
 */
CSftpDataObject::ExpandedList CSftpDataObject::_ExpandTopLevelPidl(
	const TopLevelPidl& pidl)
const throw(...)
{
	ExpandedList pidls;

	if (pidl.IsFolder())
	{
		CAbsolutePidl pidlFolder(m_pidlCommonParent, pidl);

		// Explode subfolder and add to lists
		CSftpDirectory subdirectory(pidlFolder, m_conn);
		vector<CRelativePidl> vecPidls = subdirectory.FlattenDirectoryTree();
		pidls.insert(pidls.end(), vecPidls.begin(), vecPidls.end());
	}
	else
	{
		// Add simple item - common case
		pidls.push_back(pidl);
	}

	return pidls;
}