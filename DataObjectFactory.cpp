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

#include "RemotePidl.h"
#include "ShellDataObject.h"
#include "DummyStream.h"

/* static */ CComPtr<IDataObject> CDataObjectFactory::CreateDataObjectFromPIDLs(
	HWND /*hwndOwner*/, PIDLIST_ABSOLUTE pidlCommonParent,
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

		fd.nFileSizeLow = LOWORD(pidl.GetFileSize());
		fd.nFileSizeHigh = HIWORD(pidl.GetFileSize());

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

		CComObject<CDummyStream> *pDummy;
		hr = CComObject<CDummyStream>::CreateInstance(&pDummy);
		if (SUCCEEDED(hr))
		{
			pDummy->AddRef();
			hr = pDummy->QueryInterface(&stg.pstm);
			ATLASSERT(SUCCEEDED(hr));
			pDummy->Release();
		}
		
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