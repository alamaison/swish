/**
    @file

    Resource-managed HGLOBAL locking.

    @if license

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

    @endif
*/

#pragma once

#include <boost/system/system_error.hpp>  // system_error

#include <Windows.h> // GlobalLock/Unlock, HGLOBAL, GetLastError

namespace swish {
namespace shell_folder {
namespace data_object {

template<typename T> class GlobalLocker;

/**
 * Swap two GlobalLocker instances.
 *
 * This operation cannot fail and offers the strong guarantee.
 */
template<typename T>
inline void swap(GlobalLocker<T>& lhs, GlobalLocker<T>& rhs) throw()
{
	std::swap(lhs.m_hglobal, rhs.m_hglobal);
	std::swap(lhs.m_mem, rhs.m_mem);
}

/**
 * Resource-management (RAII) container handling locking on an HGLOBAL.
 *
 * @templateparam  Type of object the HGLOBAL points to.  The get() method
 *                 returns a pointer to an object of this type.
 */
template<typename T>
class GlobalLocker
{
public:

	/**
	 * Lock the given HGLOBAL.
	 *
	 * The HGLOBAL remains locked for the lifetime of the object.
	 *
	 * @throws system_error if locking fails.
	 */
	explicit GlobalLocker(HGLOBAL hglobal) : 
		m_hglobal(hglobal), m_mem(::GlobalLock(hglobal))
	{
		if (!m_mem)
			throw boost::system::system_error(
				::GetLastError(), boost::system::get_system_category());
	}

	/**
	 * Unlock the HGLOBAL.
	 *
	 * As the global lock functions maintain a lock-count for each
	 * HGLOBAL, our HGLOBAL may remain locked after this object is
	 * destroyed if it has been locked elsewhere.  For example, if the
	 * GlobalLocker is copied, that will increment the lock-count.
	 */
	~GlobalLocker() throw()
	{
		m_mem = NULL;
		if (m_hglobal)
		{
			BOOL result = ::GlobalUnlock(m_hglobal);
			(void) result;
			assert(result || ::GetLastError() == NO_ERROR); // Too many unlocks
		}
		m_hglobal = NULL;
	}

	/**
	 * Copy the lock.
	 *
	 * Global locking is maintains a lock-count per HGLOBAL that holds the
	 * number of outstanding locks.  It increases every time the HGLOBAL
	 * is locked and decreases on each call the GlobalUnlock().  When it 
	 * reaches zero, the global memory is actually unlocked and free to be
	 * moved.
	 *
	 * Instances of the GlobalLocker object can be copied safely as the 
	 * operation increments the lock count and so destruction of one
	 * GlobalLocker instance can't accidentally unlock the memory held by 
	 * another.
	 *
	 * @throws system_error if locking fails.
	 */
	GlobalLocker(const GlobalLocker& lock) : 
		m_hglobal(lock.m_hglobal), m_mem(::GlobalLock(m_hglobal))
	{
		if (!m_mem)
			throw boost::system::system_error(
				::GetLastError(), boost::system::get_system_category());
	}

	/**
	 * Copy-assign one lock to another.
	 *
	 * @see the copy constructor for more info on copying locks.
	 */
	GlobalLocker& operator=(GlobalLocker lock) throw()
	{
		swap(*this, lock);
		return *this;
	}

	/**
	 * Return a pointer to the item held in the HGLOBAL.
	 */
	T* get() const
	{
		return static_cast<T*>(m_mem);
	}

private:
	HGLOBAL m_hglobal;
	void* m_mem;

	friend void swap<>(GlobalLocker<T>&, GlobalLocker<T>&) throw();
};

}}} // namespace swish::shell_folder::data_object
