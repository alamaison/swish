/**
    @file

    Testing Provider via the external COM interfaces alone.

    @if license

    Copyright (C) 2008, 2009, 2010, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "test/common_boost/helpers.hpp"
#include "test/common_boost/MockConsumer.hpp"
#include "test/common_boost/remote_test_config.hpp"

#include "swish/provider/Provider.hpp"

#include <comet/bstr.h> // bstr_t
#include <comet/datetime.h> // datetime_t
#include <comet/error.h> // com_error_from_interface
#include <comet/ptr.h> // com_ptr

#include <boost/filesystem/path.hpp> // wpath
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/make_shared.hpp>
#include <boost/range/empty.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <exception>
#include <string> // wstring
#include <vector>

using swish::provider::CProvider;
using swish::provider::Listing;
using swish::provider::SmartListing;
using swish::provider::directory_listing;
using swish::provider::sftp_provider;

using comet::bstr_t;
using comet::com_error;
using comet::com_error_from_interface;
using comet::com_ptr;
using comet::datetime_t;

using boost::filesystem::wpath;
using boost::make_shared;
using boost::shared_ptr;
using boost::test_tools::predicate_result;

using std::exception;
using std::vector;
using std::wstring;

namespace test {

namespace {
    
    shared_ptr<sftp_provider> create_provider()
    {
        remote_test_config config;
        wstring user = config.GetUser();
        wstring host = config.GetHost();

        return make_shared<CProvider>(user, host, config.GetPort());
    }

    /**
     * Check that the given provider responds sensibly to a request given
     * a particular consumer.
     *
     * This may mean that the provider wasn't authenticated but survived
     * an attempt to make it do something (presumably) by authenticating.
     */
    predicate_result alive(
        shared_ptr<sftp_provider> provider, com_ptr<ISftpConsumer> consumer)
    {
        try
        {
            provider->listing(consumer, L"/");

            predicate_result res(true);
            res.message() << "Provider seems to be alive";
            return res;
        }
        catch(const exception& e)
        {
            predicate_result res(false);
            res.message() << "Provider seems to be dead: " << e.what();
            return res;
        }
    }
    
    /**
     * Check that the given provider responds sensibly to a request.
     */
    predicate_result alive(shared_ptr<sftp_provider> provider)
    {
        return alive(provider, new MockConsumer());
    }
}

BOOST_AUTO_TEST_SUITE( provider_legacy_auth_tests )

BOOST_AUTO_TEST_CASE( SimplePasswordAuthentication )
{
    // Choose mock behaviours to force only simple password authentication
    com_ptr<MockConsumer> consumer = new MockConsumer();
    consumer->set_password_behaviour(MockConsumer::CustomPassword);
    consumer->set_keyboard_interactive_behaviour(MockConsumer::FailResponse);

    remote_test_config config;
    consumer->set_password(config.GetPassword());

    shared_ptr<sftp_provider> provider = create_provider();

    BOOST_CHECK(alive(provider, consumer));
}

BOOST_AUTO_TEST_CASE( WrongPassword )
{
    com_ptr<MockConsumer> consumer = new MockConsumer();

    consumer->set_password_behaviour(MockConsumer::WrongPassword);

    shared_ptr<sftp_provider> provider = create_provider();

    BOOST_CHECK(!alive(provider, consumer));
}

BOOST_AUTO_TEST_CASE( KeyboardInteractiveAuthentication )
{
    // Choose mock behaviours to force only kbd-interactive authentication
    com_ptr<MockConsumer> consumer = new MockConsumer();
    consumer->set_password_behaviour(MockConsumer::FailPassword);
    consumer->set_keyboard_interactive_behaviour(MockConsumer::CustomResponse);

    remote_test_config config;
    consumer->set_password(config.GetPassword());

    shared_ptr<sftp_provider> provider = create_provider();

    // This may fail if the server (which we can't control) doesn't allow
    // ki-auth
    BOOST_CHECK(alive(provider, consumer));
}

/**
 * Test to see that we can connect successfully after an aborted attempt.
 */
BOOST_AUTO_TEST_CASE( ReconnectAfterAbort )
{
    // Choose mock behaviours to simulate a user cancelling authentication
    com_ptr<MockConsumer> consumer = new MockConsumer();
    consumer->set_password_behaviour(MockConsumer::AbortPassword);
    consumer->set_keyboard_interactive_behaviour(
        MockConsumer::AbortResponse);

    shared_ptr<sftp_provider> provider = create_provider();

    // Try to fetch a listing enumerator - it should fail
    BOOST_CHECK(!alive(provider, consumer));

    // Change mock behaviours so that authentication succeeds
    consumer->set_password_max_attempts(2);
    consumer->set_password_behaviour(MockConsumer::CustomPassword);
    consumer->set_keyboard_interactive_behaviour(MockConsumer::CustomResponse);

    remote_test_config config;
    consumer->set_password(config.GetPassword());

    BOOST_CHECK(alive(provider, consumer));
}

BOOST_AUTO_TEST_SUITE_END();

namespace {

predicate_result file_exists_in_listing(
    const wstring& filename, const directory_listing& listing)
{
    if (boost::empty(listing))
    {
        predicate_result res(false);
        res.message() << "Enumerator is empty";
        return res;
    }
    else
    {
        BOOST_FOREACH(const SmartListing& entry, listing)
        {
            if (filename == bstr_t(entry.get().bstrFilename))
            {
                predicate_result res(true);
                res.message() << "File found in enumerator: " << filename;
                return res;
            }
        }

        predicate_result res(false);
        res.message() << "File not in enumerator: " << filename;
        return res;
    }
}

/**
 * Performs a typical test setup.
 *
 * The mock consumer is set to authenticate using the correct password
 * and throw an exception on all other callbacks to it.  This setup is 
 * suitable for any tests that simply to test functionality rather than
 * testing the process of authentication itself.  If the test expects
 * the provider to callback to the consumer, these behaviours can be added
 * after this method is called.
 */
class ProviderLegacyFixture
{
public:
    ProviderLegacyFixture()
        :
    provider(create_provider()), consumer(new MockConsumer()),
        m_pConsumer(consumer.get()),
        home_directory(wpath(L"/home") / config.GetUser())
    {
        consumer->set_password_behaviour(MockConsumer::CustomPassword);
        consumer->set_password(config.GetPassword());

        // Create test area (not used by all tests)
        if (!path_exists(_TestArea()))
        {
            provider->create_new_directory(
                m_pConsumer, bstr_t(_TestArea()).in());
        }
    }

    ~ProviderLegacyFixture()
    {
        if (path_exists(_TestArea()))
        {
            try
            {
                provider->delete_directory(
                    m_pConsumer, bstr_t(_TestArea()).in());
            }
            catch (const std::exception&) { /* ignore */ }
        }
    }

protected:
    shared_ptr<sftp_provider> provider;
    com_ptr<MockConsumer> consumer;
    ISftpConsumer *m_pConsumer;
    remote_test_config config;
    wpath home_directory;

    predicate_result path_exists(const wpath& file_path)
    {
        directory_listing listing;
        try
        {
            listing = provider->listing(m_pConsumer, file_path.parent_path());
        }
        catch (const exception&)
        {
            predicate_result res(false);
            res.message()
                << "Parent path does not exist: " << file_path.parent_path();
            return res;
        }

        return file_exists_in_listing(file_path.filename(), listing);
    }

    /**
     * Returns path as subpath of home directory in a BSTR.
     */
    wpath _HomeDir(const wstring& path) const
    {
        return home_directory / path;
    }
    
    /**
     * Returns path as subpath of the test area directory in a BSTR.
     */
    wstring _TestArea(const wpath& path = wpath()) const
    {
        return (_HomeDir(L"testArea") / path).string();
    }
};

}

BOOST_FIXTURE_TEST_SUITE(provider_legacy_tests, ProviderLegacyFixture)

BOOST_AUTO_TEST_CASE( GetListing )
{
    directory_listing listing = provider->listing(consumer, L"/tmp");

    // Check format of listing is sensible
    BOOST_FOREACH(const SmartListing& entry, listing)
    {
        const Listing& lt = entry.get();
        wstring filename = lt.bstrFilename;
        wstring owner = lt.bstrOwner;
        wstring group = lt.bstrGroup;

        BOOST_CHECK(!filename.empty());
        BOOST_CHECK_NE(filename, L".");
        BOOST_CHECK_NE(filename, L"..");

        BOOST_CHECK(!owner.empty());
        BOOST_CHECK(!group.empty());

        BOOST_CHECK( lt.dateModified );
        datetime_t modified(lt.dateModified);
        BOOST_CHECK(modified.valid());
        BOOST_CHECK_LE(modified.year(), datetime_t::now().year());

        // TODO: test numerical permissions using old swish C 
        //       permissions functions here
        //BOOST_CHECK(
        //    strPermissions[0] == _T('d') ||
        //    strPermissions[0] == _T('b') ||
        //    strPermissions[0] == _T('c') ||
        //    strPermissions[0] == _T('l') ||
        //    strPermissions[0] == _T('p') ||
        //    strPermissions[0] == _T('s') ||
        //    strPermissions[0] == _T('-'));
    }
}

BOOST_AUTO_TEST_CASE( GetListingRepeatedly )
{
    // Fetch 5 listing enumerators
    vector<directory_listing> enums(5);

    BOOST_FOREACH(directory_listing& listing, enums)
    {
        listing = provider->listing(m_pConsumer, L"/tmp");
    }
}

BOOST_AUTO_TEST_CASE( GetListingIndependence )
{
    // Put some files in the test area
    wstring directory(_TestArea());
    bstr_t one(_TestArea(L"GetListingIndependence1"));
    bstr_t two(_TestArea(L"GetListingIndependence2"));
    bstr_t three(_TestArea(L"GetListingIndependence3"));
    provider->create_new_file(m_pConsumer, one.in());
    provider->create_new_file(m_pConsumer, two.in());
    provider->create_new_file(m_pConsumer, three.in());

    // Fetch first listing enumerator
    directory_listing listing_before =
        provider->listing(m_pConsumer, directory);

    // Delete one of the files
    provider->delete_file(m_pConsumer, two.in());

    // Fetch second listing enumerator
    directory_listing listing_after = provider->listing(m_pConsumer, directory);

    // The first listing should still show the file. The second should not.
    BOOST_CHECK(
        file_exists_in_listing(L"GetListingIndependence1", listing_before));
    BOOST_CHECK(
        file_exists_in_listing(L"GetListingIndependence2", listing_before));
    BOOST_CHECK(
        file_exists_in_listing(L"GetListingIndependence3", listing_before));
    BOOST_CHECK(
        file_exists_in_listing(L"GetListingIndependence1", listing_after));
    BOOST_CHECK(
        !file_exists_in_listing(L"GetListingIndependence2", listing_after));
    BOOST_CHECK(
        file_exists_in_listing(L"GetListingIndependence3", listing_after));

    // Cleanup
    provider->delete_file(m_pConsumer, one.in());
    provider->delete_file(m_pConsumer, three.in());
}

BOOST_AUTO_TEST_CASE( Rename )
{
    bstr_t subject(_TestArea(L"Rename"));
    bstr_t target(_TestArea(L"Rename_Passed"));

    // Create our test subject and check existence
    provider->create_new_file(m_pConsumer, subject.in());
    BOOST_CHECK(path_exists(subject.in()));
    BOOST_CHECK(!path_exists(target.in()));

    // Test renaming file
    BOOST_CHECK(
        provider->rename(m_pConsumer, subject.in(), target.in())
        == VARIANT_FALSE);

    // Test renaming file back
    BOOST_CHECK(
        provider->rename(m_pConsumer, target.in(), subject.in())
        == VARIANT_FALSE);

    // Check that the target does not still exist
    BOOST_CHECK(!path_exists(target.in()));

    // Cleanup
    provider->delete_file(m_pConsumer, subject.in());
}

BOOST_AUTO_TEST_CASE( RenameWithObstruction )
{
    // Choose mock behaviour
    consumer->set_confirm_overwrite_behaviour(MockConsumer::AllowOverwrite);

    bstr_t subject(_TestArea(L"RenameWithObstruction"));
    bstr_t target(
        _TestArea(L"RenameWithObstruction_Obstruction"));
    bstr_t swish_temp(
        _TestArea(L"RenameWithObstruction_Obstruction.swish_rename_temp"));

    // Create our test subjects and check existence
    provider->create_new_file(m_pConsumer, subject.in());
    provider->create_new_file(m_pConsumer, target.in());
    BOOST_CHECK(path_exists(subject.in()));
    BOOST_CHECK(path_exists(target.in()));

    // Check that the non-atomic overwrite temp does not already exists
    BOOST_CHECK(!path_exists(swish_temp.in()));

    // Test renaming file
    BOOST_CHECK(
        provider->rename(m_pConsumer, subject.in(), target.in())
        == VARIANT_TRUE);

    // Check that the old file no longer exists but the target does
    BOOST_CHECK(!path_exists(subject.in()));
    BOOST_CHECK(path_exists(target.in()));

    // Check that the non-atomic overwrite temp has been removed
    BOOST_CHECK(!path_exists(swish_temp.in()));

    // Cleanup
    provider->delete_file(m_pConsumer, target.in());
    BOOST_CHECK(!path_exists(subject.in()));
    BOOST_CHECK(!path_exists(target.in()));
}

/**
 * We are not checking that the file exists beforehand so the libssh2 has
 * no way to know which directory we intended.  If this passes then it is
 * defaulting to home directory.
 */
BOOST_AUTO_TEST_CASE( RenameNoDirectory )
{
    bstr_t subject(L"RenameNoDirectory");
    bstr_t target(L"RenameNoDirectory_Passed");
    provider->create_new_file(m_pConsumer, subject.in());

    // Test renaming file
    BOOST_CHECK(
        provider->rename(m_pConsumer, subject.in(), target.in())
        == VARIANT_FALSE);

    // Test renaming file back
    BOOST_CHECK(
        provider->rename(m_pConsumer, target.in(), subject.in())
        == VARIANT_FALSE);

    // Cleanup
    provider->delete_file(m_pConsumer, subject.in());
}

BOOST_AUTO_TEST_CASE( RenameFolder )
{
    bstr_t subject(_TestArea(L"RenameFolder"));
    bstr_t target(_TestArea(L"RenameFolder_Passed"));

    // Create our test subject and check existence
    provider->create_new_directory(m_pConsumer, subject.in());
    BOOST_CHECK(path_exists(subject.in()));
    BOOST_CHECK(!path_exists(target.in()));

    // Test renaming file
    BOOST_CHECK(
        provider->rename(m_pConsumer, subject.in(), target.in())
        == VARIANT_FALSE);

    // Test renaming file back
    BOOST_CHECK(
        provider->rename(m_pConsumer, target.in(), subject.in())
        == VARIANT_FALSE);

    // Check that the target does not still exist
    BOOST_CHECK(!path_exists(target.in()));

    // Cleanup
    provider->delete_directory(m_pConsumer, subject.in());
    BOOST_CHECK(!path_exists(subject.in()));
}

BOOST_AUTO_TEST_CASE( RenameFolderWithObstruction )
{
    // Choose mock behaviour
    consumer->set_confirm_overwrite_behaviour(
        MockConsumer::AllowOverwrite);

    bstr_t subject(_TestArea(L"RenameFolderWithObstruction"));
    bstr_t target(
        _TestArea(L"RenameFolderWithObstruction_Obstruction"));
    bstr_t targetContents(
        _TestArea(L"RenameFolderWithObstruction_Obstruction/file"));
    bstr_t swish_temp(_TestArea(
        L"RenameFolderWithObstruction_Obstruction.swish_rename_temp"));

    // Create our test subjects and check existence
    provider->create_new_directory(m_pConsumer, subject.in());
    provider->create_new_directory(m_pConsumer, target.in());
    BOOST_CHECK(path_exists(subject.in()));
    BOOST_CHECK(path_exists(target.in()));

    // Add a file in the obstructing directory to make it harder to delete
    provider->create_new_file(m_pConsumer, targetContents.in());
    BOOST_CHECK(path_exists(targetContents.in()));

    // Check that the non-atomic overwrite temp does not already exists
    BOOST_CHECK(!path_exists(swish_temp.in()));

    // Test renaming file
    BOOST_CHECK(
        provider->rename(m_pConsumer, subject.in(), target.in())
        == VARIANT_TRUE);

    // Check that the old file no longer exists but the target does
    BOOST_CHECK(!path_exists(subject.in()));
    BOOST_CHECK(path_exists(target.in()));

    // Check that the non-atomic overwrite temp has been removed
    BOOST_CHECK(!path_exists(swish_temp.in()));

    // Cleanup
    provider->delete_directory(m_pConsumer, target.in());
    BOOST_CHECK(!path_exists(subject.in()));
    BOOST_CHECK(!path_exists(target.in()));
}

namespace {

    bool is_abort(const com_error& error)
    {
        return error.hr() == E_ABORT;
    }
}

BOOST_AUTO_TEST_CASE( RenameWithRefusedConfirmation )
{
    // Choose mock behaviour
    consumer->set_confirm_overwrite_behaviour(
        MockConsumer::PreventOverwrite);

    bstr_t subject(_TestArea(L"RenameWithRefusedConfirmation"));
    bstr_t target(
        _TestArea(L"RenameWithRefusedConfirmation_Obstruction"));

    // Create our test subjects and check existence
    provider->create_new_file(m_pConsumer, subject.in());
    provider->create_new_file(m_pConsumer, target.in());
    BOOST_CHECK(path_exists(subject.in()));
    BOOST_CHECK(path_exists(target.in()));

    // Test renaming file
    BOOST_CHECK_EXCEPTION(
        provider->rename(m_pConsumer, subject.in(), target.in()),
        com_error, is_abort);

    // Check that both files still exist
    BOOST_CHECK(path_exists(subject.in()));
    BOOST_CHECK(path_exists(target.in()));

    // Cleanup
    provider->delete_file(m_pConsumer, subject.in());
    provider->delete_file(m_pConsumer, target.in());
    BOOST_CHECK(!path_exists(subject.in()));
    BOOST_CHECK(!path_exists(target.in()));
}

BOOST_AUTO_TEST_CASE( RenameFolderWithRefusedConfirmation )
{
    // Choose mock behaviour
    consumer->set_confirm_overwrite_behaviour(
        MockConsumer::PreventOverwrite);

    bstr_t subject(
        _TestArea(L"RenameFolderWithRefusedConfirmation"));
    bstr_t target(
        _TestArea(L"RenameFolderWithRefusedConfirmation_Obstruction"));

    // Create our test subjects and check existence
    provider->create_new_directory(m_pConsumer, subject.in());
    provider->create_new_directory(m_pConsumer, target.in());
    BOOST_CHECK(path_exists(subject.in()));
    BOOST_CHECK(path_exists(target.in()));

    // Test renaming directory
    BOOST_CHECK_EXCEPTION(
        provider->rename(m_pConsumer, subject.in(), target.in()),
        com_error, is_abort);

    // Check that both directories still exist
    BOOST_CHECK(path_exists(subject.in()));
    BOOST_CHECK(path_exists(target.in()));

    // Cleanup
    provider->delete_directory(m_pConsumer, subject.in());
    provider->delete_directory(m_pConsumer, target.in());
    BOOST_CHECK(!path_exists(subject.in()));
    BOOST_CHECK(!path_exists(target.in()));
}

BOOST_AUTO_TEST_CASE( RenameInNonHomeFolder )
{
    bstr_t subject(L"/tmp/swishRenameInNonHomeFolder");
    bstr_t target(L"/tmp/swishRenameInNonHomeFolder_Passed");

    // Create our test subjects and check existence
    provider->create_new_file(m_pConsumer, subject.in());
    BOOST_CHECK(path_exists(subject.in()));
    BOOST_CHECK(!path_exists(target.in()));

    // Test renaming file
    BOOST_CHECK(
        provider->rename(m_pConsumer, subject.in(), target.in())
        == VARIANT_FALSE);

    // Test renaming file back
    BOOST_CHECK(
        provider->rename(m_pConsumer, target.in(), subject.in())
        == VARIANT_FALSE);

    // Check that the target does not still exist
    BOOST_CHECK(!path_exists(target.in()));

    // Cleanup
    provider->delete_file(m_pConsumer, subject.in());
    BOOST_CHECK(!path_exists(subject.in()));
    BOOST_CHECK(!path_exists(target.in()));
}

BOOST_AUTO_TEST_CASE( RenameInNonHomeSubfolder )
{
    bstr_t folder(L"/tmp/swishSubfolder");
    bstr_t subject(
        L"/tmp/swishSubfolder/RenameInNonHomeSubfolder");
    bstr_t target(
        L"/tmp/swishSubfolder/RenameInNonHomeSubfolder_Passed");

    // Create our test subjects and check existence
    provider->create_new_directory(m_pConsumer, folder.in());
    provider->create_new_file(m_pConsumer, subject.in());
    BOOST_CHECK(path_exists(subject.in()));
    BOOST_CHECK(!path_exists(target.in()));

    // Test renaming file
    BOOST_CHECK(
        provider->rename(m_pConsumer, subject.in(), target.in())
        == VARIANT_FALSE);

    // Test renaming file back
    BOOST_CHECK(
        provider->rename(m_pConsumer, target.in(), subject.in())
        == VARIANT_FALSE);

    // Check that the target does not still exist
    BOOST_CHECK(!path_exists(target.in()));
    
    // Cleanup
    provider->delete_directory(m_pConsumer, folder.in());
    BOOST_CHECK(!path_exists(folder.in()));
}

BOOST_AUTO_TEST_CASE( CreateAndDelete )
{
    bstr_t subject(_TestArea(L"CreateAndDelete"));

    // Check that the file does not already exist
    BOOST_CHECK(!path_exists(subject.in()));

    // Test creating file
    provider->create_new_file(m_pConsumer, subject.in());

    // Test deleting file
    provider->delete_file(m_pConsumer, subject.in());

    // Check that the file does not still exist
    BOOST_CHECK(!path_exists(subject.in()));
}

BOOST_AUTO_TEST_CASE( CreateAndDeleteEmptyDirectory )
{
    bstr_t subject(_TestArea(L"CreateAndDeleteEmptyDirectory"));

    // Check that the directory does not already exist
    BOOST_CHECK(!path_exists(subject.in()));

    // Test creating directory
    provider->create_new_directory(m_pConsumer, subject.in());

    // Test deleting directory
    provider->delete_directory(m_pConsumer, subject.in());
    
    // Check that the directory does not still exist
    BOOST_CHECK(!path_exists(subject.in()));
}

BOOST_AUTO_TEST_CASE( CreateAndDeleteDirectoryRecursive )
{
    bstr_t directory(_TestArea(L"CreateAndDeleteDirectory"));
    bstr_t file(_TestArea(L"CreateAndDeleteDirectory/Recursive"));

    // Check that subjects do not already exist
    BOOST_CHECK(!path_exists(directory.in()));
    BOOST_CHECK(!path_exists(file.in()));

    // Create directory
    provider->create_new_directory(m_pConsumer, directory.in());

    // Add file to directory
    provider->create_new_file(m_pConsumer, file.in());

    // Test deleting directory
    provider->delete_directory(m_pConsumer, directory.in());

    // Check that the directory does not still exist
    BOOST_CHECK(!path_exists(directory.in()));
}

} // namespace test

BOOST_AUTO_TEST_SUITE_END()
