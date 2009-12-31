/**
    @file

    Fixture for tests that need a backend data provider.

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

#include "test/common_boost/ConsumerStub.hpp"  // CConsumerStub
#include "test/common_boost/fixtures.hpp"  // SandboxFixture/ComFixture

#include "swish/shell_folder/Pool.h" // CPool
#include "swish/interfaces/SftpProvider.h"  // ISftpProvider
#include "swish/utils.hpp"  // string conversion functions

#include "swish/atl.hpp"

#include <comet/ptr.h> // com_ptr

#include <string>

namespace test {
namespace provider {

class ProviderFixture : 
	public test::common_boost::ComFixture,
	public test::common_boost::SandboxFixture,
	public test::common_boost::OpenSshFixture
{
public:

	/**
	 * Get a CProvider instance connected to the fixture SSH server.
	 */
	comet::com_ptr<ISftpProvider> ProviderFixture::Provider()
	{
		if (!m_provider)
		{
			// Create args
			comet::com_ptr<test::common_boost::CConsumerStub> consumer = 
				test::common_boost::CConsumerStub::CreateCoObject();
			consumer->SetKeyPaths(PrivateKeyPath(), PublicKeyPath());

			std::wstring user = 
				swish::utils::Utf8StringToWideString(GetUser());
			std::wstring host = 
				swish::utils::Utf8StringToWideString(GetHost());

			// Create Provider instance
			CPool pool;
			m_provider = pool.GetSession(consumer, host, user, GetPort());
		}

		return m_provider;
	}

private:
	comet::com_ptr<ISftpProvider> m_provider;
};

}} // namespace test::provider
