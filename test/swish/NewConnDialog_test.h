#pragma once

#include "stdafx.h"
#include "../common/CppUnitExtensions.h"

// Redefine the 'private' keyword to inject a friend declaration for this 
// test class directly into the target class's header
class CNewConnDialog_test;
#undef private
#define private \
	friend class CNewConnDialog_test; private
#include <NewConnDialog.h>
#undef private

class CNewConnDialog_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CNewConnDialog_test );
		CPPUNIT_TEST( testGetUser );
		CPPUNIT_TEST( testGetHost );
		CPPUNIT_TEST( testGetPath );
		CPPUNIT_TEST( testGetPort );
		CPPUNIT_TEST( testDoModal );
	CPPUNIT_TEST_SUITE_END();

public:
	CNewConnDialog_test() {}
	~CNewConnDialog_test() {}
	void setUp();
	void tearDown();

protected:
	void testGetUser();
	void testGetHost();
	void testGetPath();
	void testGetPort();
	void testDoModal();

private:
	CNewConnDialog m_dlg;

	/* Thread related functions and variables */
	static DWORD WINAPI ClickCancelThread( __in LPVOID lpThreadParam );
};
