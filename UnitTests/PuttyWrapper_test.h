#pragma once

#include "stdafx.h"
#include <cppunit/extensions/HelperMacros.h>

// Redefine the 'private' keyword to inject a friend declaration for this 
// test class directly into the target class's header
class CPuttyWrapper_test;
#undef private
#define private \
	friend class CPuttyWrapper_test; private
#include "PuttyWrapper.h"
#undef private

class CPuttyWrapper_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CPuttyWrapper_test );
		CPPUNIT_TEST( testGetSizeOfDataInPipe );
		CPPUNIT_TEST( testRead );
		CPPUNIT_TEST( testWrite );
		CPPUNIT_TEST( testRunLS );
	CPPUNIT_TEST_SUITE_END();

public:
	CPuttyWrapper_test();
	void setUp();
	void tearDown();

protected:
	void testRead();
	void testWrite();
	void testGetSizeOfDataInPipe();
	void testRunLS();

private:
	CString GetExePath() const;
	CString GetHostName() const;
	CString GetUserName() const;
	CPuttyWrapper *m_pPutty;
};

CPPUNIT_NS_BEGIN
/**
 * Provides CPPUNIT_ASSERT_EQUAL capability for CStrings.
 *
 * @example
 * CString x = "bob"
 * CString y = "sally"
 * CPPUNIT_ASSERT_EQUAL( x, y )
 */
template <>
struct assertion_traits<CString>
{  
    static bool equal( CString x, CString y )
    {
        return x == y;
    }

    static std::string toString( CString x )
    {
		CStringA y(x);
		std::string s(y);
		return s;
    }
};
CPPUNIT_NS_END