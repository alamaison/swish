/*  PIDL wrapper classes.

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

/**
 * Wrapper for an pointer to an ITEMIDLIST otherwise knowns as a PIDL.
 *
 * The template takes the type of the ITEMIDLIST; either relative 
 * (ITEMIDLIST_RELATIVE), absolute (ITEMIDLIST_ABSOLUTE) or child 
 * (ITEMID_CHILD). This enhances type-safety when using the PIDL with 
 * functions, etc.  The only state stored by this wrapper is the PIDL itself 
 * and so can be used anywhere a PIDL can.
 *
 * Most methods that take a PIDL argument, including the constructors, make a
 * copy of the PIDL first, although the class can take ownership of an
 * exisiting PIDL with Attach().
 *
 * @param IdListType  The type of ITEMIDLIST whose pointer to be wrapped; 
 *                    either ITEMIDLIST_RELATIVE, ITEMIDLIST_ABSOLUTE or 
 *                    ITEMID_CHILD.
 *
 * @attention This class requires STRICT_TYPED_ITEMIDS to be defined in order
 *            to make the different types of PIDL distinct.  Whithout it, all
 *            three PIDL types appear to the compiler as LPITEMIDLIST.
 */
template <typename IdListType>
class CPidl
{
	typedef IdListType *PidlType;
	typedef const IdListType *ConstPidlType;

public:
	PidlType m_pidl;

public:
	CPidl() : m_pidl(NULL) {}
	CPidl( __in_opt ConstPidlType pidl ) throw(...) : m_pidl(Clone(pidl)) {}
	CPidl( __in const CPidl& pidl ) throw(...) : m_pidl(Clone(pidl)) {}
	CPidl& operator=( __in const CPidl& pidl ) throw(...)
	{
		if (m_pidl != pidl.m_pidl)
		{
			CopyFrom(pidl);
		}
		return *this;
	}

	template<typename IdListSupertype>
	operator CPidl<IdListSupertype>() const
	{
		return CPidl<IdListSupertype>(m_pidl);
	}

	~CPidl() throw()
	{
		Delete();
	}

	void Attach( __in_opt PidlType pidl ) throw()
	{
		Delete();
		m_pidl = pidl;
	}

	void CopyFrom( __in_opt ConstPidlType pidl ) throw(...)
	{
		Attach(Clone(pidl));
	}

	PidlType Detach() throw()
	{
		PidlType pidl = m_pidl;
		m_pidl = NULL;
		return pidl;
	}

	void Delete()
	{
		::ILFree(m_pidl);
		m_pidl = NULL;
	}

	PidlType CopyTo() const throw(...)
	{
		return Clone(m_pidl);
	}

	operator ConstPidlType() const throw()
	{
		return m_pidl;
	}

	/**
	 * @todo Returning a child PIDL as the result of a join is not correct. It
	 * should return a relative PIDL. How can we do this using templates?
	 */
	PidlType Join( __in_opt PCUIDLIST_RELATIVE pidl )
	{
		return reinterpret_cast<PidlType>(
			::ILCombine(reinterpret_cast<PCIDLIST_ABSOLUTE>(m_pidl), pidl)
		);
	}

	PCUIDLIST_RELATIVE GetNext()
	{
		if (m_pidl == NULL)
			return NULL;

		PCUIDLIST_RELATIVE pidl = ::ILNext(m_pidl);
		return (::ILIsEmpty(pidl)) ? NULL : pidl;
	}

	static PidlType Clone( __in_opt ConstPidlType pidl ) throw(...)
	{
		if (pidl == NULL)
			return NULL;
		
		PidlType pidlOut = static_cast<PidlType>(::ILClone(pidl));
		if (pidlOut == NULL)
			AtlThrow(E_OUTOFMEMORY);

		return pidlOut;
	}

	bool IsEmpty() const
	{
		return ((m_pidl == NULL) || (m_pidl->mkid.cb == 0));
	}
};

typedef CPidl<ITEMID_CHILD> CChildPidl;
typedef CPidl<ITEMIDLIST_ABSOLUTE> CAbsolutePidl;
typedef CPidl<ITEMIDLIST_RELATIVE> CRelativePidl;
