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
#include <boost/test/unit_test.hpp>

#include <string> // wstring
#include <vector>

using comet::bstr_t;
using comet::com_error_from_interface;
using comet::com_ptr;
using comet::datetime_t;

using boost::filesystem::wpath;
using boost::test_tools::predicate_result;

using std::vector;
using std::wstring;

namespace test {

#define CHECK_PATH_EXISTS(path) _CheckPathExists(path)
#define CHECK_PATH_NOT_EXISTS(path) _CheckPathNotExists(path)

class ProviderLegacyFixture
{
public:
    ProviderLegacyFixture() :
      m_pProvider(new swish::provider::CProvider()), m_pConsumer(NULL),
        home_directory(wpath(L"/home") / config.GetUser())
    {
        m_pProvider->AddRef();

        // Create mock SftpConsumer for use in Initialize()
        m_pCoConsumer = _CreateMockSftpConsumer();
        m_pConsumer = m_pCoConsumer.get();
    }

    ~ProviderLegacyFixture()
    {
        if (_FileExists(_TestArea()))
        {
            try
            {
                m_pProvider->delete_directory(
                    m_pConsumer, bstr_t(_TestArea()).in());
            }
            catch (const std::exception&) { /* ignore */ }
        }

        if (m_pProvider) // Possible for test to fail before initialised
        {
            ULONG cRefs = m_pProvider->Release();
            cRefs;
            //BOOST_CHECK_EQUAL( (ULONG)0, cRefs );
        }
        m_pProvider = NULL;
    }

protected:
    com_ptr<MockConsumer> m_pCoConsumer;
    ISftpConsumer *m_pConsumer;
    ISftpProvider *m_pProvider;
    remote_test_config config;
    wpath home_directory;

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
    void _StandardSetup()
    {
        bstr_t user = config.GetUser();
        bstr_t host = config.GetHost();

        // Standard mock behaviours
        m_pCoConsumer->set_password_behaviour(MockConsumer::CustomPassword);
        m_pCoConsumer->set_password(config.GetPassword());

        HRESULT hr = m_pProvider->Initialize(
            user.in(), host.in(), config.GetPort());
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(m_pProvider, hr));

        // Create test area (not used by all tests)
        if (!_FileExists(_TestArea()))
        {
            HRESULT hr = m_pProvider->CreateNewDirectory(
                m_pConsumer, bstr_t(_TestArea()).in());
            if (FAILED(hr))
                BOOST_THROW_EXCEPTION(com_error_from_interface(m_pProvider, hr));
        }
    }

    /**
     * Tests that the format of the enumeration of listings is correct.
     *
     * @param pEnum The Listing enumerator to be tested.
     */
    void _TestListingFormat(com_ptr<IEnumListing> enumerator) const
    {
        // Check format of listing is sensible
        HRESULT hr = enumerator->Reset();
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(enumerator, hr));

        Listing lt;
        hr = enumerator->Next(1, &lt, NULL);
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(enumerator, hr));
        while (hr == S_OK)
        {
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

            hr = enumerator->Next(1, &lt, NULL);
        }
        BOOST_CHECK(hr == S_FALSE);
    }

    bool _FileExistsInListing(
        const bstr_t& filename, com_ptr<IEnumListing> enumerator)
    {
        HRESULT hr;

        // Search for file
        Listing lt;
        hr = enumerator->Reset();
        BOOST_REQUIRE_OK(hr);
        hr = enumerator->Next(1, &lt, NULL);
        BOOST_CHECK(SUCCEEDED(hr));
        while (hr == S_OK)
        {
            if (filename == lt.bstrFilename)
                return true;

            hr = enumerator->Next(1, &lt, NULL);
        }

        return false;
    }

    bool _FileExists(const wpath& file_path)
    {
        // Fetch listing enumerator
        com_ptr<IEnumListing> enumerator;
        HRESULT hr = m_pProvider->GetListing(
            m_pConsumer, bstr_t(file_path.parent_path().string()).in(),
            enumerator.out());
        if (FAILED(hr))
            return false;

        return _FileExistsInListing(file_path.filename(), enumerator);
    }

    void _CheckPathExists(const wpath& path)
    {
        BOOST_CHECK_MESSAGE(
            _FileExists(path), L"Expected file not found: " + path.string());
    }

    void _CheckPathNotExists(const wpath& path)
    {
        BOOST_CHECK_MESSAGE(
            !_FileExists(path), L"File unexpectedly found: " + path.string());
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

    static com_ptr<MockConsumer> _CreateMockSftpConsumer()
    {
        return new MockConsumer();
    }

    /**
     * Check that the given provider responds sensibly to a request.
     */
    predicate_result alive(com_ptr<ISftpProvider> provider)
    {
        com_ptr<IEnumListing> listing;
        HRESULT hr = provider->GetListing(
            m_pConsumer, bstr_t(L"/home").in(), listing.out());
        if (FAILED(hr))
        {
            predicate_result res(false);
            res.message()
                << "Provider seems to be dead: "
                << com_error_from_interface(provider, hr).what();
            return res;
        }
        else
        {
            return true;
        }
    }
};

BOOST_FIXTURE_TEST_SUITE(provider_legacy_tests, ProviderLegacyFixture)

BOOST_AUTO_TEST_CASE( Initialize )
{
    bstr_t user = config.GetUser();
    bstr_t host = config.GetHost();

    // Test with invalid port values
#pragma warning (push)
#pragma warning (disable: 4245) // unsigned signed mismatch
    BOOST_CHECK_EQUAL(
        E_INVALIDARG,
        m_pProvider->Initialize(user.in(), host.in(), -1)
    );
    BOOST_CHECK_EQUAL(
        E_INVALIDARG,
        m_pProvider->Initialize(user.in(), host.in(), 65536)
    );
#pragma warning (pop)

    // Run real test
    BOOST_REQUIRE_OK(
        m_pProvider->Initialize(user.in(), host.in(), config.GetPort()));
}

BOOST_AUTO_TEST_CASE( GetListing )
{
    _StandardSetup();

    // Fetch listing enumerator
    com_ptr<IEnumListing> enumerator;
    HRESULT hr = m_pProvider->GetListing(
        m_pConsumer, bstr_t(L"/tmp").in(), enumerator.out());
    if (FAILED(hr))
        BOOST_THROW_EXCEPTION(com_error_from_interface(m_pProvider, hr));

    // Check format of listing is sensible
    _TestListingFormat(enumerator);
}

BOOST_AUTO_TEST_CASE( GetListing_WrongPassword )
{
    bstr_t user = config.GetUser();
    bstr_t host = config.GetHost();

    // Choose mock behaviours
    m_pCoConsumer->set_password_behaviour(MockConsumer::WrongPassword);

    BOOST_REQUIRE_OK(
        m_pProvider->Initialize(user.in(), host.in(), config.GetPort()));

    // Fetch listing enumerator
    com_ptr<IEnumListing> enumerator;
    HRESULT hr = m_pProvider->GetListing(
        m_pConsumer, bstr_t(L"/tmp").in(), enumerator.out());
    BOOST_CHECK(FAILED(hr));
}

BOOST_AUTO_TEST_CASE( GetListingRepeatedly )
{
    _StandardSetup();

    // Fetch 5 listing enumerators
    vector< com_ptr<IEnumListing> > enums(5);

    BOOST_FOREACH(com_ptr<IEnumListing> listing, enums)
    {
        HRESULT hr = m_pProvider->GetListing(
            m_pConsumer, bstr_t(L"/tmp").in(), listing.out());
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(m_pProvider, hr));
    }
}

BOOST_AUTO_TEST_CASE( GetListingIndependence )
{
    _StandardSetup();

    HRESULT hr;

    // Put some files in the test area
    bstr_t directory(_TestArea());
    bstr_t one(_TestArea(L"GetListingIndependence1"));
    bstr_t two(_TestArea(L"GetListingIndependence2"));
    bstr_t three(_TestArea(L"GetListingIndependence3"));
    BOOST_REQUIRE_OK(m_pProvider->CreateNewFile(m_pConsumer, one.in()));
    BOOST_REQUIRE_OK(m_pProvider->CreateNewFile(m_pConsumer, two.in()));
    BOOST_REQUIRE_OK(m_pProvider->CreateNewFile(m_pConsumer, three.in()));

    // Fetch first listing enumerator
    com_ptr<IEnumListing> enum_before;
    hr = m_pProvider->GetListing(
        m_pConsumer, directory.in(), enum_before.out());
    if (FAILED(hr))
        BOOST_THROW_EXCEPTION(com_error_from_interface(m_pProvider, hr));

    // Delete one of the files
    m_pProvider->delete_file(m_pConsumer, two.in());

    // Fetch second listing enumerator
    com_ptr<IEnumListing> enum_after;
    hr = m_pProvider->GetListing(
        m_pConsumer, directory.in(), enum_after.out());
    if (FAILED(hr))
        BOOST_THROW_EXCEPTION(com_error_from_interface(m_pProvider, hr));

    // The first listing should still show the file. The second should not.
    BOOST_CHECK(_FileExistsInListing(
        L"GetListingIndependence1", enum_before));
    BOOST_CHECK(_FileExistsInListing(
        L"GetListingIndependence2", enum_before));
    BOOST_CHECK(_FileExistsInListing(
        L"GetListingIndependence3", enum_before));
    BOOST_CHECK(_FileExistsInListing(
        L"GetListingIndependence1", enum_after));
    BOOST_CHECK(!_FileExistsInListing(
        L"GetListingIndependence2", enum_after));
    BOOST_CHECK(_FileExistsInListing(
        L"GetListingIndependence3", enum_after));

    // Cleanup
    m_pProvider->delete_file(m_pConsumer, one.in());
    m_pProvider->delete_file(m_pConsumer, three.in());
}

BOOST_AUTO_TEST_CASE( Rename )
{
    _StandardSetup();

    HRESULT hr;

    bstr_t subject(_TestArea(L"Rename"));
    bstr_t target(_TestArea(L"Rename_Passed"));

    // Create our test subject and check existence
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewFile(m_pConsumer, subject.in()));
    CHECK_PATH_EXISTS(subject.in());
    CHECK_PATH_NOT_EXISTS(target.in());

    // Test renaming file
    VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
    hr = m_pProvider->Rename(
        m_pConsumer, subject.in(), target.in(), &fWasOverwritten);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK(fWasOverwritten == VARIANT_FALSE);

    // Test renaming file back
    hr = m_pProvider->Rename(
        m_pConsumer, target.in(), subject.in(), &fWasOverwritten);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK(fWasOverwritten == VARIANT_FALSE);

    // Check that the target does not still exist
    CHECK_PATH_NOT_EXISTS(target.in());

    // Cleanup
    m_pProvider->delete_file(m_pConsumer, subject.in());
}

BOOST_AUTO_TEST_CASE( RenameWithObstruction )
{
    _StandardSetup();

    HRESULT hr;

    // Choose mock behaviour
    m_pCoConsumer->set_confirm_overwrite_behaviour(MockConsumer::AllowOverwrite);

    bstr_t subject(_TestArea(L"RenameWithObstruction"));
    bstr_t target(
        _TestArea(L"RenameWithObstruction_Obstruction"));
    bstr_t swish_temp(
        _TestArea(L"RenameWithObstruction_Obstruction.swish_rename_temp"));

    // Create our test subjects and check existence
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewFile(m_pConsumer, subject.in()));
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewFile(m_pConsumer, target.in()));
    CHECK_PATH_EXISTS(subject.in());
    CHECK_PATH_EXISTS(target.in());

    // Check that the non-atomic overwrite temp does not already exists
    CHECK_PATH_NOT_EXISTS(swish_temp.in());

    // Test renaming file
    VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
    hr = m_pProvider->Rename(
        m_pConsumer, subject.in(), target.in(), &fWasOverwritten);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK(fWasOverwritten == VARIANT_TRUE);

    // Check that the old file no longer exists but the target does
    CHECK_PATH_NOT_EXISTS(subject.in());
    CHECK_PATH_EXISTS(target.in());

    // Check that the non-atomic overwrite temp has been removed
    CHECK_PATH_NOT_EXISTS(swish_temp.in());

    // Cleanup
    m_pProvider->delete_file(m_pConsumer, target.in());
    CHECK_PATH_NOT_EXISTS(subject.in());
    CHECK_PATH_NOT_EXISTS(target.in());
}

/**
 * We are not checking that the file exists beforehand so the libssh2 has
 * no way to know which directory we intended.  If this passes then it is
 * defaulting to home directory.
 */
BOOST_AUTO_TEST_CASE( RenameNoDirectory )
{
    _StandardSetup();

    HRESULT hr;

    bstr_t subject(L"RenameNoDirectory");
    bstr_t target(L"RenameNoDirectory_Passed");
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewFile(m_pConsumer, subject.in()));

    // Test renaming file
    VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
    hr = m_pProvider->Rename(
        m_pConsumer, subject.in(), target.in(), &fWasOverwritten);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK(fWasOverwritten == VARIANT_FALSE);

    // Test renaming file back
    hr = m_pProvider->Rename(
        m_pConsumer, target.in(), subject.in(), &fWasOverwritten);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK(fWasOverwritten == VARIANT_FALSE);

    // Cleanup
    m_pProvider->delete_file(m_pConsumer, subject.in());
}

BOOST_AUTO_TEST_CASE( RenameFolder )
{
    _StandardSetup();

    HRESULT hr;

    bstr_t subject(_TestArea(L"RenameFolder"));
    bstr_t target(_TestArea(L"RenameFolder_Passed"));

    // Create our test subject and check existence
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewDirectory(m_pConsumer, subject.in()));
    CHECK_PATH_EXISTS(subject.in());
    CHECK_PATH_NOT_EXISTS(target.in());

    // Test renaming directory
    VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
    hr = m_pProvider->Rename(
        m_pConsumer, subject.in(), target.in(), &fWasOverwritten);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK(fWasOverwritten == VARIANT_FALSE);

    // Test renaming directory back
    hr = m_pProvider->Rename(
        m_pConsumer, target.in(), subject.in(), &fWasOverwritten);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK(fWasOverwritten == VARIANT_FALSE);

    // Check that the target does not still exist
    CHECK_PATH_NOT_EXISTS(target.in());

    // Cleanup
    m_pProvider->delete_directory(m_pConsumer, subject.in());
    CHECK_PATH_NOT_EXISTS(subject.in());
}

BOOST_AUTO_TEST_CASE( RenameFolderWithObstruction )
{
    _StandardSetup();

    HRESULT hr;

    // Choose mock behaviour
    m_pCoConsumer->set_confirm_overwrite_behaviour(
        MockConsumer::AllowOverwrite);

    bstr_t subject(_TestArea(L"RenameFolderWithObstruction"));
    bstr_t target(
        _TestArea(L"RenameFolderWithObstruction_Obstruction"));
    bstr_t targetContents(
        _TestArea(L"RenameFolderWithObstruction_Obstruction/file"));
    bstr_t swish_temp(_TestArea(
        L"RenameFolderWithObstruction_Obstruction.swish_rename_temp"));

    // Create our test subjects and check existence
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewDirectory(m_pConsumer, subject.in()));
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewDirectory(m_pConsumer, target.in()));
    CHECK_PATH_EXISTS(subject.in());
    CHECK_PATH_EXISTS(target.in());

    // Add a file in the obstructing directory to make it harder to delete
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewFile(m_pConsumer, targetContents.in()));
    CHECK_PATH_EXISTS(targetContents.in());

    // Check that the non-atomic overwrite temp does not already exists
    CHECK_PATH_NOT_EXISTS(swish_temp.in());

    // Test renaming file
    VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
    hr = m_pProvider->Rename(
        m_pConsumer, subject.in(), target.in(), &fWasOverwritten);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK(fWasOverwritten == VARIANT_TRUE);

    // Check that the old file no longer exists but the target does
    CHECK_PATH_NOT_EXISTS(subject.in());
    CHECK_PATH_EXISTS(target.in());

    // Check that the non-atomic overwrite temp has been removed
    CHECK_PATH_NOT_EXISTS(swish_temp.in());

    // Cleanup
    m_pProvider->delete_directory(m_pConsumer, target.in());
    CHECK_PATH_NOT_EXISTS(subject.in());
    CHECK_PATH_NOT_EXISTS(target.in());
}

BOOST_AUTO_TEST_CASE( RenameWithRefusedConfirmation )
{
    _StandardSetup();

    HRESULT hr;

    // Choose mock behaviour
    m_pCoConsumer->set_confirm_overwrite_behaviour(
        MockConsumer::PreventOverwrite);

    bstr_t subject(_TestArea(L"RenameWithRefusedConfirmation"));
    bstr_t target(
        _TestArea(L"RenameWithRefusedConfirmation_Obstruction"));

    // Create our test subjects and check existence
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewFile(m_pConsumer, subject.in()));
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewFile(m_pConsumer, target.in()));
    CHECK_PATH_EXISTS(subject.in());
    CHECK_PATH_EXISTS(target.in());

    // Test renaming file
    VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
    hr = m_pProvider->Rename(
        m_pConsumer, subject.in(), target.in(), &fWasOverwritten);
    BOOST_CHECK(FAILED(hr));
    BOOST_CHECK(fWasOverwritten == VARIANT_FALSE);

    // Check that both files still exist
    CHECK_PATH_EXISTS(subject.in());
    CHECK_PATH_EXISTS(target.in());

    // Cleanup
    m_pProvider->delete_file(m_pConsumer, subject.in());
    m_pProvider->delete_file(m_pConsumer, target.in());
    CHECK_PATH_NOT_EXISTS(subject.in());
    CHECK_PATH_NOT_EXISTS(target.in());
}

BOOST_AUTO_TEST_CASE( RenameFolderWithRefusedConfirmation )
{
    _StandardSetup();

    HRESULT hr;

    // Choose mock behaviour
    m_pCoConsumer->set_confirm_overwrite_behaviour(
        MockConsumer::PreventOverwrite);

    bstr_t subject(
        _TestArea(L"RenameFolderWithRefusedConfirmation"));
    bstr_t target(
        _TestArea(L"RenameFolderWithRefusedConfirmation_Obstruction"));

    // Create our test subjects and check existence
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewDirectory(m_pConsumer, subject.in()));
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewDirectory(m_pConsumer, target.in()));
    CHECK_PATH_EXISTS(subject.in());
    CHECK_PATH_EXISTS(target.in());

    // Test renaming directory
    VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
    hr = m_pProvider->Rename(
        m_pConsumer, subject.in(), target.in(), &fWasOverwritten);
    BOOST_CHECK(FAILED(hr));
    BOOST_CHECK(fWasOverwritten == VARIANT_FALSE);

    // Check that both directories still exist
    CHECK_PATH_EXISTS(subject.in());
    CHECK_PATH_EXISTS(target.in());

    // Cleanup
    m_pProvider->delete_directory(m_pConsumer, subject.in());
    m_pProvider->delete_directory(m_pConsumer, target.in());
    CHECK_PATH_NOT_EXISTS(subject.in());
    CHECK_PATH_NOT_EXISTS(target.in());
}

BOOST_AUTO_TEST_CASE( RenameInNonHomeFolder )
{
    _StandardSetup();

    HRESULT hr;

    bstr_t subject(L"/tmp/swishRenameInNonHomeFolder");
    bstr_t target(L"/tmp/swishRenameInNonHomeFolder_Passed");

    // Create our test subjects and check existence
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewFile(m_pConsumer, subject.in()));
    CHECK_PATH_EXISTS(subject.in());
    CHECK_PATH_NOT_EXISTS(target.in());

    // Test renaming file
    VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
    hr = m_pProvider->Rename(
        m_pConsumer, subject.in(), target.in(), &fWasOverwritten);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK(fWasOverwritten == VARIANT_FALSE);

    // Test renaming file back
    hr = m_pProvider->Rename(
        m_pConsumer, target.in(), subject.in(), &fWasOverwritten);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK(fWasOverwritten == VARIANT_FALSE);

    // Check that the target does not still exist
    CHECK_PATH_NOT_EXISTS(target.in());

    // Cleanup
    m_pProvider->delete_file(m_pConsumer, subject.in());
    CHECK_PATH_NOT_EXISTS(subject.in());
    CHECK_PATH_NOT_EXISTS(target.in());
}

BOOST_AUTO_TEST_CASE( RenameInNonHomeSubfolder )
{
    _StandardSetup();

    HRESULT hr;

    bstr_t folder(L"/tmp/swishSubfolder");
    bstr_t subject(
        L"/tmp/swishSubfolder/RenameInNonHomeSubfolder");
    bstr_t target(
        L"/tmp/swishSubfolder/RenameInNonHomeSubfolder_Passed");

    // Create our test subjects and check existence
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewDirectory(m_pConsumer, folder.in()));
    BOOST_REQUIRE_OK(
        m_pProvider->CreateNewFile(m_pConsumer, subject.in()));
    CHECK_PATH_EXISTS(subject.in());
    CHECK_PATH_NOT_EXISTS(target.in());

    // Test renaming file
    VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
    hr = m_pProvider->Rename(
        m_pConsumer, subject.in(), target.in(), &fWasOverwritten);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK(fWasOverwritten == VARIANT_FALSE);

    // Test renaming file back
    hr = m_pProvider->Rename(
        m_pConsumer, target.in(), subject.in(), &fWasOverwritten);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK(fWasOverwritten == VARIANT_FALSE);

    // Check that the target does not still exist
    CHECK_PATH_NOT_EXISTS(target.in());
    
    // Cleanup
    m_pProvider->delete_directory(m_pConsumer, folder.in());
    CHECK_PATH_NOT_EXISTS(folder.in());
}

BOOST_AUTO_TEST_CASE( CreateAndDelete )
{
    _StandardSetup();

    HRESULT hr;

    bstr_t subject(_TestArea(L"CreateAndDelete"));

    // Check that the file does not already exist
    CHECK_PATH_NOT_EXISTS(subject.in());

    // Test creating file
    hr = m_pProvider->CreateNewFile(m_pConsumer, subject.in());
    BOOST_REQUIRE_OK(hr);

    // Test deleting file
    m_pProvider->delete_file(m_pConsumer, subject.in());

    // Check that the file does not still exist
    CHECK_PATH_NOT_EXISTS(subject.in());
}

BOOST_AUTO_TEST_CASE( CreateAndDeleteEmptyDirectory )
{
    _StandardSetup();

    HRESULT hr;

    bstr_t subject(_TestArea(L"CreateAndDeleteEmptyDirectory"));

    // Check that the directory does not already exist
    CHECK_PATH_NOT_EXISTS(subject.in());

    // Test creating directory
    hr = m_pProvider->CreateNewDirectory(m_pConsumer, subject.in());
    BOOST_REQUIRE_OK(hr);

    // Test deleting directory
    m_pProvider->delete_directory(m_pConsumer, subject.in());
    
    // Check that the directory does not still exist
    CHECK_PATH_NOT_EXISTS(subject.in());
}

BOOST_AUTO_TEST_CASE( CreateAndDeleteDirectoryRecursive )
{
    _StandardSetup();

    HRESULT hr;

    bstr_t directory(_TestArea(L"CreateAndDeleteDirectory"));
    bstr_t file(_TestArea(L"CreateAndDeleteDirectory/Recursive"));

    // Check that subjects do not already exist
    CHECK_PATH_NOT_EXISTS(directory.in());
    CHECK_PATH_NOT_EXISTS(file.in());

    // Create directory
    hr = m_pProvider->CreateNewDirectory(m_pConsumer, directory.in());
    BOOST_REQUIRE_OK(hr);

    // Add file to directory
    hr = m_pProvider->CreateNewFile(m_pConsumer, file.in());
    BOOST_REQUIRE_OK(hr);

    // Test deleting directory
    m_pProvider->delete_directory(m_pConsumer, directory.in());

    // Check that the directory does not still exist
    CHECK_PATH_NOT_EXISTS(directory.in());
}

BOOST_AUTO_TEST_CASE( KeyboardInteractiveAuthentication )
{
    bstr_t user = config.GetUser();
    bstr_t host = config.GetHost();

    // Choose mock behaviours to force only kbd-interactive authentication
    m_pCoConsumer->set_password_behaviour(MockConsumer::FailPassword);
    m_pCoConsumer->set_keyboard_interactive_behaviour(
        MockConsumer::CustomResponse);
    m_pCoConsumer->set_password(config.GetPassword());

    BOOST_REQUIRE_OK(
        m_pProvider->Initialize(user.in(), host.in(), config.GetPort()));

    // This may fail if the server (which we can't control) doesn't allow
    // ki-auth
    BOOST_CHECK(alive(m_pProvider));
}

BOOST_AUTO_TEST_CASE( SimplePasswordAuthentication )
{
    bstr_t user = config.GetUser();
    bstr_t host = config.GetHost();

    // Choose mock behaviours to force only simple password authentication
    m_pCoConsumer->set_password_behaviour(MockConsumer::CustomPassword);
    m_pCoConsumer->set_keyboard_interactive_behaviour(
        MockConsumer::FailResponse);
    m_pCoConsumer->set_password(config.GetPassword());

    BOOST_REQUIRE_OK(
        m_pProvider->Initialize(user.in(), host.in(), config.GetPort()));

    BOOST_CHECK(alive(m_pProvider));
}

/**
 * Test to see that we can connect successfully after an aborted attempt.
 */
BOOST_AUTO_TEST_CASE( ReconnectAfterAbort )
{
    bstr_t user = config.GetUser();
    bstr_t host = config.GetHost();

    BOOST_REQUIRE_OK(
        m_pProvider->Initialize(user.in(), host.in(), config.GetPort()));

    // Choose mock behaviours to simulate a user cancelling authentication
    m_pCoConsumer->set_password_behaviour(MockConsumer::AbortPassword);
    m_pCoConsumer->set_keyboard_interactive_behaviour(
        MockConsumer::AbortResponse);

    // Try to fetch a listing enumerator - it should fail
    com_ptr<IEnumListing> enumerator;
    bstr_t directory("/tmp");
    HRESULT hr = m_pProvider->GetListing(
        m_pConsumer, directory.in(), enumerator.out());
    BOOST_CHECK(FAILED(hr));

    // Choose mock behaviours so that authentication succeeds
    m_pCoConsumer->set_password_max_attempts(2);
    m_pCoConsumer->set_password_behaviour(MockConsumer::CustomPassword);
    m_pCoConsumer->set_keyboard_interactive_behaviour(
        MockConsumer::CustomResponse);
    m_pCoConsumer->set_password(config.GetPassword());

    // Try to fetch a listing again - this time is should succeed
    BOOST_CHECK(!enumerator);
    hr = m_pProvider->GetListing(m_pConsumer, directory.in(), enumerator.out());
    BOOST_REQUIRE_OK(hr);
}

} // namespace test

BOOST_AUTO_TEST_SUITE_END()
