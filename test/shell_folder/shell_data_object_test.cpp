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

#include "test/common_boost/fixtures.hpp"
#include "test/common_boost/helpers.hpp"

#include <comet/interface.h>  // uuidof
#include <comet/ptr.h>  // com_ptr

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION
#include <boost/iterator/indirect_iterator.hpp>  // indirect_iterator
#include <boost/spirit/home/phoenix.hpp>  // bind and _1 that supports &_1

#include <vector>
#include <string>
#include <stdexcept>  // range_error, invalid_argument
#include <algorithm>  // transform

using swish::shell_folder::data_object::ShellDataObject;
using swish::shell_folder::data_object::StorageMedium;
using swish::exception::com_exception;
using test::common_boost::ComFixture;
using test::common_boost::SandboxFixture;
using comet::uuidof;
using comet::comtype;
using comet::com_ptr;
using boost::filesystem::wpath;
using boost::filesystem::ofstream;
using boost::test_tools::predicate_result;
using boost::shared_ptr;
using boost::make_indirect_iterator;
using namespace boost::phoenix::arg_names;
using std::vector;
using std::wstring;
using std::invalid_argument;

template<> struct comtype<IShellFolder>
{
	static const IID& uuid() throw() { return IID_IShellFolder; }
	typedef ::IUnknown base;
};

template<> struct comtype<IDataObject>
{
	static const IID& uuid() throw() { return IID_IDataObject; }
	typedef ::IUnknown base;
};

template<> struct comtype<IDropTarget>
{
	static const IID& uuid() throw() { return IID_IDropTarget; }
	typedef ::IUnknown base;
};

template<> struct comtype<IEnumIDList>
{
	static const IID& uuid() throw() { return IID_IEnumIDList; }
	typedef ::IUnknown base;
};

namespace { // private

	/**
	 * Return an IDataObject representing several files in the same folder.
	 *
	 * The files are passed as a half-open range of fully-qualified paths to
	 * each file.
	 *
	 * @templateparam It  An iterator type whose items are convertible to wpath
	 *                    by the wpath constructor.
	 */
	template<typename It>
	com_ptr<IDataObject> data_object_for_files(It begin, It end)
	{
		vector<shared_ptr<ITEMIDLIST_ABSOLUTE> > absolute_pidls;
		transform(begin, end, back_inserter(absolute_pidls), pidl_from_path);

		return ui_object_of_items<IDataObject>(
			absolute_pidls.begin(), absolute_pidls.end());
	}

	/**
	 * Return an IDataObject representing a file on the local filesystem.
	 */
	com_ptr<IDataObject> data_object_for_file(const wpath& file)
	{
		return data_object_for_files(&file, &file+1);
	}

	/**
	 * Wrapper to change the calling convention of ILFindLastID to cdecl.
	 */
	PUITEMID_CHILD ILFindLastID(PCUIDLIST_RELATIVE pidl)
	{
		return ::ILFindLastID(pidl);
	}

	/**
	 * Return the associated object of several items.
	 *
	 * This is a convenience function that binds to the items' parent and then
	 * asks the parent for the associated object.  The items are passed as a 
	 * half-open range of absolute PIDLs.
	 *
	 * @warning
	 * In order for this to work all items MUST HAVE THE SAME PARENT (i.e. they
	 * must all be in the same folder).
	 */
	template<typename T, typename It>
	com_ptr<T> ui_object_of_items(It begin, It end)
	{
		//
		// All the items we're passed have to have the same parent folder so
		// we just bind to the parent of the *first* item in the collection.
		//

		if (begin == end)
			BOOST_THROW_EXCEPTION(invalid_argument("Empty range given"));

		com_ptr<IShellFolder> parent;
		HRESULT hr = ::SHBindToParent(
			&**begin, uuidof(parent.in()), // &* strips smart pointer, if any
			reinterpret_cast<void**>(parent.out()), NULL);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		vector<const ITEMID_CHILD *> child_pidls;
		transform( // using address of dereference (&*) to strip smart pointers
			make_indirect_iterator(begin), make_indirect_iterator(end),
			back_inserter(child_pidls), bind(ILFindLastID, &_1));

		com_ptr<T> ui_object;
		hr = parent->GetUIObjectOf(
			NULL, child_pidls.size(),
			(child_pidls.empty()) ? NULL : &child_pidls[0],
			uuidof<T>(), NULL, reinterpret_cast<void**>(ui_object.out()));
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		return ui_object;
	}

	/**
	 * Return the associated object of an item.
	 *
	 * This is a convenience function that binds to the item's parent
	 * and then asks the parent for the associated object.  The type of
	 * associated object is determined by the template parameter.
	 *
	 * @templateparam T  Type of associated object to return.
	 */
	template<typename T>
	com_ptr<T> ui_object_of_item(PCIDLIST_ABSOLUTE pidl)
	{
		return ui_object_of_items<T>(&pidl, &pidl + 1);
	}

	/**
	 * Bind to the handler object of an item.
	 *
	 * This handler object is usually an IShellFolder implementation but may be
	 * an IStream as well as other handler types.  The type of handler is
	 * determined by the template parameter.
	 *
	 * @templateparam T  Type of handler to return.
	 */
	template<typename T>
	com_ptr<T> bind_to_hander_object(PCIDLIST_ABSOLUTE pidl)
	{
		com_ptr<IShellFolder> desktop = desktop_folder();
		com_ptr<T> handler;

		HRESULT hr = desktop->BindToObject(
			pidl, NULL, uuidof<T>(), reinterpret_cast<void**>(handler.out()));
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));
		if (!handler)
			BOOST_THROW_EXCEPTION(com_exception(E_FAIL));

		return handler;
	}

	/**
	 * Return an IDataObject representing all the files in a directory.
	 */
	com_ptr<IDataObject> data_object_for_directory(const wpath& directory)
	{
		if (!is_directory(directory))
			BOOST_THROW_EXCEPTION(
				invalid_argument("The path must be to a directory."));

		return data_object_for_files(
			boost::filesystem::wdirectory_iterator(directory),
			boost::filesystem::wdirectory_iterator());
	}

	/**
	 * Return the FORPARSING name of the given PIDL.
	 *
	 * For filesystem items this will be the absolute path.
	 */
	wstring parsing_name_from_pidl(PIDLIST_ABSOLUTE pidl)
	{
		com_ptr<IShellFolder> folder;
		PCUITEMID_CHILD child_pidl;
		HRESULT hr = ::SHBindToParent(
			pidl, uuidof(folder.in()), reinterpret_cast<void**>(folder.out()), 
			&child_pidl);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		STRRET str;
		hr = folder->GetDisplayNameOf(child_pidl, SHGDN_FORPARSING, &str);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		vector<wchar_t> buffer(MAX_PATH);
		hr = ::StrRetToBufW(&str, child_pidl, &buffer[0], buffer.size());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));
		buffer[buffer.size()-1] = L'\0';

		return wstring(&buffer[0]);
	}

	/**
	 * Return the filesystem path represented by the given PIDL.
	 *
	 * @warning
	 * The PIDL must be a PIDL to a filesystem item.  If it isn't this
	 * function is likely but not guaranteed to throw an exception
	 * when it converts the parsing name to a path.  If the parsing
	 * name looks sufficiently path-like, however, it may silently
	 * succeed and return a bogus path.
	 */
	wpath path_from_pidl(PIDLIST_ABSOLUTE pidl)
	{
		return parsing_name_from_pidl(pidl);
	}

	/**
	 * Return an absolute PIDL to the item in the filesystem at the given 
	 * path.
	 */
	shared_ptr<ITEMIDLIST_ABSOLUTE> pidl_from_path(
		const wpath& filesystem_path)
	{
		PIDLIST_ABSOLUTE pidl;
		HRESULT hr = ::SHILCreateFromPath(
			filesystem_path.file_string().c_str(), &pidl, NULL);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		return shared_ptr<ITEMIDLIST_ABSOLUTE>(pidl, ::ILFree);
	}

	/**
	 * Return the desktop folder IShellFolder handler.
	 */
	com_ptr<IShellFolder> desktop_folder()
	{
		com_ptr<IShellFolder> folder;
		HRESULT hr = ::SHGetDesktopFolder(folder.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));
		return folder;
	}

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
		com_ptr<IShellFolder> zip_folder = bind_to_hander_object<IShellFolder>(
			zip_pidl.get());

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

#pragma region Shell utility function tests
BOOST_FIXTURE_TEST_SUITE(shell_utility_tests, DataObjectFixture)

/**
 * Convert a PIDL to a path.  The path should match the source from which the
 * PIDL was created.
 */
BOOST_AUTO_TEST_CASE( convert_pidl_to_path )
{
	wpath source = NewFileInSandbox();
	shared_ptr<ITEMIDLIST_ABSOLUTE> pidl(
		::ILCreateFromPathW(source.file_string().c_str()), ::ILFree);

	wpath path_from_conversion = path_from_pidl(pidl.get());

	BOOST_REQUIRE(equivalent(path_from_conversion, source));
}

/**
 * Make a PIDL from a path.  We should be able to convert the PIDL back to
 * a path that refers to the same item as the original path.
 */
BOOST_AUTO_TEST_CASE( convert_path_to_pidl )
{
	wpath source = NewFileInSandbox();

	shared_ptr<ITEMIDLIST_ABSOLUTE> pidl = pidl_from_path(source);

	vector<wchar_t> buffer(MAX_PATH);
	BOOST_REQUIRE(::SHGetPathFromIDListW(pidl.get(), &buffer[0]));
	BOOST_REQUIRE(equivalent(wstring(&buffer[0]), source));
}

/**
 * Ask the shell for a DataObject 'on' a given file.  This means that the 
 * shell should create a DataObject holding a PIDL list format 
 * (CFSTR_SHELLIDLIST) with two items in it: 
 * - an absolute PIDL to the given file's parent folder
 * - the file's single-item (child) PIDL relative to the parent folder
 */
BOOST_AUTO_TEST_CASE( single_item_dataobject )
{
	wpath source = NewFileInSandbox();
	
	ShellDataObject data_object(data_object_for_file(source).get());

	BOOST_REQUIRE_EQUAL(data_object.pidl_count(), 1U);

	BOOST_REQUIRE(pidl_path_equivalence(data_object.GetParentFolder(), Sandbox()));
	BOOST_REQUIRE(pidl_path_equivalence(data_object.GetFile(0), source));
}


/**
 * Ask the shell for a DataObject 'on' two items in the same folder.
 * This means that the shell should create a DataObject holding a PIDL list 
 * format (CFSTR_SHELLIDLIST) with three items in it: 
 * - an absolute PIDL to the given files' parent folder
 * - the first file's single-item (child) PIDL relative to the parent folder
 * - the second file's single-item (child) PIDL relative to the parent folder
 */
BOOST_AUTO_TEST_CASE( multi_item_dataobject )
{
	vector<wpath> sources;
	sources.push_back(NewFileInSandbox());
	sources.push_back(NewFileInSandbox());
	
	ShellDataObject data_object(
		data_object_for_files(sources.begin(), sources.end()).get());

	BOOST_REQUIRE_EQUAL(data_object.pidl_count(), 2U);

	BOOST_REQUIRE(
		pidl_path_equivalence(data_object.GetParentFolder(), Sandbox()));
	BOOST_REQUIRE(pidl_path_equivalence(data_object.GetFile(0), sources[0]));
	BOOST_REQUIRE(pidl_path_equivalence(data_object.GetFile(1), sources[1]));
}

/**
 * Ask for an associated object of a given file.  In this case we ask for a 
 * DataObject because then we can subject it to the same tests as the
 * data_object_for_file test above.
 */
BOOST_AUTO_TEST_CASE( single_item_ui_object )
{
	wpath source = NewFileInSandbox();
	
	ShellDataObject data_object(
		ui_object_of_item<IDataObject>(pidl_from_path(source).get()).get());

	BOOST_REQUIRE_EQUAL(data_object.pidl_count(), 1U);

	BOOST_REQUIRE(
		pidl_path_equivalence(data_object.GetParentFolder(), Sandbox()));
	BOOST_REQUIRE(pidl_path_equivalence(data_object.GetFile(0), source));
}

/**
 * Ask for an associated object of two files in the same folder.  In this case we ask for a 
 * DataObject because then we can subject it to the same tests as the
 * data_object_for_files test above.
 */
BOOST_AUTO_TEST_CASE( multi_item_ui_object )
{
	vector<wpath> sources;
	sources.push_back(NewFileInSandbox());
	sources.push_back(NewFileInSandbox());

	vector<shared_ptr<ITEMIDLIST_ABSOLUTE> > managed_source_pidls;
	transform(
		sources.begin(), sources.end(), back_inserter(managed_source_pidls), 
		pidl_from_path);
	
	vector<const ITEMIDLIST_ABSOLUTE*> source_pidls;
	transform(
		managed_source_pidls.begin(), managed_source_pidls.end(), 
		back_inserter(source_pidls), 
		std::mem_fun_ref(&shared_ptr<ITEMIDLIST_ABSOLUTE>::get));
	
	ShellDataObject data_object(
		ui_object_of_items<IDataObject>(
			source_pidls.begin(), source_pidls.end()).get());

	BOOST_REQUIRE_EQUAL(data_object.pidl_count(), 2U);

	BOOST_REQUIRE(
		pidl_path_equivalence(data_object.GetParentFolder(), Sandbox()));
	BOOST_REQUIRE(pidl_path_equivalence(data_object.GetFile(0), sources[0]));
	BOOST_REQUIRE(pidl_path_equivalence(data_object.GetFile(1), sources[1]));
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

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
	ShellDataObject data_object(data_object_for_file(file).get());

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
	ShellDataObject data_object(data_object_for_file(file).get());

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
	ShellDataObject data_object(data_object_for_file(file).get());

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
	ShellDataObject data_object(data_object_for_directory(Sandbox()).get());

	BOOST_REQUIRE_EQUAL(data_object.pidl_count(), 3U);

	CAbsolutePidl pidl1 = data_object.GetFile(0);
	CAbsolutePidl pidl2 = data_object.GetFile(1);
	CAbsolutePidl pidl3 = data_object.GetFile(2);

	BOOST_REQUIRE(pidl_path_equivalence(pidl1, file1));
	BOOST_REQUIRE(pidl_path_equivalence(pidl2, file2));
	BOOST_REQUIRE(pidl_path_equivalence(pidl3, file3));

	BOOST_REQUIRE_THROW(data_object.GetFile(4), std::range_error)
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion
