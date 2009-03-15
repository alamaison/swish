#include "pch.h"
#include "Pidl_test.h"

#include <RemotePidl.h>

/** Standard PIDL-wrapper tests for CRemotePidl family */

template <>
void CPidl_test<CRemoteItem, ITEMID_CHILD>::_testGetNext(
	PidlType /*pidlTest*/, PCUIDLIST_RELATIVE pidlNext) throw(...)
{
	// GetNext on a child pidl should always return NULL
	CPPUNIT_ASSERT( pidlNext == NULL );
}

template <>
ITEMID_CHILD* CPidl_test<CRemoteItem, ITEMID_CHILD>::_setUp(
	PIDLIST_ABSOLUTE pidl)
{
	return ::ILCloneChild(::ILFindLastID(pidl));
}

template <>
ITEMIDLIST_RELATIVE* CPidl_test<CRemoteItemList, ITEMIDLIST_RELATIVE>::_setUp(
	PIDLIST_ABSOLUTE pidl)
{
	return ::ILClone(::ILGetNext(pidl));
}

template <>
ITEMIDLIST_ABSOLUTE* 
CPidl_test<CRemoteItemAbsolute, ITEMIDLIST_ABSOLUTE>::_setUp(
	PIDLIST_ABSOLUTE pidl)
{
	return ::ILCloneFull(pidl);
}

typedef CPidl_test<CRemoteItemList, ITEMIDLIST_RELATIVE> CRemoteItemList_test;
typedef CPidl_test<CRemoteItemAbsolute, ITEMIDLIST_ABSOLUTE> 
	CRemoteItemAbsolute_test;
typedef CPidl_test<CRemoteItem, ITEMID_CHILD> CRemoteItem_test;

CPPUNIT_TEST_SUITE_REGISTRATION( CRemoteItemList_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CRemoteItemAbsolute_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CRemoteItem_test );

class CRemotePidl_assignment_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CRemotePidl_assignment_test );
		CPPUNIT_TEST( testAssignment );
		CPPUNIT_TEST( testAssignment2 );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {}

	void tearDown() {}

	void testAssignment()
	{
		CRemoteItem pidlC;
		CRemoteItemList pidlR;
		CRemoteItemAbsolute pidlA;

		PITEMID_CHILD pidlItemC = NULL;
		PIDLIST_RELATIVE pidlItemR = NULL;
		PIDLIST_ABSOLUTE pidlItemA = NULL;

		// Upcast CRemotePidls
		pidlR = pidlC;
		pidlR = pidlA;
		pidlR = pidlItemC;
		pidlR = pidlItemA;
		pidlItemR = pidlC.CopyTo();
		pidlItemR = pidlA.CopyTo();

		// DownCast CRemotePidls
		pidlC = static_cast<PCITEMID_CHILD>((PCUIDLIST_RELATIVE)pidlR);
		pidlC = static_cast<PCITEMID_CHILD>(
		                    static_cast<PCUIDLIST_RELATIVE>(pidlR));

		// CrossCast CRemotePidls
		pidlA = static_cast<PCIDLIST_ABSOLUTE>((PCUIDLIST_RELATIVE)pidlC);
		pidlA = static_cast<PCIDLIST_ABSOLUTE>(
		                    static_cast<PCUIDLIST_RELATIVE>(pidlC));
	}

	void testAssignment2()
	{
		CRemoteItem pidlC;
		CRemoteItemList pidlR;
		CRemoteItemAbsolute pidlA;

		CRemoteItemHandle pidlHandC = NULL;
		CRemoteItemListHandle pidlHandR = NULL;
		CRemoteItemAbsoluteHandle pidlHandA = NULL;

		// Cast CRemotePidls to CRemotePidlHandles
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

CPPUNIT_TEST_SUITE_REGISTRATION( CRemotePidl_assignment_test );
