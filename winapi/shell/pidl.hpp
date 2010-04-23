/**
    @file

    PIDL wrapper types.

    @if licence

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    @endif
*/

#ifndef WINAPI_SHELL_PIDL_HPP
#define WINAPI_SHELL_PIDL_HPP
#pragma once

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/utility/enable_if.hpp> // conditional specsation of combine

#include <exception> // bad_alloc
#include <algorithm> // swap
#include <cstring> // memcpy

#include <Objbase.h> // CoTaskMemAlloc/Free
#define STRICT_TYPED_ITEMIDS ///< Better type safety for PIDLs (must be 
                             ///< before <shtypes.h> or <shlobj.h>).
#include <ShTypes.h> // Raw PIDL types

namespace winapi {
namespace shell {
namespace pidl {

/**
 * @name  Allocators
 * These are inspired by the Standard Library allocators but differ in one
 * important respect: they allocate memory given a size in @b bytes rather
 * than a number of elements.  This is due to unorthodox nature of PIDLs.
 */
// @{

/**
 * Allocator for PIDLs using new/delete.
 *
 * @note  This is only meant for use during testing as it allows us to detect
 * memory leaks
 */
template<typename T>
struct newdelete_alloc
{
	static T* allocate(size_t size)
	{
		return reinterpret_cast<T*>(new BYTE[size]);
	}

	static void deallocate(T* mem) throw()
	{
		delete [] mem;
	}

	/**
	 * Allow reference to new/delete allocator for different element types.
	 */
	template<class Other>
	struct rebind
	{
		typedef newdelete_alloc<Other> other;
	};
};

/**
 * Allocators using the same allocation scheme are always equal regardless of
 * element type.
 */
template<typename T, typename U>
inline bool operator==(const newdelete_alloc<T>&, const newdelete_alloc<U>&)
{
	return true;
}

/**
 * Allocators using different allocation schemes are never equal regardless of
 * element type.
 */
template<typename T, typename U>
inline bool operator!=(const newdelete_alloc<T>&, const newdelete_alloc<U>&)
{
	return false;
}


/**
 * Allocator for PIDLs using the COM memory allocator.
 *
 * Although this uses CoTaskMemAlloc/CoTaskMemFree, this is compatible with
 * PIDLs allocated via the IL* functions.
 */
template<typename T>
struct cotaskmem_alloc
{
	static T* allocate(size_t size)
	{
		T* mem = reinterpret_cast<T*>(::CoTaskMemAlloc(size));
		if (!mem)
			BOOST_THROW_EXCEPTION(std::bad_alloc("not enough memory"));
		return mem;
	}

	static void deallocate(T* mem) throw()
	{
		::CoTaskMemFree(mem);
	}

	/**
	 * Allow reference to COM memory allocator for different element types.
	 *
	 * This is used, for example, when combining PIDLs where we want to
	 * allocate from the same memory pool but for a different type of PIDL.
	 */
	template<class Other>
	struct rebind
	{
		typedef cotaskmem_alloc<Other> other;
	};
};

/**
 * Allocators using the same allocation scheme are always equal regardless of
 * element type.
 */
template<typename T, typename U>
inline bool operator==(const cotaskmem_alloc<T>&, const cotaskmem_alloc<U>&)
{
	return true;
}

/**
 * Allocators using different allocation schemes are never equal regardless of
 * element type.
 */
template<typename T, typename U>
inline bool operator!=(const cotaskmem_alloc<T>&, const cotaskmem_alloc<U>&)
{
	return false;
}

// @}


namespace raw_pidl {

	/**
	 * Return address of the PIDL offset by the given amount in bytes.
	 */
	template<typename T>
	inline T* skip(T* pidl, size_t offset)
	{
		return reinterpret_cast<T*>(reinterpret_cast<BYTE*>(pidl) + offset);
	}

	template<typename T>
	inline const T* skip(const T* pidl, size_t offset)
	{
		return reinterpret_cast<const T*>(
			reinterpret_cast<const BYTE*>(pidl) + offset);
	}

	/**
	 * Return address of the next item in the PIDL.
	 */
	template<typename T>
	inline const T* next(const T* pidl)
	{
		return skip(pidl, pidl->mkid.cb);
	}

	/**
	 * Return if PIDL is considered empty (aka. desktop folder).
	 */
	template<typename T>
	inline bool empty(const T* pidl)
	{
		return (pidl == NULL) || (pidl->mkid.cb == 0);
	}

	/**
	 * Traits governing operations on raw PIDLs.
	 */
	template<typename T>
	struct traits
	{
		typedef T idlist_type; ///< Type of IDLIST

		typedef idlist_type* pidl_type; ///< Type of pointer to ITEMID
		typedef const idlist_type* const_pidl_type; ///< Type of const pointer
		                                            ///< to ITEMID

		typedef idlist_type combine_type; ///< Type of IDLIST that results
		                                  ///< from appending to one of type
		                                  ///< T*

		typedef pidl_type combine_pidl_type; ///< Type that results from 
		                                     ///< appending a PIDL to one of
		                                     ///< type T*
		
		static const bool is_appendable = true; ///< Can this type of PIDL be
		                                        ///< appended to other pidls

		static void type_check(const_pidl_type) {} ///< Check PIDLs are what
		                                           ///< they say they are
	};

	template<>
	struct traits<ITEMID_CHILD>
	{
		typedef ITEMID_CHILD idlist_type;
		typedef idlist_type* pidl_type;
		typedef const idlist_type* const_pidl_type;
		typedef ITEMIDLIST_RELATIVE combine_type;
		typedef ITEMIDLIST_RELATIVE* combine_pidl_type;
		static const bool is_appendable = true;
		static const void type_check(const_pidl_type pidl)
		{
			if (!empty(pidl) && !empty(next(pidl)))
				BOOST_THROW_EXCEPTION(
					std::invalid_argument(
						"type violation, encountered non-child pidl"));
		}
	};

	template<>
	struct traits<ITEMIDLIST_ABSOLUTE>
	{
		typedef ITEMIDLIST_ABSOLUTE idlist_type;
		typedef idlist_type* pidl_type;
		typedef const idlist_type* const_pidl_type;
		typedef idlist_type combine_type;
		typedef pidl_type combine_pidl_type;
		static const bool is_appendable = false;
		static const void type_check(const_pidl_type) {}
	};

	/**
	 * Return size of a raw PIDL in bytes.
	 * This is not necessarily the allocated size of the PIDL, just the 
	 * minumum size of the block of memory required to hold it.  There may 
	 * be more memory after the null-terminator.
	 */
	template<typename T>
	inline size_t size(const T* pidl)
	{
		if (!pidl)
			return 0;

		size_t s = sizeof(pidl->mkid.cb);
		while (pidl->mkid.cb)
		{
			s += pidl->mkid.cb;
			pidl = next(pidl);
		}

		return s;
	}

	/**
	 * Clone a raw PIDL.
	 */
	template<typename Alloc, typename T>
	inline T* clone(const T* pidl)
	{
		if (!pidl)
			return NULL;

		size_t len = size(pidl);

		T* mem = Alloc::allocate(len); // XXX: What happens if copy fails?!
		std::memcpy(mem, pidl, len);

		return mem;
	}

	/**
	 * Clone a raw PIDL after a type check that throws on failure.
	 *
	 * Applies the type_check from the PIDL traits class before cloning the PIDL.
	 * This catches the case where a non-child PIDL masquerades as a child (as
	 * well as any other type policy the caller chooses to mandate).
	 */
	template<typename Alloc, typename T>
	inline T* type_checked_clone(const T* pidl)
	{
		traits<T>::type_check(pidl);

		return clone<Alloc>(pidl);
	}

	/**
	 * Return a newly-allocated raw PIDL with the contents of both given PIDLs.
	 *
	 * The PIDLs are not simply appended; the null-terminator of the left-hand
	 * PIDL (if any) is removed.  The new PIDL is allocated using an allocator
	 * of the same variety as the left-hand operand.
	 *
	 * The template will fail to compile if used with an absolute PIDL as the
	 * right-hand operand.  It doesn't make sense to join an abolute PIDL onto
	 * the end of something else.
	 *
	 * @retval NULL              If both PIDLs are NULL
	 * @retval copy of rhs_pidl  If lhs_pidl is empty or NULL
	 * @retval copy of lhs_pidl  If rhs_pidl is empty or NULL
	 * @retval combined PIDL     Otherwise
	 */
	template<typename Alloc, typename T, typename U>
	inline typename traits<T>::combine_pidl_type combine(
		const T* lhs_pidl, const U* rhs_pidl,
		typename boost::enable_if_c<traits<U>::is_appendable>::type* dummy=0)
	{
		(void) dummy;
		typedef typename traits<T>::combine_type return_item_type;
		typedef typename Alloc::rebind<return_item_type>::other 
			allocator_type;

		if (!lhs_pidl && !rhs_pidl)
			return NULL;

		size_t lhs_len = size(lhs_pidl);
		size_t rhs_len = size(rhs_pidl);
		assert(lhs_len + rhs_len >= sizeof(lhs_pidl->mkid.cb));

		size_t len = lhs_len + rhs_len;
		if (lhs_len && rhs_len)
			len -= sizeof(lhs_pidl->mkid.cb);

		traits<T>::combine_pidl_type mem = allocator_type::allocate(len);
		std::memcpy(mem, lhs_pidl, lhs_len);
		std::memcpy(
			skip(mem, lhs_len - ((lhs_len) ? sizeof(lhs_pidl->mkid.cb) : 0)),
			rhs_pidl, rhs_len);

		return mem;
	}
}

/**
 * Templated PIDL wrapper class.
 *
 * Takes the particular type of raw PIDL (e.g. child - ITEMID_CHILD,
 * relative - ITEMIDLIST_RELATIVE or absolute ITEMIDLIST_ABSOLUTE) as a
 * template parameter, T, ensuring that the operations remain safe with
 * respect to the underlying PIDL type.
 *
 * The template also takes a parameter, Alloc, which specifies an allocator
 * to use when creating new PIDLs.  This allows us to use new & delete rather
 * than the COM allocator (the usual allocator for PIDLs) when testing so that
 * we can detect memory leaks.
 */
template<typename T, typename Alloc>
class basic_pidl
{
public:

	typedef T                                                value_type;
	typedef T*                                               pointer;
	typedef const T*                                         const_pointer;
	typedef Alloc                                            allocator;
	typedef typename raw_pidl::traits<T>::combine_type       join_type;
	typedef typename raw_pidl::traits<T>::combine_pidl_type  join_pidl_type;
	typedef typename allocator::rebind<join_type>::other     join_allocator;
	typedef typename basic_pidl<join_type, join_allocator>   join_pidl;

	basic_pidl() : m_pidl(NULL), m_allocator(Alloc()) {}

	~basic_pidl() throw()
	{
		m_allocator.deallocate(m_pidl);
	}

	/**
	 * Copy construction.
	 */
	basic_pidl(const basic_pidl& pidl) : 
		m_pidl(typename raw_pidl::clone<Alloc>(pidl.m_pidl)) {}

	/**
	 * Construct by copying a raw PIDL.
	 */
	basic_pidl(const T* raw_pidl) : 
		m_pidl(typename raw_pidl::type_checked_clone<Alloc>(raw_pidl)) {}

	/**
	 * Copy assignment.
	 */
	basic_pidl& operator=(const basic_pidl& pidl)
	{
		basic_pidl copy(pidl);
		swap(copy);
		return *this;
	}

	/**
	 * Copy a raw PIDL into this wrapper instance.
	 */
	basic_pidl& operator=(const T* raw_pidl)
	{
		basic_pidl copy(raw_pidl);
		swap(copy);
		return *this;
	}

	/**
	 * Result of comparing with NULL.
	 */
	bool operator!() const
	{
		return !m_pidl;
	}

	/**
	 * Upcast operator.
	 * Will fail to compile unless it is legal to upcasting the underlying raw
	 * PIDL type to this PIDL's type.
	 */
	template<typename U, typename AllocU>
	operator basic_pidl<U, AllocU>() const
	{
		return m_pidl;
	}

	/**
	 * Return underlying PIDL.
	 *
	 * Returned const to prevent unexpected modification outside the wrapper.
	 */
	const T* get() const
	{
		return m_pidl;
	}

	/**
	 * Return a pointer to the internal PIDL suitable for use as an 
	 * out-parameter.
	 *
	 * The current PIDL, if any, is deallocated and set to NULL.
	 *
	 * @warning  The memory assigned to the PIDL by the caller must have been 
	 * allocated with the SAME ALLOCATOR as used by the wrapper so that it can 
	 * be destroyed in the wrapper's destructor.
	 */
	T** out()
	{
		m_allocator.deallocate(m_pidl);
		m_pidl = NULL;
		return &m_pidl;
	}

	/**
	 * Clone internal PIDL as a raw PIDL.
	 *
	 * This is needed when returning a PIDL through a COM interface where the
	 * caller takes ownership.  I.e. where the parameter is PIDLIST rather
	 * than PCIDLIST.
	 *
	 * The output pidl type is a template parameter to allow upcasting.  This
	 * will fail to compile unless it is legal to upcasting the underlying raw
	 * PIDL type to this PIDL's type.
	 *
	 * @param[out] raw_pidl  Location in which to return the address of the
	 *                       cloned PIDL.
	 */
	template<typename U>
	void copy_to(U*& raw_pidl) const
	{
		assert(&m_pidl != &raw_pidl);

		raw_pidl = typename raw_pidl::clone<Alloc>(m_pidl);
	}

	/**
	 * Attach wrapper to a raw PIDL without copying.
	 *
	 * @warning  The raw PIDL must have been allocated with the SAME ALLOCATOR
	 * as used by the wrapper so that it can be destroyed in the wrapper's
	 * destructor.
	 */
	basic_pidl& attach(T* raw_pidl)
	{
		assert(m_pidl != raw_pidl);

		raw_pidl::traits<T>::type_check(raw_pidl);

		m_allocator.deallocate(m_pidl);
		m_pidl = raw_pidl;
		return *this;
	}

	/**
	 * The size of the PIDL in bytes.
	 *
	 * This is not necessarily the allocated size of the PIDL, just the 
	 * minumum size of the block of memory required to hold it.  There may 
	 * be more memory after the null-terminator.
	 */
	size_t size() const
	{
		return raw_pidl::size(m_pidl);
	}

	/**
	 * Is the PIDL empty?
	 *
	 * Empty PIDLs are either NULL or point to a NULL-terminator.
	 */
	bool empty() const
	{
		return raw_pidl::empty(m_pidl);
	}

	/**
	 * No-fail swap.
	 */
	void swap(basic_pidl& pidl) throw()
	{
		if (m_allocator == pidl.m_allocator)
			std::swap(pidl.m_pidl, m_pidl);
		else
		{
			// Differing allocators require us to copy memory between the pools
			basic_pidl<T, Alloc> temp = *this;
			*this = pidl;
			pidl = temp;
		}
	}

private:
	T* m_pidl;
	Alloc m_allocator;
};

/**
 * @name Concatenation
 *
 * Join two PIDLs with the + operator.
 *
 * The template will fail to compile if used with an absolute PIDL as the
 * right-hand operand.  It doesn't make sense to append an abolute PIDL to
 * something else.
 *
 * @returns  A new PIDL with the contents of the second operand appended to
 *           the first and the null-terminator adjusted appropriately.
 */
// @{
template<typename T, typename U, typename Alloc, typename AllocU>
inline typename basic_pidl<T, Alloc>::join_pidl operator+(
	const basic_pidl<T, Alloc>& lhs, const basic_pidl<U, AllocU>& rhs)
{
	typedef basic_pidl<T, Alloc>::join_pidl result_type;

	typename result_type pidl;
	pidl.attach(
		raw_pidl::combine<result_type::allocator>(lhs.get(), rhs.get()));
	return pidl;
}

template<typename T, typename U, typename Alloc>
inline typename basic_pidl<T, Alloc>::join_pidl operator+(
	const basic_pidl<T, Alloc>& lhs, const U* rhs)
{
	typedef basic_pidl<U, basic_pidl<T, Alloc>::allocator::rebind<U>::other>
		wrapped_type;
	return lhs + wrapped_type(rhs);
}

template<typename T, typename U, typename Alloc>
inline typename basic_pidl<T, Alloc>::join_pidl operator+(
	const U* lhs, const basic_pidl<T, Alloc>& rhs)
{
	typedef basic_pidl<U, basic_pidl<T, Alloc>::allocator::rebind<U>::other>
		wrapped_type;
	return wrapped_type(lhs) + rhs;
}
// @}

/**
 * @name Appending
 *
 * Append one PIDL to another with the += operator.
 *
 * The template will fail to compile if used with an absolute PIDL as the
 * right-hand operand.  It doesn't make sense to append an abolute PIDL to
 * something else.
 *
 * Results in the left-hand PIDL's contents being replaced by new memory
 * that contains the contents of both PIDLs with the null-terminator adjusted
 * appropriately.
 */
//@{
template<typename T, typename U, typename Alloc, typename AllocU>
inline basic_pidl<T, Alloc>& operator+=(
	basic_pidl<T, Alloc>& lhs, const basic_pidl<U, AllocU>& rhs)
{
	lhs = lhs + rhs;
	return lhs;
}

template<typename T, typename U, typename Alloc>
inline basic_pidl<T, Alloc>& operator+=(
	basic_pidl<T, Alloc>& lhs, const U* rhs)
{
	lhs = lhs + rhs;
	return lhs;
}
//@}

/**
 * No-fail swap.
 */
template<typename T, typename Alloc>
inline void swap(
	basic_pidl<T, Alloc>& pidl1, basic_pidl<T, Alloc>& pidl2) throw()
{
	pidl1.swap(pidl2);
}

/**
 * Explicit downcast.
 */
template<typename T, typename U, typename Alloc>
inline T pidl_cast(const basic_pidl<U, Alloc>& pidl)
{
	return pidl_cast<T>(pidl.get());
}

/**
 * Explicit downcast from raw pointer to basic_pidl.
 */
template<typename T, typename U>
inline T pidl_cast(const U* raw_pidl)
{
	return static_cast<T::const_pointer>(raw_pidl);
}

/**
 * @name  Standard shell PIDL types.
 *
 * These all use the CoTaskMemAlloc allocation method.
 */
// @{
typedef basic_pidl<
	ITEMIDLIST_RELATIVE, cotaskmem_alloc<ITEMIDLIST_RELATIVE> > pidl_t;
typedef basic_pidl<
	ITEMIDLIST_ABSOLUTE, cotaskmem_alloc<ITEMIDLIST_ABSOLUTE> > apidl_t;
typedef basic_pidl<ITEMID_CHILD, cotaskmem_alloc<ITEMID_CHILD> > cpidl_t;
// @}

}}} // namespace winapi::shell::pidl

#endif
