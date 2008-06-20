#pragma once

#include "stdafx.h"
#include "CppUnitExtensions.h"

// Redefine the 'private' keyword to inject a friend declaration for this 
// test class directly into the target class's header
class CPuttyWrapper_test;
#undef private
#define private \
	friend class CPuttyWrapper_test; private
#include "../PuttyProvider/PuttyWrapper.h"
#undef private

class CPuttyWrapper_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CPuttyWrapper_test );
		CPPUNIT_TEST( testGetSizeOfDataInPipe );
		CPPUNIT_TEST( testRead );
		CPPUNIT_TEST( testReadLine );
		CPPUNIT_TEST( testWrite );
		CPPUNIT_TEST( testRunLS );
	CPPUNIT_TEST_SUITE_END();

public:
	CPuttyWrapper_test();
	void setUp();
	void tearDown();

protected:
	void testRead();
	void testReadLine();
	void testWrite();
	void testGetSizeOfDataInPipe();
	void testRunLS();

private:
	CString _GetExePath() const;
	CString _GetHostName() const;
	CString _GetUserName() const;
	CString _GetPassword() const;
	void _HandlePasswordRequest(CString &strChunk);
	CPuttyWrapper *m_pPutty;
};
