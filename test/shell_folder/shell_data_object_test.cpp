/**
    @file

    Unit tests for the ShellDataObject wrapper.

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

#include "swish/shell_folder/data_object/ShellDataObject.hpp"  // Test subject
#include "swish/exception.hpp"  // com_exception

#include "test/common_boost/fixtures.hpp"
#include "test/common_boost/helpers.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>

#include <vector>
#include <string>
#include <stdexcept>  // range_error

using swish::shell_folder::data_object::ShellDataObject;
using swish::shell_folder::data_object::StorageMedium;
using swish::exception::com_exception;
using test::common_boost::ComFixture;
using test::common_boost::SandboxFixture;
using boost::filesystem::wpath;
using boost::test_tools::predicate_result;
using boost::shared_ptr;
using ATL::CComPtr;
using std::vector;
using std::wstring;

namespace { // private

	/**
	 * Return an IDataObject representing several files in the same folder.
	 *
	 * @templateparam It  An iterator type whose items are convertible to wpath
	 *                    by the wpath constructor.
	 */
	template<typename It>
	CComPtr<IDataObject> data_object_for_files(It begin, It end)
	{
		HRESULT hr;

		// Translate each path into a PIDL and store them in a vector as
		// shared_ptrs.  This will take care of freeing them when needed.
		// Store pointers to the last item in the PIDLs in another vector.  
		// As they are simply pointers into the absolute PIDLs, the don't need
		// seperately freeing so store them as plain pointers.

		vector<shared_ptr<ITEMIDLIST_ABSOLUTE> > absolute_pidls;
		vector<const ITEMID_CHILD *> child_pidls;

		for (It i = begin; i != end; ++i)
		{
			wpath item(*i);
			shared_ptr<ITEMIDLIST_ABSOLUTE> absolute_pidl(
				::ILCreateFromPathW(item.file_string().c_str()), ::ILFree);

			PCUITEMID_CHILD child_pidl = ::ILFindLastID(absolute_pidl.get());

			absolute_pidls.push_back(absolute_pidl);
			child_pidls.push_back(child_pidl);
		}
		
		CComPtr<IShellFolder> spsf;
		hr = ::SHBindToParent(
			(absolute_pidls.empty()) ? NULL : absolute_pidls[0].get(),
			__uuidof(IShellFolder), (void**)&spsf, NULL);
		if (FAILED(hr))
			throw com_exception(hr);

		CComPtr<IDataObject> spdo;
		hr = spsf->GetUIObjectOf(
			NULL, child_pidls.size(), 
			(child_pidls.empty()) ? NULL : &child_pidls[0],
			__uuidof(IDataObject), NULL, (void**)&spdo);
		if (FAILED(hr))
			throw com_exception(hr);

		return spdo;
	}

	/**
	 * Return an IDataObject representing a file on the local filesystem.
	 */
	CComPtr<IDataObject> data_object_for_file(const wpath& file)
	{
		return data_object_for_files(&file, &file+1);
	}

	/**
	 * Return an IDataObject representing all the files in a directory.
	 */
	CComPtr<IDataObject> data_object_for_directory(const wpath& directory)
	{
		if (!is_directory(directory))
			throw std::invalid_argument("The path must give a directory.");

		return data_object_for_files(
			boost::filesystem::wdirectory_iterator(directory),
			boost::filesystem::wdirectory_iterator());
	}

	class DataObjectFixture : public ComFixture, public SandboxFixture
	{
	};

	/**
	 * Check that a PIDL and a filesystem path refer to the same item.
	 */
	predicate_result pidl_path_equivalence(PCIDLIST_ABSOLUTE pidl, wpath path)
	{
		vector<wchar_t> name(MAX_PATH);
		::SHGetPathFromIDListW(pidl, &name[0]);

		if (!equivalent(path, &name[0]))
		{
			predicate_result res(false);
			res.message()
				<< "Different items [" << wstring(&name[0])
				<< " != " << path.file_string() << "]";
			return res;
		}

		return true;
	}

}

#pragma region StorageMedium tests
BOOST_AUTO_TEST_SUITE(storage_medium_tests)

/**
 * Create and destroy an instance of the StorageMedium helper object.
 * Check a few members to see if they are initialsed properly.
 */
BOOST_AUTO_TEST_CASE( storage_medium_lifecycle )
{
	{
		StorageMedium medium;

		BOOST_REQUIRE(medium.empty());
		BOOST_REQUIRE_EQUAL(medium.get().hGlobal, static_cast<HGLOBAL>(NULL));
		BOOST_REQUIRE_EQUAL(medium.get().pstm, static_cast<IStream*>(NULL));
		BOOST_REQUIRE_EQUAL(
			medium.get().pUnkForRelease, static_cast<IUnknown*>(NULL));
	}
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion


#pragma region ShellDataObject tests
BOOST_FIXTURE_TEST_SUITE(shell_data_object_tests, DataObjectFixture)

/**
 * Get a PIDL from a shell data object.
 *
 * Create the DataObject with one item, the test file in the sandbox.  Get
 * the item from the data object as a PIDL and check that it can be resolved 
 * back the to filename from which the data object was created.
 */
BOOST_AUTO_TEST_CASE( cfstr_shellidlist_item )
{
	wpath file = NewFileInSandbox();
	ShellDataObject data_object(data_object_for_file(file));

	BOOST_REQUIRE(data_object.has_pidl_format());
	BOOST_REQUIRE_EQUAL(data_object.pidl_count(), 1U);

	CAbsolutePidl pidl = data_object.GetFile(0);

	BOOST_REQUIRE(pidl_path_equivalence(pidl, file));
}

/**
 * Get a PIDL's parent from a shell data object.
 *
 * Create the DataObject with one item, the test file in the sandbox.  Get
 * the parent folder of this item (the sandbox) from the data object as a 
 * PIDL and check that it can be resolved back the sandbox's path.
 */
BOOST_AUTO_TEST_CASE( cfstr_shellidlist_parent )
{
	wpath file = NewFileInSandbox();
	ShellDataObject data_object(data_object_for_file(file));

	BOOST_REQUIRE(data_object.has_pidl_format());
	BOOST_REQUIRE_EQUAL(data_object.pidl_count(), 1U);

	CAbsolutePidl folder_pidl = data_object.GetParentFolder();

	BOOST_REQUIRE(pidl_path_equivalence(folder_pidl, file.parent_path()));
}

/**
 * Try to get a non-existent PIDL from the data object.
 *
 * Create the DataObject with one item.  Attempt to get the @b second item 
 * from the data object.  As there is no second item this should fail by
 * throwing an range_error exception.
 */
BOOST_AUTO_TEST_CASE( cfstr_shellidlist_item_fail )
{
	wpath file = NewFileInSandbox();
	ShellDataObject data_object(data_object_for_file(file));

	BOOST_REQUIRE_EQUAL(data_object.pidl_count(), 1U);

	BOOST_REQUIRE_THROW(data_object.GetFile(1), std::range_error)
}

/**
 * Get PIDLs from a shell data object with more than one.
 *
 * Create the DataObject with three items, test files in the sandbox.  Get
 * the items from the data object as PIDLs and check that they can be resolved
 * back the to the filenames from which the data object was created.
 */
BOOST_AUTO_TEST_CASE( cfstr_shellidlist_multiple_items )
{
	wpath file1 = NewFileInSandbox();
	wpath file2 = NewFileInSandbox();
	wpath file3 = NewFileInSandbox();
	ShellDataObject data_object(data_object_for_directory(Sandbox()));

	BOOST_REQUIRE(data_object.has_pidl_format());
	BOOST_REQUIRE_EQUAL(data_object.pidl_count(), 3U);

	CAbsolutePidl pidl1 = data_object.GetFile(0);
	CAbsolutePidl pidl2 = data_object.GetFile(1);
	CAbsolutePidl pidl3 = data_object.GetFile(2);

	BOOST_REQUIRE(pidl_path_equivalence(pidl1, file1));
	BOOST_REQUIRE(pidl_path_equivalence(pidl2, file2));
	BOOST_REQUIRE(pidl_path_equivalence(pidl3, file3));
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion
