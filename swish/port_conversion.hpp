/**
    @file

    Convert between port numbers and canonical strings.

    @if license

    Copyright (C) 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

/**
 * @file
 * Use these functions instead of lexical_cast if the port number must
 * be canonical, e.g., 65535 rather than 65,535 or 65.535.  Locales affect
 * the result of lexical_casts but canonical port numbers must not depend
 * on the locale.
 *
 * Based on a fix provided by David J.
 */

#ifndef SWISH_PORT_CONVERSION_HPP
#define SWISH_PORT_CONVERSION_HPP
#pragma once

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <locale> // locale::classic
#include <sstream> // basic_ostringstream
#include <stdexcept> // logic_error
#include <string>

namespace swish {

/**
 * Locale-independent port number to port string conversion.
 */
template<typename T>
inline T basic_port_to_string(long port)
{
	std::basic_ostringstream<T::value_type> stream;
	stream.imbue(std::locale::classic()); // force locale-independence
	stream << port;
	if (!stream)
		BOOST_THROW_EXCEPTION(
			std::logic_error("Unable to convert port number to string"));

	return stream.str();
}

/**
 * Locale-independent port number to narrow port string conversion.
 */
inline std::string port_to_string(long port)
{ return basic_port_to_string<std::string>(port); }

/**
 * Locale-independent port number to wide port string conversion.
 */
inline std::wstring port_to_wstring(long port)
{ return basic_port_to_string<std::wstring>(port); }

} // namespace swish

#endif
