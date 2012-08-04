/**
    @file

    Test IStream implementation over a real network connection.

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

#include "test/common_boost/fixtures.hpp" // WinsockFixture
#include "test/common_boost/MockConsumer.hpp" // MockConsumer
#include "test/common_boost/remote_test_config.hpp" // remote_test_config
#include "test/common_boost/stream_utils.hpp" // verify_stream_read

#include "swish/provider/SftpStream.hpp"
#include "swish/provider/SessionFactory.hpp" // CSessionFactory
#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer

#include <comet/ptr.h> // com_ptr

#include <boost/filesystem/path.hpp> // wpath
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/test/unit_test.hpp>

#include <algorithm> // generate
#include <cstdlib> // rand
#include <memory>  // auto_ptr
#include <string>
#include <vector>

using test::MockConsumer;
using test::remote_test_config;
using test::stream_utils::verify_stream_read;
using test::WinsockFixture;

using comet::com_ptr;

using boost::filesystem::wpath;
using boost::numeric_cast;
using boost::shared_ptr;

using std::auto_ptr;
using std::string;
using std::generate;
using std::rand;
using std::vector;

namespace {

class RemoteSftpFixture : public WinsockFixture
{
public:
    RemoteSftpFixture() : m_consumer(new MockConsumer())
    {
        remote_test_config config;
        m_consumer->set_password_behaviour(MockConsumer::CustomPassword);
        m_consumer->set_password(config.GetPassword());

        m_session = shared_ptr<CSession>(CSessionFactory::CreateSftpSession(
            config.GetHost().c_str(), config.GetPort(),
            config.GetUser().c_str(), m_consumer.get()));
    }

    shared_ptr<CSession> session() const
    {
        return m_session;
    }

private:
    com_ptr<MockConsumer> m_consumer;
    shared_ptr<CSession> m_session;
};

}

BOOST_FIXTURE_TEST_SUITE( remote_stream_tests, RemoteSftpFixture )

/**
 * Simply get a stream.
 */
BOOST_AUTO_TEST_CASE( get )
{
    shared_ptr<CSession> session(session());

    com_ptr<IStream> stream = new CSftpStream(
        session, "/var/log/syslog", CSftpStream::read);
    BOOST_REQUIRE(stream);
}

BOOST_AUTO_TEST_CASE( stat )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/var/log/syslog", CSftpStream::read);

    STATSTG stat = STATSTG();
    HRESULT hr = stream->Stat(&stat, STATFLAG_DEFAULT);
    BOOST_REQUIRE_OK(hr);

    BOOST_CHECK(stat.pwcsName);
    BOOST_CHECK_EQUAL(L"syslog", stat.pwcsName);
    BOOST_CHECK_EQUAL(STGTY_STREAM, (STGTY)stat.type);
    BOOST_CHECK_GT(stat.cbSize.QuadPart, 0U);
    FILETIME ft;
    BOOST_REQUIRE_OK(::CoFileTimeNow(&ft));
    BOOST_CHECK_EQUAL(::CompareFileTime(&ft, &(stat.mtime)), 1);
    BOOST_CHECK_EQUAL(::CompareFileTime(&ft, &(stat.atime)), 1);
    BOOST_CHECK_EQUAL(::CompareFileTime(&ft, &(stat.ctime)), 1);
    BOOST_CHECK_EQUAL(stat.grfMode, 0U);
    BOOST_CHECK_EQUAL(stat.grfLocksSupported, 0U);
    BOOST_CHECK(stat.clsid == GUID_NULL);
    BOOST_CHECK_EQUAL(stat.grfStateBits, 0U);
    BOOST_CHECK_EQUAL(stat.reserved, 0U);
}

BOOST_AUTO_TEST_CASE( stat_exclude_name )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/var/log/syslog", CSftpStream::read);

    STATSTG stat = STATSTG();
    HRESULT hr = stream->Stat(&stat, STATFLAG_NONAME);
    BOOST_REQUIRE_OK(hr);

    BOOST_CHECK(!stat.pwcsName);
    BOOST_CHECK_EQUAL(STGTY_STREAM, (STGTY)stat.type);
    BOOST_CHECK_GT(stat.cbSize.QuadPart, 0U);
    FILETIME ft;
    BOOST_REQUIRE_OK(::CoFileTimeNow(&ft));
    BOOST_CHECK_EQUAL(::CompareFileTime(&ft, &(stat.mtime)), 1);
    BOOST_CHECK_EQUAL(::CompareFileTime(&ft, &(stat.atime)), 1);
    BOOST_CHECK_EQUAL(::CompareFileTime(&ft, &(stat.ctime)), 1);
    BOOST_CHECK_EQUAL(stat.grfMode, 0U);
    BOOST_CHECK_EQUAL(stat.grfLocksSupported, 0U);
    BOOST_CHECK(stat.clsid == GUID_NULL);
    BOOST_CHECK_EQUAL(stat.grfStateBits, 0U);
    BOOST_CHECK_EQUAL(stat.reserved, 0U);
}

BOOST_AUTO_TEST_CASE( read_file_small_buffer )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/proc/cpuinfo", CSftpStream::read);

    string file_contents_read;
    ULONG bytes_read = 0;
    char buf[1];
    HRESULT hr;
    do {
        hr = stream->Read(buf, ARRAYSIZE(buf), &bytes_read);
        file_contents_read.append(buf, bytes_read);
    } while (hr == S_OK && bytes_read == ARRAYSIZE(buf));

    BOOST_CHECK_GT(file_contents_read.size(), 100U);
    BOOST_CHECK_EQUAL("processor", file_contents_read.substr(0, 9));
}

BOOST_AUTO_TEST_CASE( read_file_medium_buffer )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/proc/cpuinfo", CSftpStream::read);

    string file_contents_read;
    ULONG bytes_read = 0;
    char buf[4096];
    HRESULT hr;
    do {
        hr = stream->Read(buf, ARRAYSIZE(buf), &bytes_read);
        file_contents_read.append(buf, bytes_read);
    } while (hr == S_OK && bytes_read == ARRAYSIZE(buf));

    BOOST_CHECK_GT(file_contents_read.size(), 100U);
    BOOST_CHECK_EQUAL("processor", file_contents_read.substr(0, 9));
}

/**
 * This highlights problems caused by short reads.
 * /dev/random produces data very slowly so the stream should block while
 * waiting for more data to become available.
 * libssh2 seems to get this wrong between 1.2.8 and 1.3.0
 */
BOOST_AUTO_TEST_CASE( read_small_buffer_from_slow_blocking_device )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/dev/random", CSftpStream::read);

    vector<char> buffer(15, 'x');
    size_t bytes_read = verify_stream_read(&buffer[0], buffer.size(), stream);

    BOOST_CHECK_EQUAL(bytes_read, buffer.size());
}

/**
 * This tests a scenario that should *never* block.
 * /dev/zero immediately produces an endless stream of zeroes so the stream
 * should just keep reading until the buffer is full.  If it blocks, something
 * has gone wrong somewhere.
 */
BOOST_AUTO_TEST_CASE( read_large_buffer )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/dev/zero", CSftpStream::read);

    // using int to get legible output when collection comparison fails
    vector<int> buffer(20000, 74);
    size_t size = buffer.size() * sizeof(buffer[0]);
    size_t bytes_read = verify_stream_read(&buffer[0], size, stream);

    BOOST_CHECK_EQUAL(bytes_read, size);

    vector<int> expected(20000, 0);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        buffer.begin(), buffer.end(), expected.begin(), expected.end());
}

namespace {

    vector<int> random_buffer(size_t buffer_size)
    {
        vector<int> buffer(buffer_size);
        generate(buffer.begin(), buffer.end(), rand);
        return buffer;
    }
}

/**
 * This tests a scenario that should *never* block.
 * /dev/zero immediately produces an endless stream of zeroes so the stream
 * should just keep reading until the buffer is full.  If it blocks, something
 * has gone wrong somewhere.
 */
BOOST_AUTO_TEST_CASE( roundtrip )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "test_file",
        CSftpStream::read | CSftpStream::write | CSftpStream::create);

    // using int to get legible output when collection comparison fails
    vector<int> source_data = random_buffer(6543210);
    ULONG size_in_bytes = numeric_cast<ULONG>(
        source_data.size() * sizeof(source_data[0]));

    ULONG bytes_written = 0;
    HRESULT hr = stream->Write(&source_data[0], size_in_bytes, &bytes_written);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK_EQUAL(bytes_written, size_in_bytes);

    LARGE_INTEGER dlibMove = {0};
    ULARGE_INTEGER dlibNewPos = {0};
    hr = stream->Seek(dlibMove, STREAM_SEEK_SET, &dlibNewPos);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, 0UL);

    vector<int> buffer(source_data.size(), 33);
    size_t bytes_read = verify_stream_read(&buffer[0], size_in_bytes, stream);

    BOOST_CHECK_EQUAL(bytes_read, size_in_bytes);

    BOOST_CHECK_EQUAL_COLLECTIONS(
        buffer.begin(), buffer.end(), source_data.begin(), source_data.end());
}

BOOST_AUTO_TEST_CASE( read_empty_file )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/dev/null", CSftpStream::read);

    vector<char> buffer(6543210, 'x');
    size_t bytes_read = verify_stream_read(&buffer[0], buffer.size(), stream);

    BOOST_CHECK_EQUAL(bytes_read, 0U);

    char expected[] = { 'x', 'x', 'x', 'x' };
    BOOST_CHECK_EQUAL_COLLECTIONS(
        &buffer[0], &buffer[0] + 4, expected, expected + 4);
}

BOOST_AUTO_TEST_CASE( seek_noop )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/var/log/syslog", CSftpStream::read);

    HRESULT hr;

    // Move by 0 relative to current position
    {
        LARGE_INTEGER dlibMove = {0};
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, 0UL);
    }

    // Move by 0 but don't provide return slot
    {
        LARGE_INTEGER dlibMove = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_CUR, NULL);
        BOOST_REQUIRE_OK(hr);
    }
}

BOOST_AUTO_TEST_CASE( seek_relative )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/var/log/syslog", CSftpStream::read);

    HRESULT hr;

    // Move by 7 relative to current position: absolute pos 7
    {
        LARGE_INTEGER dlibMove = {7};
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, 7UL);
    }

    // Move by 7 relative to current position: absolute pos 14
    {
        LARGE_INTEGER dlibMove = {7};
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, 14UL);
    }

    // Move by -5 relative to current position: absolute pos 9
    {
        LARGE_INTEGER dlibMove;
        dlibMove.QuadPart = -5;
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, 9UL);
    }
}

BOOST_AUTO_TEST_CASE( seek_relative_fail )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/var/log/syslog", CSftpStream::read);

    HRESULT hr;

    // Move by 7 relative to current position: absolute pos 7
    {
        LARGE_INTEGER dlibMove = {7};
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, 7UL);
    }

    // Move by -9 relative to current position: absolute pos -2
    {
        LARGE_INTEGER dlibMove;
        dlibMove.QuadPart = -9;
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
        BOOST_CHECK(hr == STG_E_INVALIDFUNCTION);
    }
}

BOOST_AUTO_TEST_CASE( seek_absolute )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/var/log/syslog", CSftpStream::read);

    HRESULT hr;

    // Move to absolute position 7
    {
        LARGE_INTEGER dlibMove = {7};
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_SET, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, 7UL);
    }

    // Move to absolute position 14
    {
        LARGE_INTEGER dlibMove = {14};
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_SET, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, 14UL);
    }

    // Move to beginning of file: absolute position 0
    {
        LARGE_INTEGER dlibMove = {0};
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_SET, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, 0UL);
    }
}

BOOST_AUTO_TEST_CASE( seek_absolute_fail )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/var/log/syslog", CSftpStream::read);

    HRESULT hr;

    // Move to absolute position -3
    {
        LARGE_INTEGER dlibMove;
        dlibMove.QuadPart = -3;
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_SET, &dlibNewPos);
        BOOST_CHECK(hr == STG_E_INVALIDFUNCTION);
    }
}

BOOST_AUTO_TEST_CASE( seek_get_current_position )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/var/log/syslog", CSftpStream::read);
    HRESULT hr;

    // Move to absolute position 7
    {
        LARGE_INTEGER dlibMove = {7};
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_SET, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, 7UL);
    }

    // Move by 0 relative to current pos which should return current pos
    {
        LARGE_INTEGER dlibMove = {0};
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, 7UL);
    }
}

BOOST_AUTO_TEST_CASE( seek_relative_to_end )
{
    com_ptr<IStream> stream = new CSftpStream(
        session(), "/var/log/syslog", CSftpStream::read);

    HRESULT hr;

    ULONGLONG uSize;
    // Move to end of file: absolute position 0 from end
    {
        LARGE_INTEGER dlibMove = {0};
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_END, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        // Should be a fairly large number
        BOOST_CHECK_GT(dlibNewPos.QuadPart, 100UL);
        uSize = dlibNewPos.QuadPart;
    }

    // Move to absolute position 7 from end of file
    {
        LARGE_INTEGER dlibMove = {7};
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_END, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        // Should be a fairly large number
        BOOST_CHECK_GT(dlibNewPos.QuadPart, 100UL);
        // Should be size of file minus 7
        BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, uSize - 7);
    }

    // Move 50 past end of the file: this should still succeed
    {
        LARGE_INTEGER dlibMove;
        dlibMove.QuadPart = -50;
        ULARGE_INTEGER dlibNewPos = {0};
        hr = stream->Seek(dlibMove, STREAM_SEEK_END, &dlibNewPos);
        BOOST_REQUIRE_OK(hr);
        // Should be a fairly large number
        BOOST_CHECK_GT(dlibNewPos.QuadPart, 100UL);
        // Should be size of file plus 50
        BOOST_CHECK_EQUAL(dlibNewPos.QuadPart, uSize + 50);
    }
}
BOOST_AUTO_TEST_SUITE_END()


/*
    void testStatExact()
    {
        CComPtr<IStream> pStream = _CreateConnectInit("/boot/grub/default");

        STATSTG stat;
        ::ZeroMemory(&stat, sizeof stat);
        HRESULT hr = stream->Stat(&stat, STATFLAG_DEFAULT);
        BOOST_REQUIRE_OK(hr);

        BOOST_CHECK(stat.pwcsName);
        BOOST_CHECK_EQUAL(CString(L"default"), CString(stat.pwcsName));
        BOOST_CHECK_EQUAL(STGTY_STREAM, (STGTY)stat.type);
        BOOST_CHECK_EQUAL((ULONGLONG)197, stat.cbSize.QuadPart);
        BOOST_CHECK_EQUAL((DWORD)0, stat.grfMode);
        BOOST_CHECK_EQUAL((DWORD)0, stat.grfLocksSupported);
        //BOOST_CHECK_EQUAL(GUID_NULL, stat.clsid);
        BOOST_CHECK_EQUAL((DWORD)0, stat.grfStateBits);
        BOOST_CHECK_EQUAL((DWORD)0, stat.reserved);
    }

    void testReadFileExact()
    {
        CComPtr<IStream> pStream = _CreateConnectInit("/boot/grub/default");
        HRESULT hr;

        CStringA strFile;
        ULONG cbRead = 0;
        char buf[4096];
        do {
            hr = stream->Read(buf, ARRAYSIZE(buf), &cbRead);
            strFile.Append(buf, cbRead);
        } while (hr == S_OK && cbRead == ARRAYSIZE(buf));

        BOOST_CHECK_EQUAL(197, strFile.GetLength());
        BOOST_CHECK_EQUAL(CStringA(szTestFile), strFile);
    }

*/

