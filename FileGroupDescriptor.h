/*  FILEGROUPDESCRIPTOR and FILEDESCRIPTOR wrappers.

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

/**
 * Wrapper around the FILEGROUPDESCRIPTOR structure.
 *
 * This wrapper adds construction including, notably, from a PIDL, as well as 
 * accessors for the FILEDESCRIPTORS contained within it.
 */
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

		fgd.fgd[i] = fd;
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
		ATLASSERT(SUCCEEDED(hr)); (void)hr;

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
