/**
    @file

    Fixture for tests that need instances of CSftpStream.

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

#include "test/common_boost/ConsumerStub.hpp"  // CConsumerStub
#include "test/common_boost/fixtures.hpp"  // SandboxFixture etc.

#include "swish/provider/Session.hpp"  // CSession
#include "swish/utils.hpp"  // String conversion functions, GetCurrentUser

#include <memory>  // auto_ptr

namespace test {
namespace provider {

/**
 * Extends the Sandbox fixture by allowing the creation of swish::provider
 * IStreams that pass through the OpenSSH server pointing to files in the
 * sandbox.
 */
class StreamFixture : 
	public test::common_boost::ComFixture,
	public test::common_boost::SandboxFixture,
	public test::common_boost::OpenSshFixture
{
public:

	/**
	 * Create an IStream instance open for writing on a temporary file
	 * in our sandbox.
	 */
	ATL::CComPtr<IStream> GetStream()
	{
		std::auto_ptr<CSession> session(_GetSession());
		std::string file = swish::utils::WideStringToUtf8String(
			NewFileInSandbox().file_string());
		std::string remote_file = ToRemotePath(file);

		ATL::CComPtr<IStream> stream = CSftpStream::Create(
			session, remote_file, CSftpStream::read | CSftpStream::write);
		return stream;
	}

private:

	/**
	 * Return a new CSession instance connected to the fixture SSH server.
	 */
	std::auto_ptr<CSession> _GetSession()
	{
		ATL::CComPtr<test::common_boost::CConsumerStub> spConsumer = 
			test::common_boost::CConsumerStub::CreateCoObject();
		spConsumer->SetKeyPaths(PrivateKeyPath(), PublicKeyPath());

		return CSessionFactory::CreateSftpSession(
			swish::utils::Utf8StringToWideString(GetHost()).c_str(),
			GetPort(), swish::utils::GetCurrentUser().c_str(), spConsumer);
	}
};

}} // namespace test::provider
