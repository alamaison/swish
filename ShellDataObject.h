/*  Wrapper class for Shell Data Objects containing lists of PIDLs.

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

#pragma once

#include "Pidl.h"

class CStorageMedium : public STGMEDIUM
{
public:
	~CStorageMedium() throw()
	{
		::ReleaseStgMedium(this);
	}
};

class CGlobalLock
{
public:
	CGlobalLock() throw() :
		m_hGlobal(NULL), m_pMem(NULL)
	{}
	CGlobalLock(__in HGLOBAL hGlobal) throw() : 
		m_hGlobal(hGlobal), m_pMem(::GlobalLock(m_hGlobal))
	{}

	~CGlobalLock() throw()
	{
		Clear();
	}

	/**
	 * Disable copy constructor.  If the object were copied, the old one would
	 * be destroyed which unlocks the global memory but the new copy would
	 * not be re-locked.
	 */
	CGlobalLock& operator=(const CGlobalLock&) throw();

	void Attach(__in HGLOBAL hGlobal) throw()
	{
		Clear();

		m_hGlobal = hGlobal;
		m_pMem = ::GlobalLock(m_hGlobal);
	}

	void Clear() throw()
	{
		m_pMem = NULL;
		if (m_hGlobal)
			::GlobalUnlock(m_hGlobal);
	}

	CIDA* GetCida()
	{
		return static_cast<CIDA *>(m_pMem);
	}

private:
	HGLOBAL m_hGlobal;
	PVOID m_pMem;
};

class CShellDataObject
{
public:
	CShellDataObject(__in IDataObject *pDataObj);
	~CShellDataObject();

	CAbsolutePidl GetParentFolder() throw(...);
	CRelativePidl GetRelativeFile(UINT i) throw(...);
	CAbsolutePidl GetFile(UINT i) throw(...);
	UINT GetPidlCount() throw(...);

private:
	CComPtr<IDataObject> m_spDataObj;
	CStorageMedium m_medium;
	CGlobalLock m_glock;
};
