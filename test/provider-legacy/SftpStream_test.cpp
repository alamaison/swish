/**
 * @file
 *
 * Tests for the IStream implementation.
 */

#include "standard.h"

#include "test/common/CppUnitExtensions.h"
#include "test/common/TestConfig.h"
#include "test/common/MockSftpConsumer.h"

#include "swish/provider/SftpStream.hpp"
#include "swish/provider/SessionFactory.hpp"

#include <memory>  // auto_ptr

#include "swish/shell_folder/SftpProvider.h"  // ISftpProvider/Consumer

using namespace ATL;
using std::auto_ptr;

static const char *szTestFile = 
	"default\n#\n#\n#\n#\n#\n#\n#\n#\n#\n#\n# WARNING: If you want to edit "
	"this file directly, do not remove any line\n# from this file, "
	"including this warning. Using `grub-set-default\\' is\n# strongly "
	"recommended.\n";

class CSftpStream_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CSftpStream_test );
		CPPUNIT_TEST( testCreate );
		CPPUNIT_TEST( testCreateUsingFactory );
		CPPUNIT_TEST( testStat );
		CPPUNIT_TEST( testStatExcludeName );
		CPPUNIT_TEST( testStatExact ); // Likely to fail on other machine
		CPPUNIT_TEST( testSeekNoOp );
		CPPUNIT_TEST( testSeekRelative );
		CPPUNIT_TEST( testSeekRelativeFail );
		CPPUNIT_TEST( testSeekAbsolute );
		CPPUNIT_TEST( testSeekAbsoluteFail );
		CPPUNIT_TEST( testSeekRelativeToEnd );
		CPPUNIT_TEST( testSeekGetCurrentPos );
		CPPUNIT_TEST( testReadABit );
		CPPUNIT_TEST( testReadFile );
		CPPUNIT_TEST( testReadFileSmallBuffer );
		CPPUNIT_TEST( testReadFileLargeBuffer );
		CPPUNIT_TEST( testReadFileMassiveBuffer );
		CPPUNIT_TEST( testReadFileExact ); // Likely to fail on other machine
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp()
	{
		// Start up Winsock
		WSADATA wsadata;
		CPPUNIT_ASSERT( ::WSAStartup(WINSOCK_VERSION, &wsadata) == 0 );

		// Create mock
		CMockSftpConsumer::Create(m_spCoConsumer, m_spConsumer);

		// Create session
		m_spSession = _CreateSession();
	}

	void tearDown()
	{
		// Destroy session
		m_spSession.reset();

		// Shut down Winsock
		CPPUNIT_ASSERT( ::WSACleanup() == 0 );
	}

protected:
	void testCreate()
	{
		HRESULT hr;
		CComObject<CSftpStream> *pStream = NULL;

		hr = pStream->CreateInstance(&pStream);
		CPPUNIT_ASSERT_OK(hr);
		pStream->AddRef();

		IStream *p = NULL;
		hr = pStream->QueryInterface(&p);
		if (SUCCEEDED(hr))
		{
			p->Release();
		}
		pStream->Release();
		CPPUNIT_ASSERT_OK(hr);
	}

	void testCreateUsingFactory()
	{
		auto_ptr<CSession> session = _CreateSession();
		session->Connect(config.GetHost(), config.GetPort());

		CComPtr<CSftpStream> pStream = pStream->Create(
			session, "/var/log/messages");
		CPPUNIT_ASSERT(pStream);
	}

	void testStat()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/var/log/messages");

		STATSTG stat;
		::ZeroMemory(&stat, sizeof stat);
		HRESULT hr = pStream->Stat(&stat, STATFLAG_DEFAULT);
		CPPUNIT_ASSERT_OK(hr);

		CPPUNIT_ASSERT(stat.pwcsName);
		CPPUNIT_ASSERT_EQUAL(CString(L"messages"), CString(stat.pwcsName));
		CPPUNIT_ASSERT_EQUAL(STGTY_STREAM, (STGTY)stat.type);
		CPPUNIT_ASSERT(stat.cbSize.QuadPart > 0);
		FILETIME ft;
		CPPUNIT_ASSERT_OK(::CoFileTimeNow(&ft));
		CPPUNIT_ASSERT_EQUAL((LONG)1, ::CompareFileTime(&ft, &(stat.mtime)));
		CPPUNIT_ASSERT_EQUAL((LONG)1, ::CompareFileTime(&ft, &(stat.atime)));
		CPPUNIT_ASSERT_EQUAL((LONG)1, ::CompareFileTime(&ft, &(stat.ctime)));
		CPPUNIT_ASSERT_EQUAL((DWORD)0, stat.grfMode);
		CPPUNIT_ASSERT_EQUAL((DWORD)0, stat.grfLocksSupported);
		//CPPUNIT_ASSERT_EQUAL(GUID_NULL, stat.clsid);
		CPPUNIT_ASSERT_EQUAL((DWORD)0, stat.grfStateBits);
		CPPUNIT_ASSERT_EQUAL((DWORD)0, stat.reserved);
	}

	void testStatExcludeName()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/var/log/messages");

		STATSTG stat;
		::ZeroMemory(&stat, sizeof stat);
		HRESULT hr = pStream->Stat(&stat, STATFLAG_NONAME);
		CPPUNIT_ASSERT_OK(hr);

		CPPUNIT_ASSERT(stat.pwcsName == NULL);
		CPPUNIT_ASSERT_EQUAL(STGTY_STREAM, (STGTY)stat.type);
		CPPUNIT_ASSERT(stat.cbSize.QuadPart > 0);
		FILETIME ft;
		CPPUNIT_ASSERT_OK(::CoFileTimeNow(&ft));
		CPPUNIT_ASSERT_EQUAL((LONG)1, ::CompareFileTime(&ft, &(stat.mtime)));
		CPPUNIT_ASSERT_EQUAL((LONG)1, ::CompareFileTime(&ft, &(stat.atime)));
		CPPUNIT_ASSERT_EQUAL((LONG)1, ::CompareFileTime(&ft, &(stat.ctime)));
		CPPUNIT_ASSERT_EQUAL((DWORD)0, stat.grfMode);
		CPPUNIT_ASSERT_EQUAL((DWORD)0, stat.grfLocksSupported);
		//CPPUNIT_ASSERT_EQUAL(GUID_NULL, stat.clsid);
		CPPUNIT_ASSERT_EQUAL((DWORD)0, stat.grfStateBits);
		CPPUNIT_ASSERT_EQUAL((DWORD)0, stat.reserved);
	}

	void testStatExact()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/boot/grub/default");

		STATSTG stat;
		::ZeroMemory(&stat, sizeof stat);
		HRESULT hr = pStream->Stat(&stat, STATFLAG_DEFAULT);
		CPPUNIT_ASSERT_OK(hr);

		CPPUNIT_ASSERT(stat.pwcsName);
		CPPUNIT_ASSERT_EQUAL(CString(L"default"), CString(stat.pwcsName));
		CPPUNIT_ASSERT_EQUAL(STGTY_STREAM, (STGTY)stat.type);
		CPPUNIT_ASSERT_EQUAL((ULONGLONG)197, stat.cbSize.QuadPart);
		CPPUNIT_ASSERT_EQUAL((DWORD)0, stat.grfMode);
		CPPUNIT_ASSERT_EQUAL((DWORD)0, stat.grfLocksSupported);
		//CPPUNIT_ASSERT_EQUAL(GUID_NULL, stat.clsid);
		CPPUNIT_ASSERT_EQUAL((DWORD)0, stat.grfStateBits);
		CPPUNIT_ASSERT_EQUAL((DWORD)0, stat.reserved);
	}

	void testSeekNoOp()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/var/log/messages");
		HRESULT hr;

		// Move by 0 relative to current position
		{
			LARGE_INTEGER dlibMove = {0};
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			CPPUNIT_ASSERT_EQUAL((ULONGLONG)0, dlibNewPos.QuadPart);
		}

		// Move by 0 but don't provide return slot
		{
			LARGE_INTEGER dlibMove = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_CUR, NULL);
			CPPUNIT_ASSERT_OK(hr);
		}
	}

	void testSeekRelative()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/var/log/messages");
		HRESULT hr;

		// Move by 7 relative to current position: absolute pos 7
		{
			LARGE_INTEGER dlibMove = {7};
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			CPPUNIT_ASSERT_EQUAL((ULONGLONG)7, dlibNewPos.QuadPart);
		}

		// Move by 7 relative to current position: absolute pos 14
		{
			LARGE_INTEGER dlibMove = {7};
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			CPPUNIT_ASSERT_EQUAL((ULONGLONG)14, dlibNewPos.QuadPart);
		}

		// Move by -5 relative to current position: absolute pos 9
		{
			LARGE_INTEGER dlibMove;
			dlibMove.QuadPart = -5;
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			CPPUNIT_ASSERT_EQUAL((ULONGLONG)9, dlibNewPos.QuadPart);
		}
	}

	void testSeekRelativeFail()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/var/log/messages");
		HRESULT hr;

		// Move by 7 relative to current position: absolute pos 7
		{
			LARGE_INTEGER dlibMove = {7};
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			CPPUNIT_ASSERT_EQUAL((ULONGLONG)7, dlibNewPos.QuadPart);
		}

		// Move by -9 relative to current position: absolute pos -2
		{
			LARGE_INTEGER dlibMove;
			dlibMove.QuadPart = -9;
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
			CPPUNIT_ASSERT(hr == STG_E_INVALIDFUNCTION);
		}
	}

	void testSeekAbsolute()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/var/log/messages");
		HRESULT hr;

		// Move to absolute position 7
		{
			LARGE_INTEGER dlibMove = {7};
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_SET, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			CPPUNIT_ASSERT_EQUAL((ULONGLONG)7, dlibNewPos.QuadPart);
		}

		// Move to absolute position 14
		{
			LARGE_INTEGER dlibMove = {14};
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_SET, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			CPPUNIT_ASSERT_EQUAL((ULONGLONG)14, dlibNewPos.QuadPart);
		}

		// Move to beginning of file: absolute position 0
		{
			LARGE_INTEGER dlibMove = {0};
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_SET, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			CPPUNIT_ASSERT_EQUAL((ULONGLONG)0, dlibNewPos.QuadPart);
		}
	}

	void testSeekAbsoluteFail()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/var/log/messages");
		HRESULT hr;

		// Move to absolute position -3
		{
			LARGE_INTEGER dlibMove;
			dlibMove.QuadPart = -3;
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_SET, &dlibNewPos);
			CPPUNIT_ASSERT(hr == STG_E_INVALIDFUNCTION);
		}
	}

	void testSeekGetCurrentPos()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/var/log/messages");
		HRESULT hr;

		// Move to absolute position 7
		{
			LARGE_INTEGER dlibMove = {7};
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_SET, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			CPPUNIT_ASSERT_EQUAL((ULONGLONG)7, dlibNewPos.QuadPart);
		}

		// Move by 0 relative to current pos which should return current pos
		{
			LARGE_INTEGER dlibMove = {0};
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_CUR, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			CPPUNIT_ASSERT_EQUAL((ULONGLONG)7, dlibNewPos.QuadPart);
		}
	}

	void testSeekRelativeToEnd()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/var/log/messages");
		HRESULT hr;

		ULONGLONG uSize;
		// Move to end of file: absolute position 0 from end
		{
			LARGE_INTEGER dlibMove = {0};
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_END, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			// Should be a fairly large number
			CPPUNIT_ASSERT(dlibNewPos.QuadPart > 100);
			uSize = dlibNewPos.QuadPart;
		}

		// Move to absolute position 7 from end of file
		{
			LARGE_INTEGER dlibMove = {7};
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_END, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			// Should be a fairly large number
			CPPUNIT_ASSERT(dlibNewPos.QuadPart > 100);
			// Should be size of file minus 7
			CPPUNIT_ASSERT_EQUAL(uSize-7, dlibNewPos.QuadPart);
		}

		// Move 50 past end of the file: this should still succeed
		{
			LARGE_INTEGER dlibMove;
			dlibMove.QuadPart = -50;
			ULARGE_INTEGER dlibNewPos = {0};
			hr = pStream->Seek(dlibMove, STREAM_SEEK_END, &dlibNewPos);
			CPPUNIT_ASSERT_OK(hr);
			// Should be a fairly large number
			CPPUNIT_ASSERT(dlibNewPos.QuadPart > 100);
			// Should be size of file plus 50
			CPPUNIT_ASSERT_EQUAL(uSize+50, dlibNewPos.QuadPart);
		}
	}

	void testReadABit()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/proc/cpuinfo");
		HRESULT hr;

		char buf[10];
		ULONG cbRead = 0;
		hr = pStream->Read(buf, ARRAYSIZE(buf)-1, &cbRead);
		CPPUNIT_ASSERT_OK(hr);
		buf[cbRead] = '\0';

		CStringA strBuf(buf);
		CPPUNIT_ASSERT_EQUAL(9, strBuf.GetLength());
		CPPUNIT_ASSERT_EQUAL(CStringA("processor"), strBuf);
	}

	void testReadFile()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/proc/cpuinfo");
		HRESULT hr;

		CStringA strFile;
		ULONG cbRead = 0;
		char buf[100];
		do {
			hr = pStream->Read(buf, ARRAYSIZE(buf), &cbRead);
			strFile.Append(buf, cbRead);
		} while (hr == S_OK && cbRead == ARRAYSIZE(buf));

		CPPUNIT_ASSERT(strFile.GetLength() > 100);
		CPPUNIT_ASSERT_EQUAL(CStringA("processor"), strFile.Left(9));
	}

	void testReadFileSmallBuffer()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/proc/cpuinfo");
		HRESULT hr;

		CStringA strFile;
		ULONG cbRead = 0;
		char buf[1];
		do {
			hr = pStream->Read(buf, ARRAYSIZE(buf), &cbRead);
			strFile.Append(buf, cbRead);
		} while (hr == S_OK && cbRead == ARRAYSIZE(buf));

		CPPUNIT_ASSERT(strFile.GetLength() > 100);
		CPPUNIT_ASSERT_EQUAL(CStringA("processor"), strFile.Left(9));
	}

	void testReadFileLargeBuffer()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/proc/cpuinfo");
		HRESULT hr;

		CStringA strFile;
		ULONG cbRead = 0;
		char buf[4096];
		do {
			hr = pStream->Read(buf, ARRAYSIZE(buf), &cbRead);
			strFile.Append(buf, cbRead);
		} while (hr == S_OK && cbRead == ARRAYSIZE(buf));

		CPPUNIT_ASSERT_EQUAL(CStringA("processor"), strFile.Left(9));
		CPPUNIT_ASSERT(strFile.GetLength() > 100);
	}

	void testReadFileMassiveBuffer()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit(
			"/usr/share/example-content/GIMP_Ubuntu_splash_screen.xcf");
		HRESULT hr;

		ULONG cbRead = 0;
		char *aBuf = new char[6543210];
		hr = pStream->Read(aBuf, 6543210, &cbRead);
		char buf[4];
		::memcpy(buf, aBuf, 4);
		delete [] aBuf;

		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT_EQUAL((ULONG)840814, cbRead);
		CPPUNIT_ASSERT_EQUAL(CStringA("gimp"), CStringA(buf, 4));
	}

	void testReadFileExact()
	{
		CComPtr<CSftpStream> pStream = _CreateConnectInit("/boot/grub/default");
		HRESULT hr;

		CStringA strFile;
		ULONG cbRead = 0;
		char buf[4096];
		do {
			hr = pStream->Read(buf, ARRAYSIZE(buf), &cbRead);
			strFile.Append(buf, cbRead);
		} while (hr == S_OK && cbRead == ARRAYSIZE(buf));

		CPPUNIT_ASSERT_EQUAL(197, strFile.GetLength());
		CPPUNIT_ASSERT_EQUAL(CStringA(szTestFile), strFile);
	}

private:
	CTestConfig config;

	CComPtr<CMockSftpConsumer> m_spCoConsumer;
	CComPtr<ISftpConsumer> m_spConsumer;
	auto_ptr<CSession> m_spSession;

	auto_ptr<CSession> _CreateSession()
	{
		// Set mock to login successfully
		m_spCoConsumer->SetKeyboardInteractiveBehaviour(
			CMockSftpConsumer::CustomResponse);
		m_spCoConsumer->SetPasswordBehaviour(
			CMockSftpConsumer::CustomPassword);
		m_spCoConsumer->SetCustomPassword(config.GetPassword());

		// Create a session using the factory and mock consumer
		return CSessionFactory::CreateSftpSession(
			config.GetHost(), config.GetPort(), config.GetUser(), m_spConsumer);
	}

	CComPtr<CSftpStream> _CreateConnectInit(PCSTR pszFilePath)
	{
		CComPtr<CSftpStream> pStream = pStream->Create(
			m_spSession, pszFilePath);
		CPPUNIT_ASSERT(pStream);

		return pStream;
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( CSftpStream_test );
