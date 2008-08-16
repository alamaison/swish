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
 * The template takes the type of the PIDL; either relative (PIDLIST_RELATIVE),
 * absolute (PIDLIST_ABSOLUTE) or child (PITEMID_CHILD). This enhances 
 * type-safety when using the PIDL with functions, etc.  The only state stored
 * by this wrapper is the PIDL itself and so can be used anywhere a PIDL can.
 *
 * Most methods that take a PIDL argument, including the constructors, make a
 * copy of the PIDL first, although the class can take ownership of an
 * exisiting PIDL with Attach().
 *
 * @param PidlType  The type of PIDL to be wrapped; either PIDLIST_RELATIVE,
 *                  PIDLIST_ABSOLUTE or PITEMID_CHILD.
 *
 * @attention This class requires STRICT_TYPED_ITEMIDS to be defined in order
 *            to make the different types of PIDL distinct.  Whithout it, all
 *            three PIDL types appear to the compiler as LPITEMIDLIST.
 */
template <typename PidlType>
class CPidl
{
public:
	PidlType m_pidl;

public:
	CPidl() : m_pidl(NULL) {}
	CPidl( __in_opt const PidlType pidl ) : m_pidl(Clone(pidl)) {}
	CPidl( __in const CPidl& pidl ) : m_pidl(Clone(pidl)) {}
	CPidl& operator=( __in const CPidl& pidl ) throw()
	{
		if (m_pidl != pidl.m_pidl)
		{
			CopyFrom(pidl);
		}
		return *this;
	}

	~CPidl()
	{
		::ILFree(m_pidl);
	}

	void Attach( __in_opt PidlType pidl )
	{
		::ILFree(m_pidl);
		m_pidl = pidl;
	}

	void CopyFrom( __in_opt const PidlType pidl ) throw()
	{
		Attach(Clone(pidl));
	}

	PidlType Detach()
	{
		PidlType pidl = m_pidl;
		m_pidl = NULL;
		return pidl;
	}

	PidlType CopyTo() const throw()
	{
		return Clone(m_pidl);
	}

	operator const PidlType() const
	{
		return m_pidl;
	}

	static PidlType Clone( __in_opt const PidlType pidl ) throw()
	{
		if (pidl == NULL)
			return NULL;
		
		PidlType pidlOut = static_cast<PidlType>(::ILClone(pidl));
		if (pidlOut == NULL)
			AtlThrow(E_OUTOFMEMORY);

		return pidlOut;
	}
};

typedef CPidl<PITEMID_CHILD> CChildPidl;
typedef CPidl<PIDLIST_ABSOLUTE> CAbsolutePidl;
typedef CPidl<PIDLIST_RELATIVE> CRelativePidl;
