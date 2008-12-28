#include "stdafx.h"
#include "Pidl_test.h"

#include <HostPidl.h>

/** Standard PIDL-wrapper tests for CHostPidl family */

template <>
void CPidl_test<CHostItem, ITEMID_CHILD>::_testGetNext(
	PidlType /*pidlTest*/, PCUIDLIST_RELATIVE pidlNext) throw(...)
{
	// GetNext on a child pidl should always return NULL
	CPPUNIT_ASSERT( pidlNext == NULL );
}

template <>
ITEMID_CHILD* CPidl_test<CHostItem, ITEMID_CHILD>::_setUp(
	PIDLIST_ABSOLUTE pidl)
{
	return ::ILCloneChild(::ILFindLastID(pidl));
}

template <>
ITEMIDLIST_RELATIVE* CPidl_test<CHostItemList, ITEMIDLIST_RELATIVE>::_setUp(
	PIDLIST_ABSOLUTE pidl)
{
	return ::ILClone(::ILGetNext(pidl));
}

template <>
ITEMIDLIST_ABSOLUTE* 
CPidl_test<CHostItemAbsolute, ITEMIDLIST_ABSOLUTE>::_setUp(
	PIDLIST_ABSOLUTE pidl)
{
	return ::ILCloneFull(pidl);
}

typedef CPidl_test<CHostItemList, ITEMIDLIST_RELATIVE> CHostItemList_test;
typedef CPidl_test<CHostItemAbsolute, ITEMIDLIST_ABSOLUTE> 
	CHostItemAbsolute_test;
typedef CPidl_test<CHostItem, ITEMID_CHILD> CHostItem_test;

CPPUNIT_TEST_SUITE_REGISTRATION( CHostItemList_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CHostItemAbsolute_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CHostItem_test );

class CHostPidl_assignment_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CHostPidl_assignment_test );
		CPPUNIT_TEST( testAssignment );
		CPPUNIT_TEST( testAssignment2 );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {}

	void tearDown() {}

	void testAssignment()
	{
		CHostItem pidlC;
		CHostItemList pidlR;
		CHostItemAbsolute pidlA;

		PITEMID_CHILD pidlItemC = NULL;
		PIDLIST_RELATIVE pidlItemR = NULL;
		PIDLIST_ABSOLUTE pidlItemA = NULL;

		// Upcast CHostPidls
		pidlR = pidlC;
		pidlR = pidlA;
		pidlR = pidlItemC;
		pidlR = pidlItemA;
		pidlItemR = pidlC.CopyTo();
		pidlItemR = pidlA.CopyTo();

		// DownCast CHostPidls
		pidlC = static_cast<PCITEMID_CHILD>((PCUIDLIST_RELATIVE)pidlR);
		pidlC = static_cast<PCITEMID_CHILD>(
		                    static_cast<PCUIDLIST_RELATIVE>(pidlR));

		// CrossCast CHostPidls
		pidlA = static_cast<PCIDLIST_ABSOLUTE>((PCUIDLIST_RELATIVE)pidlC);
		pidlA = static_cast<PCIDLIST_ABSOLUTE>(
		                    static_cast<PCUIDLIST_RELATIVE>(pidlC));
	}

	void testAssignment2()
	{
		CHostItem pidlC;
		CHostItemList pidlR;
		CHostItemAbsolute pidlA;

		CHostItemHandle pidlHandC = NULL;
		CHostItemListHandle pidlHandR = NULL;
		CHostItemAbsoluteHandle pidlHandA = NULL;

		// Cast CHostPidls to CHostPidlHandles
		pidlR = pidlHandR;
		pidlR = pidlHandC;
		pidlR = pidlHandA;
		pidlHandR = pidlR;
		pidlHandR = pidlC;
		pidlHandR = pidlA;

		// Wrongcast - should not compile
		//pidlA = pidlHandR;
		//pidlA = pidlHandC;
		//pidlC = pidlHandR;
		//pidlC = pidlHandA;
		//pidlHandA = pidlR;
		//pidlHandA = pidlC;
		//pidlHandC = pidlR;
		//pidlHandC = pidlA;

		// Downcast
		pidlA = static_cast<PCIDLIST_ABSOLUTE>((PCUIDLIST_RELATIVE)pidlHandR);
		pidlA = static_cast<PCIDLIST_ABSOLUTE>((PCUIDLIST_RELATIVE)pidlHandC);
		pidlC = static_cast<PCITEMID_CHILD>((PCUIDLIST_RELATIVE)pidlHandR);
		pidlC = static_cast<PCITEMID_CHILD>((PCUIDLIST_RELATIVE)pidlHandA);
		pidlHandA = static_cast<PCIDLIST_ABSOLUTE>((PCUIDLIST_RELATIVE)pidlR);
		pidlHandA = static_cast<PCIDLIST_ABSOLUTE>((PCUIDLIST_RELATIVE)pidlC);
		pidlHandC = static_cast<PCITEMID_CHILD>((PCUIDLIST_RELATIVE)pidlR);
		pidlHandC = static_cast<PCITEMID_CHILD>((PCUIDLIST_RELATIVE)pidlA);
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION( CHostPidl_assignment_test );
