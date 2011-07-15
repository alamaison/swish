/**
    @file

    Tests for the remote folder IShellFolder implementation

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

#include "swish/shell_folder/RemoteFolder.h" // test subject

#include "swish/shell_folder/RemotePidl.h" // RemoteItemId

#include "test/common_boost/helpers.hpp" // BOOST_REQUIRE_OK
#include "test/common_boost/PidlFixture.hpp" // PidlFixture

#include <comet/datetime.h> // datetime_t
#include <comet/ptr.h>  // com_ptr

#include <boost/bind.hpp> // bind
#include <boost/test/unit_test.hpp>

using test::PidlFixture;

using comet::com_ptr;
using comet::datetime_t;

using boost::filesystem::wpath;

namespace { // private

	class RemoteFolderFixture : public PidlFixture
	{
	private:
		com_ptr<IShellFolder> m_folder;

	public:

		RemoteFolderFixture()
			: m_folder(
			    CRemoteFolder::Create(
					sandbox_pidl().get(),
					boost::bind(
						&RemoteFolderFixture::consumer_factory, this, _1)))
		{}

		com_ptr<IShellFolder> folder() const
		{
			return m_folder;
		}

		comet::com_ptr<ISftpConsumer> consumer_factory(HWND)
		{
			return Consumer();
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

BOOST_FIXTURE_TEST_SUITE(remote_folder_tests, RemoteFolderFixture)

/**
 * When a remote directory is empty, the remote folder's enumerator must
 * be empty.
 */
BOOST_AUTO_TEST_CASE( enum_empty )
{
	SHCONTF flags = 
		SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;

	com_ptr<IEnumIDList> listing;
	HRESULT hr = folder()->EnumObjects(NULL, flags, listing.out());
	BOOST_REQUIRE_OK(hr);

	ULONG fetched = 1;
	PITEMID_CHILD pidl;
	BOOST_CHECK_EQUAL(listing->Next(1, &pidl, &fetched), S_FALSE);
	BOOST_CHECK_EQUAL(fetched, 0U);
}

/**
 * Requesting everything should return folder and dotted files as well.
 */
BOOST_AUTO_TEST_CASE( enum_everything )
{
	wpath file1 = NewFileInSandbox();
	wpath file2 = NewFileInSandbox();
	wpath folder1 = Sandbox() / L"folder1";
	create_directory(folder1);
	wpath folder2 = Sandbox() / L"folder2";
	create_directory(folder2);

	SHCONTF flags = 
		SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;

	com_ptr<IEnumIDList> listing;
	HRESULT hr = folder()->EnumObjects(NULL, flags, listing.out());
	BOOST_REQUIRE_OK(hr);

	test_enum(listing, flags);
}

BOOST_AUTO_TEST_SUITE_END()
