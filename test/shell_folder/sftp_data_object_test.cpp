/**
    @file

    Unit tests to test the IDataObject interface to remote files.

    @if licence

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

/**
 * @file
 *
 * Unlike the tests in drop_target_test.cpp, these tests do not excercise
 * the CDropTarget component alone.  Nor do they excercise it directly.
 * Instead the simulate the calls the shell itself would make to drag
 * a file making use of the whole Shell Namespace Folder heirarchy.
 */

#include "swish/shell_folder/SftpDataObject.h"  // test subject
#include "swish/shell_folder/data_object/FileGroupDescriptor.hpp"  // accessor
#include "swish/shell_folder/data_object/ShellDataObject.hpp"  // accessor
#include "swish/shell_folder/data_object/StorageMedium.hpp"  // accessor
#include "swish/shell_folder/SftpDirectory.h"  // listing pidls in a folder
#include "swish/shell_folder/shell.hpp"  // shell utility functions
#include "swish/shell_folder/HostPidl.h"  // Host PIDL wrapper
#include "swish/utils.hpp"  // character conversion

#include "test/common_boost/helpers.hpp"  // BOOST_REQUIRE_OK
#include "test/common_boost/ProviderFixture.hpp"  // ProviderFixture

#include <winapi/shell/pidl.hpp>  // PIDL wrapper
#include <winapi/shell/pidl_array.hpp>  // PIDL array wrapper

#include <comet/ptr.h>  // com_ptr

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/system/system_error.hpp>
#pragma warning(push)
#pragma warning(disable:4244) // conversion from uint64_t to uint32_t
#include <boost/date_time/posix_time/conversion.hpp> // from_time_t
#pragma warning(pop)
#include <boost/date_time/posix_time/posix_time_io.hpp> // ptime stringerising
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <string>
#include <vector>

#include <sys/stat.h>  // _S_IREAD

using swish::shell_folder::desktop_folder;
using swish::shell_folder::parsing_name_from_pidl;
using swish::utils::Utf8StringToWideString;
using namespace swish::shell_folder::data_object;

using test::ProviderFixture;

using namespace winapi::shell::pidl;

using comet::com_ptr;

using boost::filesystem::wpath;
using boost::filesystem::ifstream;
using boost::filesystem::ofstream;
using boost::system::system_error;
using boost::system::system_category;
using boost::posix_time::from_time_t;
using boost::test_tools::predicate_result;

using std::string;
using std::wstring;
using std::vector;
using std::istreambuf_iterator;

namespace { // private

	/**
	 * Return the PIDL to the Swish HostFolder in Explorer.
	 */
	apidl_t swish_pidl()
	{
		com_ptr<IShellFolder> desktop = desktop_folder();

		apidl_t pidl;
		HRESULT hr = desktop->ParseDisplayName(
			NULL, NULL, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\"
			L"::{B816A83A-5022-11DC-9153-0090F5284F85}", NULL, 
			reinterpret_cast<PIDLIST_RELATIVE*>(&pidl), NULL);
		BOOST_REQUIRE_OK(hr);

		return pidl;
	}

	class DataObjectFixture : public ProviderFixture
	{
	public:
		/**
		 * Return an absolute PIDL to a remote directory.
		 *
		 * We cheat by returning a PIDL to a HostFolder item with the
		 * shortcut path set to the remote directory.
		 */
		apidl_t directory_pidl(wpath directory)
		{
			CHostItem item(
				Utf8StringToWideString(GetUser()).c_str(),
				Utf8StringToWideString(GetHost()).c_str(),
				directory.string().c_str(), 
				static_cast<USHORT>(GetPort()));

			return swish_pidl() + cpidl_t(item);
		}

		/**
		 * Return an absolute PIDL to the sandbox on the remote end.
		 *
		 * This is, of course, the local sandbox but the PIDL points to
		 * it via Swish rather than via the local filesystem.
		 */
		apidl_t sandbox_pidl()
		{
			return directory_pidl(ToRemotePath(Sandbox()));
		}

		/**
		 * Return pidls for all the items in the sandbox directory.
		 */
		vector<cpidl_t> pidls_in_sandbox()
		{
			CSftpDirectory dir(
				sandbox_pidl().get(), Provider().get(), Consumer().get());
			com_ptr<IEnumIDList> pidl_enum = dir.GetEnum(
				SHCONTF_FOLDERS | SHCONTF_NONFOLDERS);

			vector<cpidl_t> pidls;
			cpidl_t pidl;
			while (pidl_enum->Next(1, pidl.out(), NULL) == S_OK)
			{
				pidls.push_back(pidl);
			}

			return pidls;
		}

		vector<wpath> make_test_files(bool readonly=false)
		{
			vector<wpath> files;
			files.push_back(NewFileInSandbox());
			files.push_back(NewFileInSandbox());

			ofstream s;
			s.open(files.at(0));
			s << "blah";
			s.close();
			s.open(files.at(1));
			s << "more stuff";
			s.close();

			if (readonly)
			{
				if (_wchmod(files.at(0).string().c_str(), _S_IREAD) != 0)
					BOOST_THROW_EXCEPTION(
						system_error(errno, system_category));
				if (_wchmod(files.at(0).string().c_str(), _S_IREAD) != 0)
					BOOST_THROW_EXCEPTION(
						system_error(errno, system_category));
			}

			return files;
		}

		/**
		 * Make a DataObject to all the items in the sandbox, via the SFTP 
		 * connection.
		 */
		com_ptr<IDataObject> data_object_from_sandbox()
		{
			vector<cpidl_t> pidls = pidls_in_sandbox();
			pidl_array<cpidl_t> array(pidls.begin(), pidls.end());
			BOOST_REQUIRE_EQUAL(array.size(), 2U);

			com_ptr<IDataObject> data_object = CSftpDataObject::Create(
				array.size(), array.as_array(), 
				sandbox_pidl().get(), Provider().get(), Consumer().get());
			BOOST_REQUIRE(data_object);
			return data_object;
		}
	};

	/**
	 * Check that a PIDL and a filesystem path refer to the same item.
	 */
	predicate_result pidl_equivalence(
		const apidl_t& pidl1, const apidl_t& pidl2)
	{
		wstring name1 = parsing_name_from_pidl(pidl1.get());
		wstring name2 = parsing_name_from_pidl(pidl2.get());

		if (name1 != name2)
		{
			predicate_result res(false);
			res.message()
				<< "PIDLs resolve to different items [" << name1
				<< " != " << name2 << "]";
			return res;
		}

		return true;
	}

	/**
	 * Check that the contents of a file and a stream are the same
	 */
	predicate_result file_stream_equivalence(
		const wpath& file, const com_ptr<IStream>& stream)
	{
		// Read in from file
		ifstream in_stream(file);
		string file_contents = string(
			istreambuf_iterator<char>(in_stream),
			istreambuf_iterator<char>());

		// Read in from stream
		ULARGE_INTEGER stream_size;
		LARGE_INTEGER zero = {0};
		BOOST_REQUIRE_OK(stream->Seek(zero, STREAM_SEEK_END, &stream_size));
		BOOST_REQUIRE_OK(stream->Seek(zero, STREAM_SEEK_SET, NULL));
		vector<char> buf(stream_size.LowPart);
		string stream_contents;
		if (buf.size())
		{
			ULONG cbRead = 0;
			BOOST_REQUIRE_OK(stream->Read(&buf[0], buf.size(), &cbRead));
			
			stream_contents = string(&buf[0], cbRead);
		}

		if (file_contents != stream_contents)
		{
			predicate_result res(false);
			res.message()
				<< "File and IStream contents do not match [" << file_contents
				<< " != " << stream_contents << "]";
			return res;
		}

		return true;
	}

	const CLIPFORMAT CF_FILEDESCRIPTORW = 
		static_cast<CLIPFORMAT>(::RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW));
	const CLIPFORMAT CF_FILECONTENTS = 
		static_cast<CLIPFORMAT>(::RegisterClipboardFormat(CFSTR_FILECONTENTS));
}

#pragma region SftpDataObject tests
BOOST_FIXTURE_TEST_SUITE(sftp_data_object_tests, DataObjectFixture)

BOOST_AUTO_TEST_CASE( create )
{
	com_ptr<IDataObject> data_object = CSftpDataObject::Create(
		0, NULL, sandbox_pidl().get(), Provider().get(), Consumer().get());
	BOOST_REQUIRE(data_object);
}

/**
 * Ask for the SHELLIDLIST format.
 * This should hold the PIDLs that the DataObject was originally initialised
 * with.
 */
BOOST_AUTO_TEST_CASE( pidls )
{
	make_test_files();

	vector<cpidl_t> pidls = pidls_in_sandbox();

	com_ptr<IDataObject> data_object = data_object_from_sandbox();
	PidlFormat format(data_object);

	BOOST_REQUIRE(pidl_equivalence(format.parent_folder(), sandbox_pidl()));
	BOOST_REQUIRE(pidl_equivalence(format.file(0), sandbox_pidl() + pidls[0]));
	BOOST_REQUIRE(pidl_equivalence(format.file(1), sandbox_pidl() + pidls[1]));
	BOOST_REQUIRE_EQUAL(format.pidl_count(), pidls.size());
}

/**
 * Ask for the HDROP format.
 * This should fail as the SftpDataObject can't render such a format.
 */
BOOST_AUTO_TEST_CASE( hdrop )
{
	make_test_files();

	com_ptr<IDataObject> data_object = data_object_from_sandbox();

	FORMATETC fetc = {
		CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
	};
	
	StorageMedium medium;
	HRESULT hr = data_object->GetData(&fetc, medium.out());

	BOOST_REQUIRE(FAILED(hr));
}

void do_filedescriptor_test(
	const com_ptr<IDataObject>& data_object, const vector<wpath>& files)
{
	FORMATETC fetc = {
		CF_FILEDESCRIPTORW, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
	};
	
	StorageMedium medium;
	HRESULT hr = data_object->GetData(&fetc, medium.out());
	BOOST_REQUIRE_OK(hr);

	FileGroupDescriptor fgd(medium.get().hGlobal);
	BOOST_REQUIRE_EQUAL(fgd.size(), files.size());
	for (size_t i = 0; i < files.size(); ++i)
	{
		BOOST_REQUIRE_EQUAL(fgd[i].path(), files[i].filename());
		BOOST_REQUIRE_EQUAL(fgd[i].file_size(), file_size(files[i]));
		BOOST_REQUIRE_EQUAL(
			fgd[i].last_write_time(), from_time_t(last_write_time(files[i])));
	}
}

void do_filecontents_test(
	const com_ptr<IDataObject>& data_object, const vector<wpath>& files,
	int index)
{
	FORMATETC fetc = {
		CF_FILECONTENTS, NULL, DVASPECT_CONTENT, index, TYMED_ISTREAM
	};
	
	StorageMedium medium;
	HRESULT hr = data_object->GetData(&fetc, medium.out());
	BOOST_REQUIRE_OK(hr);

	com_ptr<IStream> stream = medium.get().pstm;
	BOOST_REQUIRE(file_stream_equivalence(files.at(index), stream));
}

/**
 * Ask for the FILEDESCRIPTOR format.
 * This should provide streams to the test files via the SSH connection.
 */
BOOST_AUTO_TEST_CASE( file_descriptor )
{
	vector<wpath> files = make_test_files();

	com_ptr<IDataObject> data_object = data_object_from_sandbox();

	do_filedescriptor_test(data_object, files);

	for (size_t i = 0; i < files.size(); ++i)
		do_filecontents_test(data_object, files, i);
}

/**
 * Ask for the FILEDESCRIPTOR format.
 * This should provide streams to the test files via the SSH connection.
 */
BOOST_AUTO_TEST_CASE( file_descriptor_readonly )
{
	vector<wpath> files = make_test_files(true);

	com_ptr<IDataObject> data_object = data_object_from_sandbox();

	do_filedescriptor_test(data_object, files);

	for (size_t i = 0; i < files.size(); ++i)
		do_filecontents_test(data_object, files, i);
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion
