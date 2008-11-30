/*  Factory which creates IDataObjects from PIDLs.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "StdAfx.h"
#include "DataObjectFactory.h"

#include "HostPidl.h"
#include "RemotePidl.h"
#include "ShellDataObject.h"
#include "DummyStream.h"

inline DWORD LODWORD(ULONGLONG qwSrc)
{
	return static_cast<DWORD>(qwSrc & 0xFFFFFFFF);
}

inline DWORD HIDWORD(ULONGLONG qwSrc)
{
	return static_cast<DWORD>((qwSrc >> 32) & 0xFFFFFFFF);
}

/* static */ CComPtr<IDataObject> CDataObjectFactory::CreateDataObjectFromPIDLs(
	CConnection& conn, PIDLIST_ABSOLUTE pidlCommonParent,
	UINT cPidl, PCUITEMID_CHILD_ARRAY aPidl
) throw(...)
{
	ATLTRACE(__FUNCTION__" called\n");

	HRESULT hr;

	// Create FILEGROUPDESCRIPTOR from PIDLs
	CFileGroupDescriptor fgd(cPidl);
	for (UINT i = 0; i < cPidl; i++)
	{
		CRemoteRelativePidl pidl(aPidl[i]);

		FILEDESCRIPTOR fd;
		::ZeroMemory(&fd, sizeof fd);
		::StringCchCopy(
			fd.cFileName, ARRAYSIZE(fd.cFileName), pidl.GetFilename()
		);

		fd.dwFlags = FD_WRITESTIME | FD_FILESIZE | FD_ATTRIBUTES;

		fd.nFileSizeLow = LODWORD(pidl.GetFileSize());
		fd.nFileSizeHigh = HIDWORD(pidl.GetFileSize());

		SYSTEMTIME st;
		ATLVERIFY(pidl.GetDateModified().GetAsSystemTime(st));
		ATLVERIFY(::SystemTimeToFileTime(&st, &fd.ftLastWriteTime));

		if (pidl.IsFolder())
		{
			fd.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
			// TODO: recursively add contents of the directory to the DataObject
		}
		if (pidl.GetFilename()[0] == '.')
			fd.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;

		fgd.SetDescriptor(i, fd);
	}

	// Create file contents IStreams
	std::vector<STGMEDIUM> vecStgMedia(cPidl);
	for (UINT i = 0; i < cPidl; i++)
	{		
		STGMEDIUM stg;
		::ZeroMemory(&stg, sizeof stg);
		stg.tymed = TYMED_ISTREAM;

		CAbsolutePidl pidl;
		pidl.Attach(::ILCombine(pidlCommonParent, aPidl[i]));
		CComBSTR bstrPath(_ExtractPathFromPIDL(pidl));

		hr = conn.spProvider->GetFile(bstrPath, &stg.pstm);
		vecStgMedia[i] = stg;
		// TODO: this leaks memory if we fail after this point
	}

	// Let the shell create a fully-functional DataObject populated with our
	// PIDLs.  We will fill this DataObject with the files' contents afterwards.
	//
	// Typically, aPidl is an array of child IDs and pidlCommonParent is a full
	// pointer to a PIDL for those items. However, pidlCommonParent can be NULL
	// in which case, aPidl can contain absolute PIDLs.
	//
	// For this reason, CIDLData_CreateFromIDArray expects relative PIDLs so we
	// cast the array but, ironically, true relative PIDLs are the only type that
	// would *not* be valid here.
	CComPtr<IDataObject> spDo;
	hr = ::CIDLData_CreateFromIDArray(
		pidlCommonParent, cPidl, 
		reinterpret_cast<PCUIDLIST_RELATIVE_ARRAY>(aPidl), &spDo);
	ATLENSURE_SUCCEEDED(hr);

	// Add the descriptor to the DataObject
	CFormatEtc fetc(CFSTR_FILEDESCRIPTOR);

	STGMEDIUM stg;
	::ZeroMemory(&stg, sizeof stg);
	stg.tymed = TYMED_HGLOBAL;
	stg.hGlobal = fgd.Detach();
	hr = spDo->SetData(&fetc, &stg, true);
	ATLENSURE_SUCCEEDED(hr);

	// Add file contents IStreams to DataObject
	for (UINT i = 0; i < cPidl; i++)
	{
		CFormatEtc fetc(CFSTR_FILECONTENTS, TYMED_ISTREAM, i);
		hr = spDo->SetData(&fetc, &vecStgMedia[i], true);
		ATLENSURE_SUCCEEDED(hr);
	}

	return spDo;
}

/**
 * Retrieve the full path of the file on the remote system from the given PIDL.
 */
/* static */ CString CDataObjectFactory::_ExtractPathFromPIDL(
	PCIDLIST_ABSOLUTE pidl )
{
	CString strPath;

	// Find HOSTPIDL part of pidl and use it to get 'root' path of connection
	// (by root we mean the path specified by the user when they added the
	// connection to Explorer, rather than the root of the server's filesystem)
	CHostRelativePidl pidlAbsolute(pidl);
	CHostRelativePidl pidlHost(pidlAbsolute.FindHostPidl());
	ATLASSERT(pidlHost);
	strPath = pidlHost.GetPath();

	// Walk over REMOTEPIDLs and append each filename to form the path
	CRemoteRelativePidl pidlRemote(pidlHost.GetNext());
	do {
		strPath += L"/";
		strPath += pidlRemote.GetFilename();
	} while (pidlRemote = pidlRemote.GetNext());

	ATLASSERT( strPath.GetLength() <= MAX_PATH_LEN );

	return strPath;
}

