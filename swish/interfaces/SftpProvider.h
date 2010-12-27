/**
    @file

    Wrapped C++ version of ISftpProvider et. al.

    @if license

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

#ifndef SWISH_INTERFACES_SFTP_PROVIDER_H
#define SWISH_INTERFACES_SFTP_PROVIDER_H
#pragma once

#include "swish/interfaces/_SftpProvider.h" // MIDL-generated definitions

#ifdef __cplusplus

#include <comet/interface.h> // comtype
#include <comet/enum_common.h> // enumerated_type_of

#include <OleAuto.h> // SysAllocStringLen, SysStringLen, SysFreeString,
                     // VarBstrCmp

namespace comet {

template<> struct comtype<ISftpProvider>
{
	static const IID& uuid() throw() { return IID_ISftpProvider; }
	typedef IUnknown base;
};

template<> struct comtype<ISftpConsumer>
{
	static const IID& uuid() throw() { return IID_ISftpConsumer; }
	typedef IUnknown base;
};

template<> struct comtype<IEnumListing>
{
	static const IID& uuid() throw() { return IID_IEnumListing; }
	typedef IUnknown base;
};

template<> struct enumerated_type_of<IEnumListing>
{ typedef Listing is; };

}

namespace swish {

/**
 * Wrapped version of Listing that cleans up its string resources on
 * destruction.
 */
class SmartListing
{
public:

	SmartListing() : lt(Listing()) {}

	SmartListing(const SmartListing& other)
	{
		lt.bstrFilename = ::SysAllocStringLen(
			other.lt.bstrFilename, ::SysStringLen(other.lt.bstrFilename));
		lt.uPermissions = other.lt.uPermissions;
		lt.bstrOwner = ::SysAllocStringLen(
			other.lt.bstrOwner, ::SysStringLen(other.lt.bstrOwner));
		lt.bstrGroup = ::SysAllocStringLen(
			other.lt.bstrGroup, ::SysStringLen(other.lt.bstrGroup));
		lt.uUid = other.lt.uUid;
		lt.uGid = other.lt.uGid;
		lt.uSize = other.lt.uSize;
		lt.cHardLinks = other.lt.cHardLinks;
		lt.dateModified = other.lt.dateModified;
		lt.dateAccessed = other.lt.dateAccessed;
	}

	SmartListing& operator=(const SmartListing& other)
	{
		SmartListing copy(other);
		std::swap(this->lt, copy.lt);
		return *this;
	}

	~SmartListing()
	{
		::SysFreeString(lt.bstrFilename);
		::SysFreeString(lt.bstrGroup);
		::SysFreeString(lt.bstrOwner);
		std::memset(&lt, 0, sizeof(Listing));
	}

	bool operator<(const SmartListing& other) const
	{
		if (lt.bstrFilename == 0)
			return other.lt.bstrFilename != 0;

		if (other.lt.bstrFilename == 0)
			return false;

		return ::VarBstrCmp(
			lt.bstrFilename, other.lt.bstrFilename,
			::GetThreadLocale(), 0) == VARCMP_LT;
	}

	bool operator==(const SmartListing& other) const
	{
		if (lt.bstrFilename == 0 && other.lt.bstrFilename == 0)
			return true;

		return ::VarBstrCmp(
			lt.bstrFilename, other.lt.bstrFilename,
			::GetThreadLocale(), 0) == VARCMP_EQ;
	}

	bool operator==(const comet::bstr_t& name) const
	{
		return lt.bstrFilename == name;
	}

	Listing detach()
	{
		Listing out = lt;
		std::memset(&lt, 0, sizeof(Listing));
		return out;
	}

	Listing* out() { return &lt; }
	const Listing& get() const { return lt; }

private:
	Listing lt;
};

} // namespace swish


namespace comet {

/**
 * Copy-policy for use by enumerators of Listing items.
 */
template<> struct impl::type_policy<Listing>
{
	static void init(Listing& t, const swish::SmartListing& s) 
	{
		swish::SmartListing copy = s;
		t = copy.detach();
	}

	static void init(Listing& t, const Listing& s) 
	{
		t = Listing();
		t.bstrFilename = ::SysAllocStringLen(
			s.bstrFilename, ::SysStringLen(s.bstrFilename));
		t.uPermissions = s.uPermissions;
		t.bstrOwner = ::SysAllocStringLen(
			s.bstrOwner, ::SysStringLen(s.bstrOwner));
		t.bstrGroup = ::SysAllocStringLen(
			s.bstrGroup, ::SysStringLen(s.bstrGroup));
		t.uUid = s.uUid;
		t.uGid = s.uGid;
		t.uSize = s.uSize;
		t.cHardLinks = s.cHardLinks;
		t.dateModified = s.dateModified;
		t.dateAccessed = s.dateAccessed;
	}

	static void clear(Listing& t)
	{
		::SysFreeString(t.bstrFilename);
		::SysFreeString(t.bstrOwner);
		::SysFreeString(t.bstrGroup);
		t = Listing();
	}
};

} // namespace comet

#endif // __cplusplus

#endif SWISH_INTERFACES_SFTP_PROVIDER_H
