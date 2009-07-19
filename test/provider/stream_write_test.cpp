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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.

    @endif
*/

#include "swish/provider/SftpStream.hpp"  // Test subject
#include "swish/provider/SessionFactory.hpp"
#include "swish/utils.hpp"

#include "test/common_boost/fixtures.hpp"
#include "test/common_boost/ConsumerStub.hpp"
#include "test/common_boost/helpers.hpp"

#include "swish/atl.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/system/system_error.hpp>

#include <Winsock2.h>

#include <string>
#include <vector>
#include <memory>

using swish::utils::WideStringToUtf8String;
using swish::utils::GetCurrentUser;
using swish::utils::Utf8StringToWideString;

using test::common_boost::ComFixture;
using test::common_boost::OpenSshFixture;
using test::common_boost::CConsumerStub;

using ATL::CComPtr;

using boost::filesystem::wpath;

using std::string;
using std::wstring;
using std::vector;
using std::auto_ptr;

namespace { // private

	const wstring SANDBOX_NAME = L"swish-sandbox";

	wpath NewTempFilePath()
	{
		vector<wchar_t> buffer(MAX_PATH);
		DWORD len = ::GetTempPath(buffer.size(), &buffer[0]);
		BOOST_REQUIRE_LE(len, buffer.size());
		
		wpath directory(wstring(&buffer[0], buffer.size()));
		directory /= SANDBOX_NAME;
		create_directory(directory);

		if (!GetTempFileName(
			directory.directory_string().c_str(), NULL, 0, &buffer[0]))
			throw boost::system::system_error(
				::GetLastError(), boost::system::system_category);
		
		return wpath(wstring(&buffer[0], buffer.size()));
	}

	class SandboxFixture : public ComFixture<OpenSshFixture>
	{
	public:
		SandboxFixture() : m_sandboxFile(NewTempFilePath())
		{
			WSADATA wsadata;
			int err = ::WSAStartup(MAKEWORD(2, 2), &wsadata);
			if (err)
				throw boost::system::system_error(
					err, boost::system::system_category);
		}

		~SandboxFixture()
		{
			try
			{
				remove(m_sandboxFile);
			}
			catch (...) {}

			::WSACleanup();
		}
		
		/**
		 * Create an IStream instance open for writing on a temporary file
		 * in our sandbox.
		 */
		CComPtr<IStream> GetStream()
		{
			auto_ptr<CSession> session(GetSession());
			string file = ToRemotePath(
				WideStringToUtf8String(m_sandboxFile.file_string()));

			CComPtr<IStream> stream = CSftpStream::Create(
				session, file, CSftpStream::read | CSftpStream::write);
			return stream;
		}

		auto_ptr<CSession> GetSession()
		{
			CComPtr<CConsumerStub> spConsumer = 
				CConsumerStub::CreateCoObject();
			spConsumer->SetKeyPaths(GetPrivateKey(), GetPublicKey());

			return CSessionFactory::CreateSftpSession(
				Utf8StringToWideString(GetHost()).c_str(), GetPort(), 
				GetCurrentUser().c_str(), spConsumer);
		}

		wpath m_sandboxFile;
	};
}


/**
 * Exercise our test helper function.
 */
BOOST_AUTO_TEST_CASE( create_new_temp )
{
	wpath p = NewTempFilePath();
	BOOST_CHECK(exists(p));
	BOOST_CHECK(is_regular_file(p));
	BOOST_CHECK(p.is_complete());
	remove(p);
}


BOOST_FIXTURE_TEST_SUITE(StreamWrite, SandboxFixture)

/**
 * Simply get a session.
 */
BOOST_AUTO_TEST_CASE( get_session )
{
	auto_ptr<CSession> session(GetSession());
	BOOST_REQUIRE(session.get());
}

/**
 * Simply get a stream.
 */
BOOST_AUTO_TEST_CASE( get )
{
	CComPtr<IStream> spStream = GetStream();
	BOOST_REQUIRE(spStream);
}

/**
 * Write one byte to stream, read it back and check that it is the same.
 */
BOOST_AUTO_TEST_CASE( write_one_byte )
{
	CComPtr<IStream> spStream = GetStream();

	// Write the character 'M' to the file
	char in[1] = {'M'};
	ULONG cbWritten = 0;
	BOOST_REQUIRE_OK(spStream->Write(in, sizeof(in), &cbWritten));
	BOOST_REQUIRE_EQUAL(cbWritten, sizeof(in));

	// Reset seek pointer to beginning and read back
	LARGE_INTEGER move = {0};
	BOOST_REQUIRE_OK(spStream->Seek(move, STREAM_SEEK_SET, NULL));

	char out[1];
	ULONG cbRead = 0;
	BOOST_REQUIRE_OK(spStream->Read(out, sizeof(out), &cbRead));
	BOOST_REQUIRE_EQUAL(cbRead, sizeof(out));
	BOOST_REQUIRE_EQUAL('M', out[0]);
	
	// Reading another byte should succeed but return 0 bytes read
	BOOST_REQUIRE_OK(spStream->Read(out, sizeof(out), &cbRead));
	BOOST_REQUIRE_EQUAL(cbRead, 0U);
}

/**
 * Write a sequence of characters.
 */
BOOST_AUTO_TEST_CASE( write_a_string )
{
	CComPtr<IStream> spStream = GetStream();

	string in = "Lorem ipsum dolor sit amet. ";
	ULONG cbWritten = 0;
	BOOST_REQUIRE_OK(spStream->Write(&in[0], in.size(), &cbWritten));
	BOOST_REQUIRE_EQUAL(cbWritten, in.size());

	// Reset seek pointer to beginning and read back
	LARGE_INTEGER move = {0};
	BOOST_REQUIRE_OK(spStream->Seek(move, STREAM_SEEK_SET, NULL));

	vector<char> out(in.size());
	ULONG cbRead = 0;
	BOOST_REQUIRE_OK(spStream->Read(&out[0], out.size(), &cbRead));
	BOOST_REQUIRE_EQUAL(cbRead, out.size());
	BOOST_REQUIRE_EQUAL_COLLECTIONS(
		out.begin(), out.end(), in.begin(), in.end());
	
	// Trying to read more should succeed but return 0 bytes read
	BOOST_REQUIRE_OK(spStream->Read(&out[0], out.size(), &cbRead));
	BOOST_REQUIRE_EQUAL(cbRead, 0U);
}

BOOST_AUTO_TEST_SUITE_END()
