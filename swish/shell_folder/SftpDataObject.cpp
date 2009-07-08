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

#include "SftpDataObject.h"

#include "HostPidl.h"
#include "RemotePidl.h"
#include "SftpDirectory.h"

using ATL::CComPtr;

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
 * @param pProvider         Backend to communicate with remote server.
 *
 * @throws  CAtlException on error.
 */
void CSftpDataObject::Initialize(
	UINT cPidl, PCUITEMID_CHILD_ARRAY aPidl, 
	PCIDLIST_ABSOLUTE pidlCommonParent, ISftpProvider *pProvider)
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

	// Save backend session instance
	m_spProvider = pProvider;
}

/*----------------------------------------------------------------------------*
 * IDataObject methods
 *----------------------------------------------------------------------------*/

STDMETHODIMP CSftpDataObject::GetData(
	FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
	::ZeroMemory(pmedium, sizeof(STGMEDIUM));

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
	ExpandedList descriptors;
	_ExpandPidlsInto(descriptors);

	CFileGroupDescriptor fgd(static_cast<UINT>(descriptors.size()));

	for (UINT i = 0; i < descriptors.size(); ++i)
	{
		fgd.SetDescriptor(i, descriptors[i]);
	}

	ATLASSERT(descriptors.size() == fgd.GetSize());
	return fgd;
}

/**
 * Create an IStream for relative path stored in the lindexth FILEDESCRIPTOR.
 *
 * @p lindex corresponds to an item in the File Group Descriptor (which
 * we created in _DelayRenderCfFileGroupDescriptor) with the same index.
 *
 * @note Asking for an IStream to folder may not break (libssh2 can do 
 * this) but it is a waste of effort. Explorer won't use it, nor should it.
 */
CComPtr<IStream> CSftpDataObject::_CreateFileContentsStream(long lindex)
throw(...)
{
	ATLENSURE(m_fRenderedDescriptor);

	// Pull the FILEGROUPDESCRIPTOR we made earlier out of the DataObject
	CFormatEtc fetc(m_cfFileDescriptor);
	STGMEDIUM stg;
	HRESULT hr = GetData(&fetc, &stg);
	ATLENSURE_SUCCEEDED(hr);
	CFileGroupDescriptor fgd(stg.hGlobal);

	// Get stream from relative path stored in the lindexth FILEDESCRIPTOR
	CFileDescriptor fd = fgd.GetDescriptor(lindex);
	fgd.Detach();

	CSftpDirectory dir(m_pidlCommonParent, m_spProvider);
	return dir.GetFileByPath(fd.GetPath());
}

/**
 * Expand all top-level PIDLs into a list of CFileDescriptors with relative 
 * paths.
 *
 * There should be a file descriptor for every item in the directory 
 * heirarchies.  Once expanded, this should not need to be done again for this
 * DataObject as the descriptors will be saved in the superclass.
 *
 * In an attempt to reduce the memory footprint of this very expensive
 * operation to an absolute minimum, all expansion is done by appending to a
 * single container by reference.
 */
void CSftpDataObject::_ExpandPidlsInto(ExpandedList& descriptors)
const throw(...)
{
	for (UINT i = 0; i < m_pidls.size(); ++i)
	{
		_ExpandTopLevelPidlInto(m_pidls[i], descriptors);
	}
}

/**
 * Expand one of the selected PIDLs to include any descendents.
 *
 * If the given PIDL is a simple item, the returned list just contains this
 * PIDL.  However, if it a directory it will contain the PIDL followed by 
 * all the items in and below the directory.
 */
void CSftpDataObject::_ExpandTopLevelPidlInto(
	const TopLevelPidl& pidl, ExpandedList& descriptors)
const throw(...)
{
	// Add file descriptor from PIDL - common case
	ATLENSURE_THROW(
		descriptors.size() < descriptors.max_size() - 1, E_OUTOFMEMORY);
	descriptors.push_back(CFileDescriptor(pidl, _WantProgressDialogue()));

	// Explode the contents of subfolders into the list
	if (pidl.IsFolder())
	{
		_ExpandDirectoryTreeInto(m_pidlCommonParent, pidl, descriptors);
	}
}

/**
 * Flattens the filesystem tree rooted at this directory into a list of PIDLs.
 *
 * The list includes this directory, all the items in this directory and all
 * items below any of those which are directories.
 *
 * Although called 'flat', all the PIDL are returned relative to this 
 * directory's parent and therefore, actually do maintain a record of the 
 * directory structure.
 *//*
vector<CRelativePidl> CSftpDataObject::FlattenDirectoryTree()
throw(...)
{
	vector<CRelativePidl> pidls;
	_FlattenDirectoryTreeInto(pidls, NULL);
	ATLASSERT(pidls.size() > 0);
	return pidls;
}*/

/**
 * Return a list of all the PIDLs in this directory and below as a single list.
 *
 * The PIDLs are returned appended to the end of the @p vecPidls inout 
 * parameter which reduces the amount of copying.
 *
 * All the PIDL (which are relative to this directory's parent) are prefixed with 
 * a given parent PIDL. This allows this method to be used recursively and still 
 * produce a list of PIDLs relative to a common root.
 *
 * @param[in,out] vecPidl  List of flattened PIDLs to append our flattened 
 *                         PIDLs.
 * @param[in] pidlPrefix   PIDL with which to prefix the PIDLs below this 
 *                         folder. If NULL, the returned list is relative to 
 *                         this folder.
 */
void CSftpDataObject::_ExpandDirectoryTreeInto(
	const CAbsolutePidl& pidlParent, const CRelativePidl& pidlDirectory,
	ExpandedList& descriptors)
const throw(...)
{
	CComPtr<IEnumIDList> spEnum = _GetEnumAll(
		CAbsolutePidl(pidlParent, pidlDirectory));

	// Add all items below this directory (this directory added by caller)
	HRESULT hr;
	while(true)
	{
		CRemoteItem pidl;
		hr = spEnum->Next(1, &pidl, NULL);
		if (hr != S_OK)
			break;

		// Create version of pidl relative to the common root (pidlParent)
		CRelativePidl pidlRelative(pidlDirectory, pidl);

		// Add simple item - common case
		ATLENSURE_THROW(
			descriptors.size() < descriptors.max_size() - 1, E_OUTOFMEMORY);
		descriptors.push_back(CFileDescriptor(pidlRelative, true));

		// Explode the contents of subfolders into the list
		if (pidl.IsFolder())
		{
			pidl.Delete(); // Reduce recursion footprint
			_ExpandDirectoryTreeInto(pidlParent, pidlRelative, descriptors);
		}
	}
	ATLENSURE(hr == S_FALSE);
}

CComPtr<IEnumIDList> CSftpDataObject::_GetEnumAll(const CAbsolutePidl& pidl)
const throw(...)
{
	CSftpDirectory dir(pidl, m_spProvider);
	return dir.GetEnum(
		SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN);
}

/**
 * We want a progress dialogue unless the entire selection consists of a
 * single non-directory.  This is for the FD_PROGRESSUI flag.
 *
 * @rant WHY did MS put this flag in the descriptors!!??
 */
inline bool CSftpDataObject::_WantProgressDialogue()
const throw()
{
	return m_pidls.size() > 1
		|| (m_pidls.size() == 1 && m_pidls[0].IsFolder());
}