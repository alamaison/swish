/**
    @file

    Fixtures common to several test cases that use Boost.Test.

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

#include "swish/boost_process.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/system_error.hpp>  // For system_error

#include <Winsock2.h>  // For WSAStartup/Cleanup
#include <objbase.h>  // For CoInitialize/Uninitialize

#include <string>

namespace test {

/**
 * Fixture that initialises and uninitialises COM.
 */
class ComFixture
{
public:
	ComFixture()
	{
		HRESULT hr = ::CoInitialize(NULL);
		BOOST_WARN_MESSAGE(SUCCEEDED(hr), "::CoInitialize failed");
	}

	~ComFixture()
	{
		::CoUninitialize();
	}
};

/**
 * Fixture that initialises and uninitialises Winsock.
 */
class WinsockFixture
{
public:
	WinsockFixture()
	{
		WSADATA wsadata;
		int err = ::WSAStartup(MAKEWORD(2, 2), &wsadata);
		if (err)
			throw boost::system::system_error(
				err, boost::system::get_system_category());
	}

	~WinsockFixture()
	{
		::WSACleanup();
	}
};

/**
 * Fixture that starts and stops a local OpenSSH server instance.
 */
class OpenSshFixture : public WinsockFixture
{
public:
	OpenSshFixture();
	~OpenSshFixture();

	int StopServer();

	std::string GetHost() const;
	std::string GetUser() const;
	int GetPort() const;
	boost::filesystem::path PrivateKeyPath() const;
	boost::filesystem::path PublicKeyPath() const;
	std::string ToRemotePath(
		boost::filesystem::path local_path) const;
	boost::filesystem::wpath ToRemotePath(
		boost::filesystem::wpath local_path) const;

private:
	int m_port;
	boost::process::child m_sshd;
};

/**
 * Fixture that creates and destroys a sandbox directory.
 */
class SandboxFixture
{
public:
	SandboxFixture();
	~SandboxFixture();

	boost::filesystem::wpath Sandbox();
	boost::filesystem::wpath NewFileInSandbox();

private:
	boost::filesystem::wpath m_sandbox;
};

} // namespace test
