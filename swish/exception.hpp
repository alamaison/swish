/**
    @file

    COM HRESULT exception based on std::exception.

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.

    @endif
*/

#pragma once

#include <string>                      // for std::string
#include <sstream>                     // for std::ostringstream
#include <exception>                   // for std::exception

#include <Windows.h>                  // for HRESULT, FormatMessage etc.

namespace { // private

	/**
	 * Turn an HRESULT into a message using the system message table.
	 *
	 * @warning LocalFree is not exception-safe according to 
	 *          boost/libs/system/src/error_code.cpp.
	 */
	static std::string message_from_hresult(::HRESULT hr)
	{
		char* pszMessage = NULL;
		unsigned long len = ::FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<char*>(&pszMessage), 0, NULL);

		if (len > 0)
		{
			// Ignore trailing newlines
			while ((pszMessage[len-1] == '\n' || pszMessage[len-1] == '\r')
				&& len > 0)
				--len;

			try
			{
				std::string message(pszMessage, len);
				::LocalFree(static_cast<HLOCAL>(pszMessage));
				pszMessage = NULL;
				return message;
			}
			catch (...)
			{
				if (pszMessage)
					::LocalFree(static_cast<HLOCAL>(pszMessage));
				throw;
			}
		}
		else
		{
			std::ostringstream message;
			message << "Unknown HRESULT: " << std::hex << std::showbase << hr;
			return message.str();
		}
	}
}

namespace swish {
namespace exception {

/**
 * Exception class holding COM HRESULT-based errors.
 *
 * The exception itself can be used anywhere an HRESULT is expected, such
 * in a return statement.  If desired the HRESULT code can be turned into
 * text description by calling what().  If the error is not a Win32 error,
 * this description cannot be found this way and what() will return:
 *     Unknown HRESULT: 0x<value of hr>
 *
 * As creating the description requires allocation of memory, it is possible
 * for it to fail which, ordinarily, would cause it to throw an exception
 * of its own.  In this case, the exception is suppressed and what returns:
 *     com_exception
 */
class com_exception : public std::exception
{
public:
	com_exception(HRESULT hr) : m_hr(hr)
	{
	}

	operator HRESULT() const throw()
	{
		return m_hr;
	}

	virtual const char* what() const throw()
	{
		try
		{
			if (m_what.empty())
			{
				m_what = message_from_hresult(m_hr);
			}
			return m_what.c_str();
		}
		catch (...)
		{
			return "com_exception";
		}
	}

private:
	HRESULT m_hr;
	mutable std::string m_what;
};

/**
 * Create a com_exception from a Win32 error code.
 * A common way to obtain such a code is through a call to ::GetLastError().
 */
inline com_exception com_exception_from_win32(DWORD code)
{
	return com_exception(HRESULT_FROM_WIN32(code));
}

}} // namespace swish::exception
