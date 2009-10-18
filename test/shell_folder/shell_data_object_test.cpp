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

#include "swish/shell_folder/data_object/ShellDataObject.hpp"  // Test subject
#include "swish/exception.hpp"  // com_exception
#include "swish/shell_folder/shell.hpp"  // shell helper function

#include "test/common_boost/fixtures.hpp"
#include "test/common_boost/helpers.hpp"

#include <comet/interface.h>  // uuidof
#include <comet/ptr.h>  // com_ptr

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <vector>
#include <string>

using swish::shell_folder::data_object::ShellDataObject;  // test subject
using swish::shell_folder::data_object::PidlFormat;  // test subject
using swish::shell_folder::data_object::StorageMedium;  // test subject
using swish::shell_folder::bind_to_handler_object;
using swish::shell_folder::ui_object_of_item;
using swish::shell_folder::ui_object_of_items;
using swish::shell_folder::pidl_from_path;
using swish::shell_folder::data_object_for_files;
using swish::shell_folder::data_object_for_file;
using swish::shell_folder::data_object_for_directory;
using swish::exception::com_exception;
using test::common_boost::ComFixture;
using test::common_boost::SandboxFixture;
using comet::comtype;
using comet::com_ptr;
using boost::filesystem::wpath;
using boost::filesystem::ofstream;
using boost::test_tools::predicate_result;
using boost::shared_ptr;
using std::vector;
using std::wstring;

template<> struct comtype<IDropTarget>
{
	static const IID& uuid() throw() { return IID_IDropTarget; }
	typedef ::IUnknown base;
};

namespace { // private

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

	/**
	 * Return a DataObject with the contents of a zip file.
	 */
	com_ptr<IDataObject> data_object_for_zipfile(const wpath& zip_file)
	{
		shared_ptr<ITEMIDLIST_ABSOLUTE> zip_pidl = pidl_from_path(zip_file);
		com_ptr<IShellFolder> zip_folder = 
			bind_to_handler_object<IShellFolder>(zip_pidl.get());

		com_ptr<IEnumIDList> enum_items;
		HRESULT hr = zip_folder->EnumObjects(
			NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, enum_items.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		enum_items->Reset();

		vector<shared_ptr<ITEMIDLIST_ABSOLUTE> > pidls;
		while (hr == S_OK)
		{
			PITEMID_CHILD pidl;
			hr = enum_items->Next(1, &pidl, NULL);
			if (hr == S_OK)
			{
				shared_ptr<ITEMID_CHILD> child_pidl(pidl, ::ILFree);

				pidls.push_back(
					shared_ptr<ITEMIDLIST_ABSOLUTE>(
						::ILCombine(zip_pidl.get(), child_pidl.get()),
						::ILFree));
			}
		}

		return ui_object_of_items<IDataObject>(pidls.begin(), pidls.end());
	}

	/**
	 * Create an empty zip archive in the PK format.
	 *
	 * An 'empty' archive contains 22 bytes of data: the letters 'PK' follow by
	 * the bytes 5, 6 and the 18 bytes of 0.
	 */
	void create_empty_zip_archive(const wpath& name)
	{
		char empty_zip_data[] = {'P', 'K', '\x05', '\x06', 
			'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
			'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
		ofstream zip_stream(name, std::ios::binary);
		zip_stream.write(empty_zip_data, sizeof(empty_zip_data));
		zip_stream.close();
	}

	class DataObjectFixture : public ComFixture, public SandboxFixture
	{
	public:
		/**
		 * Create a zip archive containing two files that we can use as a source
		 * of 'virtual' namespace items.
		 *
		 * Virtual namespace items are not real files on disk and instead are
		 * simulated by an IShellFolder implementation.  This is how Swish 
		 * itself presents its 'files' to Explorer.  The ZIP-file browser in 
		 * Windows 2000 and later does the same thing to give access to the 
		 * files inside a .zip.  We're going to use one of these to test our 
		 * shell data object wrapper with virtual items.
		 */
		wpath create_test_zip_file()
		{
			wpath file = NewFileInSandbox();
			wpath zip_file = file;
			zip_file.replace_extension(L"zip");
			rename(file, zip_file);
			create_empty_zip_archive(zip_file);

			//
			// We're sneakily going to use the zip folder's own DropTarget to
			// add files to the empty archive.
			//

			shared_ptr<ITEMIDLIST_ABSOLUTE> zip_file_pidl = pidl_from_path(
				zip_file);

			vector<wpath> zip_contents;
			zip_contents.push_back(NewFileInSandbox());
			zip_contents.push_back(NewFileInSandbox());

			com_ptr<IDataObject> data_object = data_object_for_files(
				zip_contents.begin(), zip_contents.end());

			com_ptr<IDropTarget> drop_target = ui_object_of_item<IDropTarget>(
				zip_file_pidl.get());

			POINTL pt = {0, 0};
			DWORD dwEffect = DROPEFFECT_COPY;
			HRESULT hr = drop_target->DragEnter(
				data_object.get(), MK_LBUTTON, pt, &dwEffect);
			if (hr != S_OK)
			{
				drop_target->DragLeave();
				BOOST_THROW_EXCEPTION(com_exception(hr));
			}
			if (!dwEffect)
				BOOST_THROW_EXCEPTION(com_exception(E_UNEXPECTED));

			hr = drop_target->Drop(
				data_object.get(), MK_LBUTTON, pt, &dwEffect);
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(com_exception(hr));

			// HACK:  Shell extensions are meant to call SHCreateThread with
			// the CTF_PROCESS_REF flag so that their 'host' process (usally
			// Explorer but in this case us) is forced to stay alive.  Zipfldr
			// doesn't bother so we have no idea when its safe to continue.
			// Hence this sleep here.
			// An alternative is to enumerate the zip folder until the files
			// appear
			Sleep(700);

			return zip_file;
		}
	};
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
 * Detecting the CF_HDROP format for a filesystem item.
 *
 * The shell data object should always have this format for items that are
 * backed by a real filesystem (i.e. not virtual).  This is a test of whether
 * we can recognise that or not.
 */
BOOST_AUTO_TEST_CASE( cf_hdrop_format )
{
	wpath file = NewFileInSandbox();
	ShellDataObject data_object(data_object_for_file(file).get());

	BOOST_REQUIRE(data_object.has_hdrop_format());
}

/**
 * Detecting the CF_HDROP format for virtual items.
 *
 * A data object should not have this format for virtual items as they have no
 * filesystem path.  This is a test of whether we can recognise that or not.
 */
BOOST_AUTO_TEST_CASE( cf_hdrop_format_virtual )
{
	wpath zip_file = create_test_zip_file();
	ShellDataObject data_object(data_object_for_zipfile(zip_file).get());

	BOOST_REQUIRE(!data_object.has_hdrop_format());
}

/**
 * Detecting the CFSTR_SHELLIDLIST format for a filesystem item.
 *
 * The shell data object should always have this format.  This is a test
 * of whether we can recognise that or not.
 *
 * @todo  Unset the format and test a negative result.
 */
BOOST_AUTO_TEST_CASE( cfstr_shellidlist_format )
{
	wpath file = NewFileInSandbox();
	ShellDataObject data_object(data_object_for_file(file).get());

	BOOST_REQUIRE(data_object.has_pidl_format());
}

/**
 * Detecting the CFSTR_SHELLIDLIST format for virtual items.
 *
 * The shell data object should always have this format.  This is a test
 * of whether we can recognise that or not.
 *
 * @todo  Unset the format and test a negative result.
 */
BOOST_AUTO_TEST_CASE( cfstr_shellidlist_format_virtual )
{
	wpath zip_file = create_test_zip_file();
	ShellDataObject data_object(data_object_for_zipfile(zip_file).get());

	BOOST_REQUIRE(data_object.has_pidl_format());
}

/**
 * Detecting the CFSTR_FILEDESCRIPTOR format for a filesystem item.
 *
 * This format is not expected for regular filesystem (non-virtual) items.  Here
 * we are checking that we recognise this absence correctly.
 */
BOOST_AUTO_TEST_CASE( cf_file_group_descriptor_format )
{
	wpath file = NewFileInSandbox();
	ShellDataObject data_object(data_object_for_file(file).get());

	BOOST_REQUIRE(!data_object.has_file_group_descriptor_format());
}

/**
 * Detecting the CFSTR_FILEDESCRIPTOR format for virtual items.
 *
 * This format is expected for data objects holding virtual items.  This is a 
 * test of whether we can recognise that or not.
 */
BOOST_AUTO_TEST_CASE( cf_file_group_descriptor_format_virtual )
{
	wpath zip_file = create_test_zip_file();
	ShellDataObject data_object(data_object_for_zipfile(zip_file).get());

	BOOST_REQUIRE(data_object.has_file_group_descriptor_format());
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

#pragma region PidlFormat tests
BOOST_FIXTURE_TEST_SUITE(pidl_format_tests, DataObjectFixture)

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
	PidlFormat format(data_object_for_file(file));

	BOOST_REQUIRE_EQUAL(format.pidl_count(), 1U);

	CAbsolutePidl pidl = format.file(0);

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
	PidlFormat format(data_object_for_file(file));

	BOOST_REQUIRE_EQUAL(format.pidl_count(), 1U);

	CAbsolutePidl folder_pidl = format.parent_folder();

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
	PidlFormat format(data_object_for_file(file));

	BOOST_REQUIRE_EQUAL(format.pidl_count(), 1U);
	BOOST_REQUIRE_THROW(format.file(1), std::range_error)
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
	PidlFormat format(data_object_for_directory(Sandbox()));

	BOOST_REQUIRE_EQUAL(format.pidl_count(), 3U);

	CAbsolutePidl pidl1 = format.file(0);
	CAbsolutePidl pidl2 = format.file(1);
	CAbsolutePidl pidl3 = format.file(2);

	BOOST_REQUIRE(pidl_path_equivalence(pidl1, file1));
	BOOST_REQUIRE(pidl_path_equivalence(pidl2, file2));
	BOOST_REQUIRE(pidl_path_equivalence(pidl3, file3));

	BOOST_REQUIRE_THROW(format.file(4), std::range_error)
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion
