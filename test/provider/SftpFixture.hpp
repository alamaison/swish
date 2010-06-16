/**
    @file

    Fixture for tests that need to access a server using SFTP.

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

#include "test/common_boost/ConsumerStub.hpp"  // CConsumerStub
#include "test/common_boost/fixtures.hpp"  // SandboxFixture etc.

#include "swish/provider/SessionFactory.hpp"  // CSessionFactory
#include "swish/utils.hpp"  // String conversion functions, GetCurrentUser

#include <comet/ptr.h> // com_ptr

#include <boost/shared_ptr.hpp>  // shared_ptr

#include <memory>  // auto_ptr

namespace test {
namespace provider {

/**
 * Test fixture providing a running SFTP server, a sandbox for test files and
 * and SFTP session object to access the server.
 */
class SftpFixture : 
	public test::ComFixture,
	public test::SandboxFixture,
	public test::OpenSshFixture
{
public:

	/**
	 * Return a new CSession instance connected to the fixture SSH server.
	 */
	boost::shared_ptr<CSession> Session()
	{
		comet::com_ptr<test::CConsumerStub> consumer =
			new test::CConsumerStub();
		consumer->SetKeyPaths(PrivateKeyPath(), PublicKeyPath());

		return boost::shared_ptr<CSession>(CSessionFactory::CreateSftpSession(
			swish::utils::Utf8StringToWideString(GetHost()).c_str(), GetPort(),
			swish::utils::Utf8StringToWideString(GetUser()).c_str(),
			consumer.get()));
	}
};

}} // namespace test::provider
