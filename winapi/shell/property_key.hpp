/**
    @file

    PROPERTYKEY wrapper.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef WINAPI_SHELL_PROPERTY_KEY_HPP
#define WINAPI_SHELL_PROPERTY_KEY_HPP
#pragma once

#include <boost/operators.hpp> // totally_ordered

#include <comet/uuid.h> // uuid_t

#include <WTypes.h> // PROPERTYKEY

#ifndef PROPERTYKEY
#define PROPERTYKEY SHCOLUMNID
#endif

namespace winapi {
namespace shell {

/**
 * C++ version of the PROPERTYKEY (aka SHCOLUMNID) struct.
 *
 * Provides total ordering for use as keys in associative containers.
 */
class property_key : boost::totally_ordered<property_key>
{
public:
	property_key(const PROPERTYKEY& pkey)
		: m_pid(pkey.pid), m_fmtid(pkey.fmtid) {}

	bool operator==(const property_key& other) const
	{
		return m_pid == other.m_pid && m_fmtid == other.m_fmtid;
	}

	bool operator<(const property_key& other) const
	{
		return m_pid < other.m_pid ||
			((m_pid == other.m_pid) && (m_fmtid < other.m_fmtid));
	}

	/**
	 * Convert to raw PROPERTYKEY struct.
	 */
	PROPERTYKEY get() const
	{
		PROPERTYKEY pkey;
		pkey.fmtid = m_fmtid;
		pkey.pid = m_pid;
		return pkey;
	}

private:
    DWORD m_pid;
	comet::uuid_t m_fmtid;
};

}} // namespace winapi::shell

#endif
