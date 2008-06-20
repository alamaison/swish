#pragma once

#include "stdafx.h"
#include "CppUnitExtensions.h"

// Redefine the 'private' keyword to inject a friend declaration for this 
// test class directly into the target class's header
class CHostInfoDialog_test;
#undef private
#define private \
	friend class CHostInfoDialog_test; private
#include "../HostInfoDialog.h"
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
