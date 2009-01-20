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
class CFileDescriptor; // Forward decl

/**
 * Subclass of CDataObject which, additionally, creates CFSTR_FILEDESCRIPTOR
 * and CFSTR_FILECONTENTS from remote data on demand.
 *
 * This class creates the CFSTR_FILEDESCRIPTOR HGLOBAL data and delegates its
 * storage to the superclass (which will, in turn, delegate it to the inner
 * object provided by the system).  
 *
 * This class also creates CFSTR_FILECONTENTS data (as IStreams) as they are 
 * requested.  Although the superclass (CDataObject) can---as with the file
 * group descriptor---store these for later, we no longer use this as doing 
 * so keeps a file-handle open to every file ever requested. This would
 * cause a large transfer to fail part way through.  Instead, we create the
 * IStreams afresh on every request.  These file-handles will close when the
 * client Releases the IStream.
 *
 * These operations are expensive---they require the DataObject to contact
 * the remote server via an ISftpProvider to retrieve file data---and may not
 * be needed if the client simply wants, say, a CFSTR_SHELLIDLIST format,
 * so delay-rendering is employed to postpone this expense until we are sure
 * it is required (GetData() is called with for one of the two formats).
 *
 * If the CFSTR_FILEDESCRIPTOR format is requested and any of the initial
 * PIDLs are directories, the PIDLs are expanded to include every item
 * anywhere with those directory trees.  Unfortunately, this is a @b very
 * expensive operation but the shell design doesn't give any way to provide
 * a partial file group descriptor.
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

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	HRESULT FinalConstruct();

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
	 * Expanded types.  The are the types that the top-levl PIDLs are expanded
	 * into when a file group descriptor is requested.  They can represent all
	 * the items in or below the top-level and are needed in order to store 
	 * entire directory trees in an IDataObject.
	 */
	// @{
	typedef CFileDescriptor ExpandedItem;
	typedef vector<ExpandedItem> ExpandedList;
	// @}

	CConnection m_conn;               ///< Connection to SFTP server

	/** @name Cached PIDLs */
	// @{
	CAbsolutePidl m_pidlCommonParent; ///< Parent of PIDLs in m_pidls
	TopLevelList m_pidls;             ///< Top-level PIDLs (the selection)
	// @}

	/** @name Registered CLIPFORMATS */
	// @{
	CLIPFORMAT m_cfPreferredDropEffect;  ///< CFSTR_PREFERREDDROPEFFECT
	CLIPFORMAT m_cfFileDescriptor;       ///< CFSTR_FILEDESCRIPTOR
	CLIPFORMAT m_cfFileContents;         ///< CFSTR_FILECONTENTS
	// @}

	void _RenderCfPreferredDropEffect() throw(...);

	/** @name Delay-rendering */
	//@{
	bool m_fExpandedPidlList;         ///< Have we expanded top-level PIDLs?
	bool m_fRenderedDescriptor;       ///< Have we rendered FileGroupDescriptor

	void _DelayRenderCfFileGroupDescriptor() throw(...);
	STGMEDIUM _DelayRenderCfFileContents(long lindex) throw(...);

	CFileGroupDescriptor _CreateFileGroupDescriptor() throw(...);
	CComPtr<IStream> _CreateFileContentsStream(long lindex) throw(...);

	void _ExpandPidlsInto(__inout ExpandedList& descriptors) const throw(...);
	void _ExpandTopLevelPidlInto(
		const TopLevelPidl& pidl, __inout ExpandedList& descriptors)
		const throw(...);
	void _ExpandDirectoryTreeInto(
		const CAbsolutePidl& pidlParent, const CRelativePidl& pidlDirectory,
		__inout ExpandedList& descriptors) const throw(...);
	CComPtr<IEnumIDList> _GetEnumAll(const CAbsolutePidl& pidl)
		const throw(...);
	inline bool _WantProgressDialogue() const throw();
	// @}
};


class CFileGroupDescriptor
{
public:
	/**
	 * Create empty.
	 */
	CFileGroupDescriptor() : m_hGlobal(NULL) {}

	/**
	 * Create from HGLOBAL to FILEGROUPDESCRIPTOR.
	 */
	CFileGroupDescriptor(HGLOBAL hGlobal) : m_hGlobal(NULL)
	{
		Attach(hGlobal);
	}

	/**
	 * Create with zeroed space for FILEDESCRIPTORs allocated.
	 */
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

	/**
	 * Copy construct.
	 */
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

	/**
	 * No copy assignment.
	 */
	CFileGroupDescriptor& operator=(const CFileGroupDescriptor& fgd);
	
	~CFileGroupDescriptor()
	{
		Delete();
	}

	/**
	 * Get number of files represented by this FILEGROUPDESCRIPTOR.
	 */
	UINT GetSize() const throw()
	{
		CGlobalLock glock(m_hGlobal);
		return glock.GetFileGroupDescriptor().cItems;
	}

	void SetDescriptor(UINT i, const FILEDESCRIPTOR& fd) throw(...)
	{
		CGlobalLock glock(m_hGlobal);

		FILEGROUPDESCRIPTOR& fgd = glock.GetFileGroupDescriptor();
		if (i >= fgd.cItems)
			AtlThrow(E_INVALIDARG); // Out-of-range

		::CopyMemory(&(fgd.fgd[i]), &fd, sizeof fd);
	}

	FILEDESCRIPTOR GetDescriptor(UINT i) throw(...)
	{
		CGlobalLock glock(m_hGlobal);

		FILEGROUPDESCRIPTOR& fgd = glock.GetFileGroupDescriptor();
		if (i >= fgd.cItems)
			AtlThrow(E_INVALIDARG); // Out-of-range

		return fgd.fgd[i];
	}

	CFileGroupDescriptor& Attach(HGLOBAL hGlobal)
	{
		Delete();
		m_hGlobal = hGlobal;
		return *this;
	}

	HGLOBAL Detach()
	{
		HGLOBAL hGlobal = m_hGlobal;
		m_hGlobal = NULL;
		return hGlobal;
	}

	void Delete()
	{
		if (m_hGlobal)
			::GlobalFree(m_hGlobal);
		m_hGlobal = NULL;
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
 *
 * @note  No destructor required as FILEDESCRIPTOR has no pointer members.
 *        cFileName is an array within the descriptor.
 */
class CFileDescriptor : public FILEDESCRIPTOR
{
public:
	CFileDescriptor(const CRemoteItemList& pidl, bool fDialogue) throw(...)
	{
		::ZeroMemory(this, sizeof(FILEDESCRIPTOR));

		// Filename
		SetPath(pidl.GetFilePath());

		// The PIDL we have been passed may be multilevel, representing a
		// path to the file.  Get last item in PIDL to get properties of the
		// file itself.
		CRemoteItemHandle pidlEnd = pidl.GetLast();

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
		else
			dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;

		if (pidlEnd.GetFilename()[0] == L'.')
			dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
	}

	CFileDescriptor(const FILEDESCRIPTOR& fd)
	{
		*static_cast<FILEDESCRIPTOR *>(this) = fd;
	}

	/**
	 * Set the cFileName field.
	 *
	 * This field often holds relative paths so this method is more 
	 * appropriately named.  A FILEDESCRIPTOR path should use Windows
	 * path separators '\\' so this methods converts any forward-slashes
	 * to back-slashes.
	 */
	CFileDescriptor& SetPath(PCWSTR pwszPath) throw()
	{
		CString strPath = pwszPath;
		UnixToWin(strPath);

		HRESULT hr = ::StringCchCopy(cFileName, ARRAYSIZE(cFileName), strPath);
		ATLASSERT(SUCCEEDED(hr));

		return *this;
	}

	/**
	 * Set the path stored in the cFileName field.
	 *
	 * This field often holds relative paths so this method is more 
	 * appropriately named.  A FILEDESCRIPTOR path should use Windows
	 * path separators '\\' but the caller expects a path in Unix format so
	 * so this methods converts any back-slashes to forward-slashes.
	 */
	CString GetPath() throw()
	{
		CString strPath = cFileName;
		WinToUnix(strPath);
		return strPath;
	}

	static inline void UnixToWin(CString& strPath) throw()
	{
		strPath.Replace(L"/", L"\\");
	}

	static inline void WinToUnix(CString& strPath) throw()
	{
		strPath.Replace(L"\\", L"/");
	}
};
