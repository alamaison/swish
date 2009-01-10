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

#pragma once
#include "DataObject.h"

#include "RemotePidl.h"

class CFileGroupDescriptor; // Forward decl

/**
 * Subclass of CDataObject with responsibily for creating CFSTR_FILEDESCRIPTOR
 * and CFSTR_FILECONTENTS from remote data.
 *
 * This class creates the CFSTR_FILEDESCRIPTOR HGLOBAL data (stored in the inner
 * IDataObject) and the CFSTR_FILECONTENTS IStreams (stored locally in this 
 * object) on-demand from the list of PIDLs passed to Initialize().  This is
 * called delay-rendering.
 *
 * As these operations are expensive---they require the DataObject to contact
 * the remote server via an ISftpProvider to retrieve file data---and may not
 * be needed if the client simply wants, say, a CFSTR_SHELLIDLIST format,
 * delay-rendering is employed to postpone this expence untill we are sure
 * it is required (GetData() is called with for one of the two formats.
 */
class CSftpDataObject :
	public CDataObject,
	private CCoFactory<CSftpDataObject>
{
public:

	BEGIN_COM_MAP(CSftpDataObject)
		COM_INTERFACE_ENTRY(IUnknown)
		COM_INTERFACE_ENTRY_CHAIN(CDataObject)
	END_COM_MAP()

	using CCoFactory<CSftpDataObject>::CreateCoObject;

	static CComPtr<IDataObject> Create(
		UINT cPidl, __in_ecount_opt(cPidl) PCUITEMID_CHILD_ARRAY aPidl,
		__in PCIDLIST_ABSOLUTE pidlCommonParent, __in CConnection& conn)
	throw(...)
	{
		CComPtr<CSftpDataObject> spObject = spObject->CreateCoObject();
		
		spObject->Initialize(cPidl, aPidl, pidlCommonParent, conn);

		return spObject.p;
	}

	CSftpDataObject();
	virtual ~CSftpDataObject();

	void Initialize(
		UINT cPidl, __in_ecount_opt(cPidl) PCUITEMID_CHILD_ARRAY aPidl,
		__in PCIDLIST_ABSOLUTE pidlCommonParent, __in CConnection& conn)
	throw(...);

public: // IDataObject methods

	IFACEMETHODIMP GetData( 
		__in FORMATETC *pformatetcIn,
		__out STGMEDIUM *pmedium);
	
private:
	/**
	 * Top-level PIDL types. These represent currently-selected items
	 * and will always be single-level children of m_pidlCommonParent.
	 */
	// @{
	typedef CRemoteItem TopLevelPidl;
	typedef vector<TopLevelPidl> TopLevelList;
	// @}

	/**
	 * Expanded PIDL types.  These represent all the items in or below 
	 * the top-level and are needed in order to store entire directory trees
	 * in an IDataObject.
	 */
	// @{
	typedef CRemoteItemList ExpandedPidl;
	typedef vector<ExpandedPidl> ExpandedList;
	// @}

	/** Type of list of IStreams with contiguous (non-sparse) indices, 0..n. */
	typedef vector< CComPtr<IStream> > StreamList;

	CConnection m_conn;               ///< Connection to SFTP server

	/** @name Cached PIDLs */
	// @{
	CAbsolutePidl m_pidlCommonParent; ///< Parent of PIDLs in m_pidls
	TopLevelList m_pidls;             ///< Top-level PIDLs (the selection)
	// @}

	/** @name Delay-rendering */
	//@{
	bool m_fRenderedDescriptor;       ///< FileGroupDescriptor state flag
	bool m_fRenderedContents;         ///< File contents state flag

	void _DelayRenderCfFileGroupDescriptor() throw(...);
	void _DelayRenderCfFileContents() throw(...);

	StreamList _CreateFileContentsStreams() throw(...);
	CFileGroupDescriptor _CreateFileGroupDescriptor() throw(...);

	ExpandedList _ExpandTopLevelPidl(const TopLevelPidl& pidl);
	// @}
};


class CFileGroupDescriptor
{
public:
	CFileGroupDescriptor(UINT cFiles) throw(...)
		: m_hGlobal(NULL)
	{
		ATLENSURE_THROW(cFiles > 0, E_INVALIDARG);

		// Allocate global memory sufficient for group descriptor and as many
		// file descriptors as specified
		size_t cbData = _GetAllocSizeOf(cFiles);
		m_hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, cbData);
		ATLENSURE_THROW(m_hGlobal, E_OUTOFMEMORY);

		// Zero the entire block
		CGlobalLock glock(m_hGlobal);
		FILEGROUPDESCRIPTOR& fgd = glock.GetFileGroupDescriptor();
		::ZeroMemory(&fgd, cbData);
		fgd.cItems = cFiles;
	}

	CFileGroupDescriptor(const CFileGroupDescriptor& fgd) throw(...)
		: m_hGlobal(NULL)
	{
		// Calculate size of incoming
		size_t cbData = fgd._GetAllocatedSize();

		// Allocate new global of the same size
		m_hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, cbData);
		ATLENSURE_THROW(m_hGlobal, E_OUTOFMEMORY);

		// Copy
		CGlobalLock glockOld(fgd.m_hGlobal);
		CGlobalLock glockNew(m_hGlobal);
		::CopyMemory(
			&(glockNew.GetFileGroupDescriptor()),
			&(glockOld.GetFileGroupDescriptor()),
			cbData);
	}
	
	~CFileGroupDescriptor()
	{
		::GlobalFree(m_hGlobal);
		m_hGlobal = NULL;
	}

	/**
	 * Get number of files represented by this FILEGROUPDESCRIPTOR.
	 */
	UINT GetSize() const throw()
	{
		CGlobalLock glock(m_hGlobal);
		return glock.GetFileGroupDescriptor().cItems;
	}

	void SetDescriptor(UINT i, FILEDESCRIPTOR& fd)
	{
		CGlobalLock glock(m_hGlobal);

		FILEGROUPDESCRIPTOR& fgd = glock.GetFileGroupDescriptor();
		if (i >= fgd.cItems)
			AtlThrow(E_INVALIDARG); // Out-of-range

		::CopyMemory(&(fgd.fgd[i]), &fd, sizeof fd);
	}

	HGLOBAL Detach()
	{
		HGLOBAL hGlobal = m_hGlobal;
		m_hGlobal = NULL;
		return hGlobal;
	}

private:
	/**
	 * Get the size of global memory allocated for this FILEGROUPDESCRIPTOR.
	 */
	inline size_t _GetAllocatedSize() const throw()
	{
		return _GetAllocSizeOf(GetSize());
	}

	/**
	 * Get necessary size allocate descriptor for given number of files.
	 *
	 * Uses @code cFiles - 1 @endcode as the FILEGROUPDESCRIPTOR already
	 * contains one FILEDESCRIPTOR within it.
	 */
	static inline size_t _GetAllocSizeOf(UINT cFiles)
	{
		return sizeof(FILEGROUPDESCRIPTOR) + 
		       sizeof(FILEDESCRIPTOR) * (cFiles - 1);
	}

	HGLOBAL m_hGlobal; ///< Wrapped item
};


#define SHOW_PROGRESS_THRESHOLD 10000 ///< File size threshold after which we
                                      ///< display a progress dialogue

inline DWORD LODWORD(ULONGLONG qwSrc)
{
	return static_cast<DWORD>(qwSrc & 0xFFFFFFFF);
}

inline DWORD HIDWORD(ULONGLONG qwSrc)
{
	return static_cast<DWORD>((qwSrc >> 32) & 0xFFFFFFFF);
}

/**
 * FILEDESCRIPTOR wrapper adding construction from a remote PIDL.
 */
class CFileDescriptor : public FILEDESCRIPTOR
{
public:
	CFileDescriptor(const CRemoteItemList& pidl, bool fDialogue) throw(...)
	{
		::ZeroMemory(this, sizeof(FILEDESCRIPTOR));

		// Filename
		::StringCchCopy(cFileName, ARRAYSIZE(cFileName), pidl.GetFilePath());

		// The PIDL we have been passed may be multilevel, representing a
		// path to the file.  Get last item in PIDL to get properties of the
		// file itself.
		const CRemoteItem& pidlEnd = pidl.GetLast();

		// Size
		ULONGLONG uSize = pidlEnd.GetFileSize();
		nFileSizeLow = LODWORD(uSize);
		nFileSizeHigh = HIDWORD(uSize);

		// Date
		SYSTEMTIME st;
		ATLVERIFY(pidlEnd.GetDateModified().GetAsSystemTime(st));
		ATLVERIFY(::SystemTimeToFileTime(&st, &ftLastWriteTime));

		// Flags
		dwFlags = FD_WRITESTIME | FD_FILESIZE | FD_ATTRIBUTES;
		if (uSize > SHOW_PROGRESS_THRESHOLD || fDialogue)
			dwFlags |= FD_PROGRESSUI;

		if (pidlEnd.IsFolder())
			dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

		if (pidlEnd.GetFilename()[0] == L'.')
			dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
	}
};