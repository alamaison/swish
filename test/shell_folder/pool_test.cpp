/**
    @file

    Tests for the SFTP directory listing helper functions.

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

#include "swish/atl.hpp"

#include "swish/shell_folder/Pool.h"  // Test subject
#include "swish/utils.hpp"

#include "test/common_boost/helpers.hpp"
#include "test/common_boost/fixtures.hpp"
#include "test/common_boost/ConsumerStub.hpp"

#include <comet/ptr.h>  // com_ptr
#include <comet/bstr.h> // bstr_t

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <string>

using swish::utils::Utf8StringToWideString;

using test::common_boost::ComFixture;
using test::common_boost::OpenSshFixture;
using test::common_boost::CConsumerStub;

using comet::com_ptr;
using comet::bstr_t;

using boost::filesystem::path;

using std::string;
using std::wstring;


namespace { // private

	/**
	 * Fixture that returns backend connections from the connection pool.
	 */
	class PoolFixture : public ComFixture, OpenSshFixture
	{
	public:
		com_ptr<ISftpProvider> GetSession()
		{
			CPool pool;
			com_ptr<CConsumerStub> consumer = CConsumerStub::CreateCoObject();
			consumer->SetKeyPaths(PrivateKeyPath(), PublicKeyPath());
			return pool.GetSession(
				consumer, Utf8StringToWideString(GetHost()).c_str(), 
				Utf8StringToWideString(GetUser()).c_str(), GetPort()).get();
		}
	};

	/**
	 * Check that the given provider responds sensibly to a request.
	 */
	void CheckAlive(const com_ptr<ISftpProvider>& provider)
	{
		com_ptr<IEnumListing> listing;
		HRESULT hr = provider->GetListing(
			bstr_t(L"/home").in(), listing.out());
		BOOST_WARN(SUCCEEDED(hr));
	}
}

BOOST_FIXTURE_TEST_SUITE(pool_tests, PoolFixture)

/**
 * Test a single call to GetSession().
 */
BOOST_AUTO_TEST_CASE( get_session )
{
	com_ptr<ISftpProvider> provider = GetSession();
	BOOST_REQUIRE(provider);
	CheckAlive(provider);
}

/**
 * Test that a second call to GetSession() returns the same instance.
 */
BOOST_AUTO_TEST_CASE( get_session_twice )
{
	com_ptr<ISftpProvider> first_provider = GetSession();
	BOOST_REQUIRE(first_provider);

	com_ptr<ISftpProvider> second_provider = GetSession();
	BOOST_REQUIRE(second_provider);
	CheckAlive(second_provider);

	BOOST_REQUIRE(second_provider == first_provider);
}

/**
 * Test that a second GetSession() after releasing the first provider
 * returns a *different* instance.
 */
BOOST_AUTO_TEST_CASE( get_session_twice_separately )
{
	ISftpProvider* first_provider = GetSession().detach();
	BOOST_REQUIRE(first_provider);

	IUnknown* first_unk;
	first_provider->QueryInterface(&first_unk);
	if (first_provider)
		first_provider->Release();
	if (first_unk)
		first_unk->Release();

	com_ptr<ISftpProvider> second_provider = GetSession();
	BOOST_REQUIRE(second_provider);
	CheckAlive(second_provider);

	com_ptr<IUnknown> second_unk(second_provider);
	BOOST_REQUIRE(first_unk != second_unk.get());
}

BOOST_AUTO_TEST_SUITE_END()
