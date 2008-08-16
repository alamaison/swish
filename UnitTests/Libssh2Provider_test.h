//Libssh2Provider_test.h    -   declares the class Libssh2Provider_test

#ifndef CPP_UNIT_Libssh2Provider_TEST_H
#define CPP_UNIT_Libssh2Provider_TEST_H

#include "stdafx.h"
#include "CppUnitExtensions.h"
#include "MockSftpConsumer.h"

// Libssh2Provider CLibssh2Provider component
#import "progid:Libssh2Provider.Libssh2Provider" raw_interfaces_only, raw_native_types, auto_search

struct testFILEDATA
{
	BOOL fIsFolder;
	CString strPath;
	CString strOwner;
	CString strGroup;
	CString strAuthor;

	ULONGLONG uSize; // 64-bit allows files up to 16 Ebibytes (a lot)
	time_t dtModified;
	DWORD dwPermissions;
};

class CLibssh2Provider_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CLibssh2Provider_test );
		CPPUNIT_TEST( testQueryInterface );
		CPPUNIT_TEST( testInitialize );
		CPPUNIT_TEST( testGetListing );
	CPPUNIT_TEST_SUITE_END();

public:
	CLibssh2Provider_test() : m_pProvider(NULL), m_pConsumer(NULL) {}
	void setUp();
	void tearDown();

protected:
	void testQueryInterface();
	void testInitialize();
	void testGetListing();
	void testGetListing_WrongPassword();

private:
	CComObject<CMockSftpConsumer> *m_pCoConsumer;
	Swish::ISftpConsumer *m_pConsumer;
	Swish::ISftpProvider *m_pProvider;

	void CreateMockSftpConsumer(
		__out CComObject<CMockSftpConsumer> **ppCoConsumer,
		__out Swish::ISftpConsumer **ppConsumer
	) const;
	void testListingFormat(__in Swish::IEnumListing *pEnum) const;
	void testRegistryStructure() const;
	CString GetHostName() const;
	CString GetUserName() const;
	USHORT GetPort() const;
	CString GetPassword() const;
};

#endif
