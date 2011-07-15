/**
    @file

    Unit tests for the SftpDirector class.

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

    @endif
*/

#include "swish/shell_folder/SftpDirectory.h"  // test subject

#include "swish/atl.hpp"   // Common ATL setup
#include "swish/host_folder/host_pidl.hpp" // create_host_itemid
#include "swish/shell_folder/RemotePidl.h" // RemoteItemId

#include "test/common_boost/helpers.hpp"  // BOOST_REQUIRE_OK
#include "test/common_boost/MockConsumer.hpp" // MockConsumer
#include "test/common_boost/MockProvider.hpp" // MockProvider

#include <winapi/shell/pidl.hpp> // apidl_t

#include <comet/datetime.h> // datetime_t
#include <comet/error.h> // com_error
#include <comet/ptr.h>  // com_ptr

#include <boost/test/unit_test.hpp>

using swish::host_folder::create_host_itemid;

using winapi::shell::pidl::apidl_t;

using comet::com_error;
using comet::com_ptr;
using comet::datetime_t;

using test::MockProvider;
using test::MockConsumer;

namespace { // private

	apidl_t test_pidl()
	{
		return apidl_t() + create_host_itemid(
			L"testhost", L"testuser", L"/tmp", 22);
	}

	class SftpDirectoryFixture
	{
	private:
		com_ptr<MockProvider> m_provider;
		com_ptr<MockConsumer> m_consumer;

	public:

		SftpDirectoryFixture()
			: m_provider(new MockProvider()), m_consumer(new MockConsumer()) {}

		CSftpDirectory directory()
		{
			return directory(test_pidl());
		}

		CSftpDirectory directory(const apidl_t& pidl)
		{
			return CSftpDirectory(pidl.get(), provider(), consumer());
		}

		com_ptr<MockProvider> provider()
		{
			return m_provider;
		}

		com_ptr<MockConsumer> consumer()
		{
			return m_consumer;
		}
	};

	void test_enum(com_ptr<IEnumIDList> pidls, SHCONTF flags)
	{
		PITEMID_CHILD pidl;
		ULONG fetched;
		HRESULT hr = pidls->Next(1, &pidl, &fetched);
		BOOST_REQUIRE_OK(hr);
		BOOST_CHECK_EQUAL(fetched, 1U);

		do {
			RemoteItemId *item = reinterpret_cast<RemoteItemId *>(pidl);

			// Check REMOTEPIDLness
			BOOST_REQUIRE_EQUAL(item->cb, sizeof(RemoteItemId));
			BOOST_REQUIRE_EQUAL(item->dwFingerprint, RemoteItemId::FINGERPRINT);

			// Check filename
			BOOST_REQUIRE_GT(::wcslen(item->wszFilename), 0U);
			if (!(flags & SHCONTF_INCLUDEHIDDEN))
				BOOST_REQUIRE_NE(item->wszFilename[0], L'.');

			// Check folderness
			if (!(flags & SHCONTF_FOLDERS))
				BOOST_REQUIRE(!item->fIsFolder);
			if (!(flags & SHCONTF_NONFOLDERS))
				BOOST_REQUIRE(item->fIsFolder);

			// Check group and owner exist
			BOOST_REQUIRE_GT(::wcslen(item->wszGroup), 0U);
			BOOST_REQUIRE_GT(::wcslen(item->wszOwner), 0U);

			// Check date validity
			BOOST_REQUIRE(datetime_t(item->dateModified).good());
			
			hr = pidls->Next(1, &pidl, &fetched);
		} while (hr == S_OK);

		BOOST_CHECK_EQUAL(hr, S_FALSE);
		BOOST_CHECK_EQUAL(fetched, 0U);
	}

	void test_enum(ATL::CComPtr<IEnumIDList> pidls, SHCONTF flags)
	{
		test_enum(com_ptr<IEnumIDList>(pidls.p), flags);
	}
}

#pragma region SftpDirectory tests
BOOST_FIXTURE_TEST_SUITE(sftp_directory_tests, SftpDirectoryFixture)

/**
 * When a provider returns no files, the SftpDirectory mustn't either.
 */
BOOST_AUTO_TEST_CASE( empty )
{
	SHCONTF flags = 
		SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;
	provider()->set_listing_behaviour(MockProvider::EmptyListing);

	com_ptr<IEnumIDList> listing = directory().GetEnum(flags);

	ULONG fetched = 1;
	PITEMID_CHILD pidl;
	BOOST_CHECK_EQUAL(listing->Next(1, &pidl, &fetched), S_FALSE);
	BOOST_CHECK_EQUAL(fetched, 0U);
}

/**
 * Requesting everything should return folder and dotted files as well.
 */
BOOST_AUTO_TEST_CASE( everything )
{
	SHCONTF flags = 
		SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;

	test_enum(directory().GetEnum(flags), flags);
}

/**
 * Requesting just folders must only return folders.
 */
BOOST_AUTO_TEST_CASE( only_folder )
{
	SHCONTF flags = SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN;

	test_enum(directory().GetEnum(flags), flags);
}

/**
 * Requesting just files must only return files.
 */
BOOST_AUTO_TEST_CASE( only_files )
{
	SHCONTF flags = SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;

	test_enum(directory().GetEnum(flags), flags);
}

/**
 * If hidden items aren't requested, they mustn't be included.
 */
BOOST_AUTO_TEST_CASE( no_hidden )
{
	SHCONTF flags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

	test_enum(directory().GetEnum(flags), flags);
}

/**
 * If hidden items aren't requested, they mustn't be included even when only
 * folders are requested.
 */
BOOST_AUTO_TEST_CASE( no_hidden_only_folders )
{
	SHCONTF flags = SHCONTF_FOLDERS;

	test_enum(directory().GetEnum(flags), flags);
}

/**
 * If hidden items aren't requested, they mustn't be included even when only
 * files are requested.
 */
BOOST_AUTO_TEST_CASE( no_hidden_only_files )
{
	SHCONTF flags = SHCONTF_NONFOLDERS;

	test_enum(directory().GetEnum(flags), flags);
}

/**
 * Rename a file where to provider doesn't request confirmation (i.e. acts
 * as though the new name doesn't already exist.  Check that it reports
 * that nothing was overwritten.
 */
BOOST_AUTO_TEST_CASE( rename )
{
	provider()->set_rename_behaviour(MockProvider::RenameOK);

	// PIDL of old file.  Would normally come from GetEnum()
	CRemoteItem pidl(L"testtmpfile");

	BOOST_CHECK_EQUAL(directory().Rename(pidl, L"renamed to"), false);
}

/**
 * Rename a file where there are multiple segments to the path.
 */
BOOST_AUTO_TEST_CASE( rename_in_subfolder )
{
	provider()->set_rename_behaviour(MockProvider::RenameOK);

	// PIDL of old file.  Would normally come from GetEnum()
	CRemoteItem pidl(L"testswishfile");

	BOOST_CHECK_EQUAL(
		directory(
			apidl_t() + create_host_itemid(
				L"testhost", L"testuser", L"/tmp/swish", 22)).Rename(
					pidl, L"renamed to"),
		false);
}

/**
 * Rename a file but make the provider request confirmation and the consumer
 * grant permission.  Check that it reports that the file was overwritten.
 */
BOOST_AUTO_TEST_CASE( rename_with_confirmation_granted )
{
	provider()->set_rename_behaviour(MockProvider::ConfirmOverwrite);
	consumer()->set_confirm_overwrite_behaviour(MockConsumer::AllowOverwrite);

	CRemoteItem pidl(L"testtmpfile");

	BOOST_CHECK_EQUAL(directory().Rename(pidl, L"renamed to"), true);
	BOOST_CHECK(consumer()->confirmed_overwrite());
}

namespace {

	bool is_com_abort(const com_error& error)
	{	
		return error.hr() == E_ABORT;
	}

	bool is_com_fail(const com_error& error)
	{	
		return error.hr() == E_FAIL;
	}
}

/**
 * Rename a file but make the provider request confirmation but the consumer
 * denies permission.  Check that it reports that nothing was overwritten.
 */
BOOST_AUTO_TEST_CASE( rename_with_confirmation_denied )
{
	provider()->set_rename_behaviour(MockProvider::ConfirmOverwrite);
	consumer()->set_confirm_overwrite_behaviour(MockConsumer::PreventOverwrite);

	CRemoteItem pidl(L"testtmpfile");

	BOOST_CHECK_EXCEPTION(
		directory().Rename(pidl, L"renamed to"), com_error, is_com_abort);
	BOOST_CHECK(consumer()->confirmed_overwrite());
}

/**
 * Handle error case where we tried to rename a file but the provider aborted.
 */
BOOST_AUTO_TEST_CASE( rename_provider_aborts )
{
	provider()->set_rename_behaviour(MockProvider::AbortRename);

	CRemoteItem pidl(L"testtmpfile");

	BOOST_CHECK_EXCEPTION(
		directory().Rename(pidl, L"renamed to"), com_error, is_com_abort);
	BOOST_CHECK(!consumer()->confirmed_overwrite());
}

/**
 * Handle error case where we tried to rename a file but the provider failed.
 */
BOOST_AUTO_TEST_CASE( rename_provider_fail )
{
	provider()->set_rename_behaviour(MockProvider::FailRename);

	CRemoteItem pidl(L"testtmpfile");

	BOOST_CHECK_EXCEPTION(
		directory().Rename(pidl, L"renamed to"), com_error, is_com_fail);
	BOOST_CHECK(!consumer()->confirmed_overwrite());
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion
