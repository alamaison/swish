/**
    @file

    Fixture that runs local OpenSSH server for testing.

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

#ifndef SSH_OPENSSH_FIXTURE_HPP
#define SSH_OPENSSH_FIXTURE_HPP
#pragma once

#include "boost_process.hpp" // Boost.Process warnings wrapper
#include <boost/filesystem.hpp> // path

#include <string>

namespace test {
namespace ssh {

/**
 * Fixture that starts and stops a local OpenSSH server instance.
 */
class openssh_fixture
{
public:
	openssh_fixture();
	virtual ~openssh_fixture();

	int stop_server();

	std::string host() const;
	std::string user() const;
	int port() const;
	boost::filesystem::path private_key_path() const;
	boost::filesystem::path public_key_path() const;
	boost::filesystem::path to_remote_path(
		boost::filesystem::path local_path) const;

private:
	int m_port;
	boost::process::child m_sshd;
};

}} // namespace test::ssh

#endif
