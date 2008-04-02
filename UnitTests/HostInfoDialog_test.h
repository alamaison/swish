#pragma once

#include "stdafx.h"
#include <cppunit/extensions/HelperMacros.h>

// Redefine the 'private' keyword to inject a friend declaration for this 
// test class directly into the target class's header
class CHostInfoDialog_test;
#undef private
#define private \
	friend class CHostInfoDialog_test; private
#include "HostInfoDialog.h"
#undef private

class CHostInfoDialog_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CHostInfoDialog_test );
		CPPUNIT_TEST( testGetUser );
		CPPUNIT_TEST( testGetHost );
		CPPUNIT_TEST( testGetPath );
		CPPUNIT_TEST( testGetPort );
		CPPUNIT_TEST( testDoModal );
	CPPUNIT_TEST_SUITE_END();

public:
	CHostInfoDialog_test() {}
	~CHostInfoDialog_test() {}
	void setUp();
	void tearDown();

protected:
	void testGetUser();
	void testGetHost();
	void testGetPath();
	void testGetPort();
	void testDoModal();

private:
	CHostInfoDialog m_dlg;

	/* Thread related functions and variables */
	static DWORD WINAPI ClickCancelThread( __in LPVOID lpThreadParam );
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