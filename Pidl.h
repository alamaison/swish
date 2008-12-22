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
 * Base class of all PIDL classes which wrap a non-const PIDL.
 */
template <typename IdListType>
class CPidlData
{
protected:

	/** @name PIDL Types */
	// @{
	typedef IdListType *PidlType;            ///< @b Non-const PIDL type
	typedef const IdListType *ConstPidlType; ///< @b Const PIDL type
	typedef PidlType Type; ///< Which of const or non-const @b this PIDL is
	// @}

	CPidlData() : m_pidl(NULL) {}
	CPidlData( __in_opt Type pidl ) throw() : m_pidl(pidl) {}
	CPidlData( __in const CPidlData& pidl ) throw() : m_pidl(pidl) {}

public:
	operator ConstPidlType() const throw()
	{
		return m_pidl;
	}

	Type m_pidl;
};

/**
 * Base class of all PIDL classes which wrap a const PIDL.
 */
template <typename IdListType>
class CPidlConstData
{
protected:

	/** @name PIDL Types */
	// @{
	typedef IdListType *PidlType;            ///< @b Non-const PIDL type
	typedef const IdListType *ConstPidlType; ///< @b Const PIDL type
	typedef ConstPidlType Type; ///< Which of const or non-const @b this PIDL is
	// @}

	CPidlConstData() : m_pidl(NULL) {}
	CPidlConstData( __in_opt Type pidl ) throw() : m_pidl(pidl) {}
	CPidlConstData( __in const CPidlConstData& pidl ) throw() : m_pidl(pidl) {}

public:
	operator ConstPidlType() const throw()
	{
		return m_pidl;
	}

	Type m_pidl;
};

/**
 * Class adding operations common to all types of PIDL: const or non-const,
 * relative, absolute or child.
 *
 * The template takes the type of the ITEMIDLIST; either relative 
 * (ITEMIDLIST_RELATIVE), absolute (ITEMIDLIST_ABSOLUTE) or child (ITEMID_CHILD). 
 * This enhances type-safety when using the PIDL with functions, etc.  The only 
 * state stored by this wrapper is the PIDL itself and so can be used anywhere 
 * a PIDL can.
 *
 * @param IdListType  The type of ITEMIDLIST whose pointer to be wrapped; 
 *                    either ITEMIDLIST_RELATIVE, ITEMIDLIST_ABSOLUTE or 
 *                    ITEMID_CHILD.
 *
 * @attention This class requires STRICT_TYPED_ITEMIDS to be defined in order
 *            to make the different types of PIDL distinct.  Without it, all
 *            three PIDL types appear to the compiler as LPITEMIDLIST.
 */
template <typename DataT>
class CPidlBase : public DataT
{
protected:
	typedef typename DataT::PidlType PidlType;
	typedef typename DataT::ConstPidlType ConstPidlType;

	CPidlBase() {}
	CPidlBase( __in_opt typename DataT::Type pidl ) throw() : DataT(pidl) {}
	CPidlBase( __in const CPidlBase& pidl ) throw() : DataT(pidl) {}

public:
	PidlType CopyTo() const throw(...)
	{
		return Clone(m_pidl);
	}

	PCUIDLIST_RELATIVE GetNext() throw()
	{
		if (m_pidl == NULL)
			return NULL;

		PCUIDLIST_RELATIVE pidl = ::ILNext(m_pidl);
		return (::ILIsEmpty(pidl)) ? NULL : pidl;
	}

	bool IsEmpty() const throw()
	{
		return ((m_pidl == NULL) || (m_pidl->mkid.cb == 0));
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
};

/** @name PIDL Handle type declarations.
 *
 * These types create PIDL wrappers with @b unmanaged lifetimes.  The PIDLs
 * are also const so cannot be modified or destroyed.
 *
 * This effect is achieved by deriving the common PIDL operations base
 * class from a const-PIDL data wrapper.
 */
typedef CPidlBase< CPidlConstData<ITEMIDLIST_RELATIVE> > CRelativePidlHandle;
typedef CPidlBase< CPidlConstData<ITEMIDLIST_ABSOLUTE> > CAbsolutePidlHandle;
typedef CPidlBase< CPidlConstData<ITEMID_CHILD> > CChildPidlHandle;

/**
 * Wrapper for a PIDL with a @b managed lifetime.
 *
 * This class augments the commmon operations provided in CPidlBase with
 * those that are specific to non-const PIDLs such as creation and destruction
 * as well as concatenation.
 *
 * The template (and those from which it is derived) takes the type of 
 * the ITEMIDLIST; either relative (ITEMIDLIST_RELATIVE), absolute 
  *(ITEMIDLIST_ABSOLUTE) or child (ITEMID_CHILD). This enhances 
 * type-safety when using the PIDL with functions, etc.  The only state 
 * stored by this wrapper is the PIDL itself and so can be used anywhere 
 * a PIDL can.
 *
 * Most methods that take a PIDL argument, including the constructors, make a
 * copy of the PIDL first, although the class can take ownership of an
 * exisiting PIDL with Attach().
 *
 * Several of the methods return a reference to the current CPidl so that
 * operations can be chained, e.g.
 * @code pidl.Attach(old).Append(item).Detach() @endcode
 *
 * @param IdListType  The type of ITEMIDLIST whose pointer to be wrapped; 
 *                    either ITEMIDLIST_RELATIVE, ITEMIDLIST_ABSOLUTE or 
 *                    ITEMID_CHILD.
 *
 * @attention This class requires STRICT_TYPED_ITEMIDS to be defined in order
 *            to make the different types of PIDL distinct.  Without it, all
 *            three PIDL types appear to the compiler as LPITEMIDLIST.
 */
template <typename IdListType>
class CPidl : public CPidlBase< CPidlData<IdListType> >
{
public:
	CPidl() {}
	CPidl( __in_opt ConstPidlType pidl ) throw(...) : CPidlBase(Clone(pidl)) {}
	CPidl( __in const CPidl& pidl ) throw(...) : CPidlBase(Clone(pidl)) {}
	CPidl& operator=( __in const CPidl& pidl ) throw(...)
	{
		if (m_pidl != pidl.m_pidl)
		{
			CopyFrom(pidl);
		}
		return *this;
	}

	/**
	 * Concatenation constructor.
	 */
	CPidl( __in_opt ConstPidlType pidl1, __in_opt PCUIDLIST_RELATIVE pidl2 )
	throw(...)
	{
		if (::ILIsEmpty(pidl1) && ::ILIsEmpty(pidl2))
			return;

		m_pidl = reinterpret_cast<PidlType>(::ILCombine(
			reinterpret_cast<PCIDLIST_ABSOLUTE>(pidl1), pidl2));
		ATLENSURE_THROW(m_pidl, E_OUTOFMEMORY);
	}
	
	/**
	 * Return raw address of PIDL to allow use as an out-parameter.
	 *
	 * @warning It's incorrect to use this if the PIDL is not NULL as doing
	 * so will bypass the lifetime management causing a memory leak.  Therefore,
	 * thefore we ASSERT in DEBUG builds.
	 */
	PidlType* operator&() throw()
	{
		ATLASSERT(m_pidl == NULL);
		return &m_pidl;
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

	CPidl& Attach( __in_opt PidlType pidl ) throw()
	{
		Delete();
		m_pidl = pidl;
		return *this;
	}

	CPidl& CopyFrom( __in_opt ConstPidlType pidl ) throw(...)
	{
		return Attach(Clone(pidl));
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

	CPidl& Append(__in_opt PCUIDLIST_RELATIVE pidl) throw(...)
	{
		if (::ILIsEmpty(pidl))
			return *this;

		return Attach(CPidl(m_pidl, pidl).Detach());
	}
};

/**
 * Wrapper around a @b relative PIDL with a @b managed lifetime.
 */
typedef CPidl<ITEMIDLIST_RELATIVE> CRelativePidl;

/**
 * Wrapper around an @b absolute PIDL with a @b managed lifetime.
 */
typedef CPidl<ITEMIDLIST_ABSOLUTE> CAbsolutePidl;

/**
 * Wrapper around a @b child PIDL with a @b managed lifetime.
 *
 * This class augments the lifetime management and PIDL modification methods
 * provided by CPidl with those that are specific to a child PIDL.  Currently
 * this only includes disabling the concatenation constructor but may be
 * added to in the future.
 */
class CChildPidl : public CPidl<ITEMID_CHILD>
{
public:

	CChildPidl() {}
	CChildPidl( __in_opt ConstPidlType pidl ) throw(...) : CPidl(pidl) {}
	CChildPidl( __in const CChildPidl& pidl ) throw(...) : CPidl(pidl) {}
	CChildPidl& operator=( __in const CChildPidl& pidl ) throw(...)
	{
		if (this != &pidl)
			CPidl::operator=(pidl);
		return *this;
	}

	// No concatenation constructor
};
