/**
    @file

    Helper functions for Boost.Test adding support for wide strings.

    @if licence

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

#include <string>
#include <ostream>

#pragma warning (push)
#pragma warning (disable: 4996) // 'wctomb': This function may be unsafe.

#include <boost/archive/iterators/mb_from_wchar.hpp>

#pragma warning (pop)

typedef boost::archive::iterators::mb_from_wchar<std::wstring::const_iterator>
	converter;

namespace std {

	inline std::ostream& operator<<(
		std::ostream& out, const std::wstring& wide_in)
	{
		std::string narrow_out(
			converter(wide_in.begin()), converter(wide_in.end()));
		
		out << narrow_out;
		return out;
	}

	inline std::ostream& operator<<(
		std::ostream& out, const wchar_t* wide_in)
	{
		std::wstring wstr_in(wide_in);
		std::string narrow_out(
			converter(wstr_in.begin()), converter(wstr_in.end()));
		
		out << narrow_out;
		return out;
	}
}