//PuttyProvider_Test.h    -   declares the class PuttyProvider_Test

#ifndef CPP_UNIT_PuttyProvider_TEST_H
#define CPP_UNIT_PuttyProvider_TEST_H

#include "stdafx.h"
#include "_Swish.h"
#include <cppunit/extensions/HelperMacros.h>

#define IPuttyProvider IPuttyProviderUnstable
#define IID_IPuttyProvider IID_IPuttyProviderUnstable

/* 
TestCase.h defines the class PuttyProvider_Test which is used to test class PuttyProvider. 
 */

class CPuttyProvider_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CPuttyProvider_test );
		CPPUNIT_TEST( testQueryInterface );
		CPPUNIT_TEST( testInitialize );
		CPPUNIT_TEST( testGetListing );
	CPPUNIT_TEST_SUITE_END();

public:
	CPuttyProvider_test() : m_pProvider(NULL) {}
	void setUp();
	void tearDown();

protected:
	void testQueryInterface();
	void testInitialize();
	void testGetListing();

private:
	IPuttyProvider *m_pProvider;

	void testRegistryStructure() const;
	CString GetHostName() const;
	CString GetUserName() const;
	USHORT GetPort() const;
};


#endif
