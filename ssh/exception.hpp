/**
    @file

    Interface to known-host mechanism.

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

#ifndef SSH_EXCEPTION_HPP
#define SSH_EXCEPTION_HPP
#pragma once

#include <boost/exception/exception.hpp> // boost::exception
#include <boost/shared_ptr.hpp> // shared_ptr

#include <exception> // std::exception
#include <cassert> // assert

#include <libssh2.h>

namespace ssh {
namespace exception {

/**
 * Exception type thrown when libssh2 returns an error.
 */
class ssh_error :
	public virtual boost::exception, public virtual std::exception
{
public:

	ssh_error(const char* message, int error_code)
		: boost::exception(), std::exception(),
		  m_message(message), m_error_code(error_code)
	{
	}

	ssh_error(const char* message, int len, int error_code)
		: boost::exception(), std::exception(message, len),
		  m_message(message, len), m_error_code(error_code)
	{
	}

	virtual const char* what() const
	{
		try
		{
			return m_message.c_str();
		}
		catch (std::exception&)
		{
			return "Unknown SSH error";
		}
	}

private:
	std::string m_message;
	int m_error_code;
};

/**
 * Last error encountered by the session as an exception.
 */
inline ssh_error last_error(boost::shared_ptr<LIBSSH2_SESSION> session)
{
	char* message_buf = NULL; // read-only reference
	int message_len = 0; // len not including NULL-term
	int err = libssh2_session_last_error(
		session.get(), &message_buf, &message_len, false);

	assert(err && "throwing success!");

	return ssh_error(message_buf, message_len, err);
}

}} // namespace ssh::exception

#endif
