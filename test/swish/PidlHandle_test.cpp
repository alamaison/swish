#include "stdafx.h"
#include "../common/CppUnitExtensions.h"

#include <Pidl.h>

template<typename IdListType>
class CPidlHandle_test : public CPPUNIT_NS::TestFixture
{
	typedef IdListType *PidlType;
	typedef const IdListType *ConstPidlType;
	typedef CPidlBase< CPidlConstData<IdListType> > CPidlHandle;

	CPPUNIT_TEST_SUITE( CPidlHandle_test<IdListType> );
		CPPUNIT_TEST( testSizeof );
		CPPUNIT_TEST( testCPidlHandleDefault );
		CPPUNIT_TEST( testCPidlHandleDefaultNULL );
		CPPUNIT_TEST( testCPidlHandleFromPidl );
		CPPUNIT_TEST( testCPidlHandleFromPidlNULL );
		CPPUNIT_TEST( testCPidlHandleCopyAssignment );
		CPPUNIT_TEST( testCPidlHandleCopyAssignmentNULL );
		CPPUNIT_TEST( testCPidlHandleCopyAssignment2 );
		CPPUNIT_TEST( testCPidlHandleCopyAssignment2NULL );
		CPPUNIT_TEST( testCPidlHandleCopyConstruction );
		CPPUNIT_TEST( testCPidlHandleCopyConstructionNULL );
		CPPUNIT_TEST( testCopyTo );
		CPPUNIT_TEST( testCopyToNULL );
		CPPUNIT_TEST( testGetNext );
		CPPUNIT_TEST( testGetNextAtEnd );
		CPPUNIT_TEST( testGetNextNULL );
		CPPUNIT_TEST( testoperatorConstPidl );
		CPPUNIT_TEST( testoperatorConstPidlNULL );
		CPPUNIT_TEST( teststaticClone );
		CPPUNIT_TEST( teststaticCloneNULL );
	CPPUNIT_TEST_SUITE_END();

public:
	CPidlHandle_test() : m_pPidl(NULL), m_pidlOriginal(NULL)
	{
	}

	void setUp();

	void tearDown()
	{
		::ILFree(m_pidlOriginal);
		m_pidlOriginal = NULL;

		m_pPidl = NULL;
	}

protected:

	void testSizeof()
	{
		CPPUNIT_ASSERT_EQUAL( sizeof m_pidlOriginal, sizeof *m_pPidl );
	}

	void testCPidlHandleDefault()
	{
		m_pPidl = new CPidlHandle();
		CPPUNIT_ASSERT( m_pPidl );
		CPPUNIT_ASSERT( m_pPidl->m_pidl == NULL );
		delete m_pPidl;
		m_pPidl = NULL;
	}

	void testCPidlHandleDefaultNULL()
	{
		CPidlHandle pidl;
		CPPUNIT_ASSERT( pidl.m_pidl == NULL );
	}

	void testCPidlHandleFromPidl()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));
		CPPUNIT_ASSERT( !ILIsEmpty(pidlTest) );

		// This constructor should just create a handle to the original pidl
		m_pPidl = new CPidlHandle(pidlTest);
		CPPUNIT_ASSERT( m_pPidl->m_pidl );
		CPPUNIT_ASSERT_EQUAL(
			const_cast<ConstPidlType>(pidlTest), m_pPidl->m_pidl );

		// When we destroy it, the original PIDL should be unaffected
		delete m_pPidl;
		m_pPidl = NULL;
		CPPUNIT_ASSERT( pidlTest );
		CPPUNIT_ASSERT( ::ILGetSize(pidlTest) > 0 );
		CPPUNIT_ASSERT( !ILIsEmpty(pidlTest) );

		::ILFree(pidlTest);
	}

	void testCPidlHandleFromPidlNULL()
	{
		PidlType pidlNull = NULL;
		m_pPidl = new CPidlHandle(pidlNull);
		CPPUNIT_ASSERT( m_pPidl->m_pidl == NULL );
	}

	void testCPidlHandleCopyAssignment()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			CPidlHandle pidl(pidlTest);
			CPPUNIT_ASSERT_EQUAL( 
				const_cast<ConstPidlType>(pidlTest), pidl.m_pidl );

			// Assigning to another CPidlHandle should copy the PIDL pointer
			CPidlHandle pidlCopy;
			pidlCopy = pidl;
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest), pidlCopy.m_pidl );
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest),
				pidl.m_pidl ); // member unchanged
		}

		::ILFree(pidlTest);
	}

	void testCPidlHandleCopyAssignmentNULL()
	{
		CPidlHandle pidl;
		CPidlHandle pidlCopy;
		pidlCopy = pidl;
		CPPUNIT_ASSERT( pidl.m_pidl == NULL );
		CPPUNIT_ASSERT( pidlCopy.m_pidl == NULL );
	}

	void testCPidlHandleCopyAssignment2()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			// Assigning a PIDL to a CPidlHandle should set the member to
			// equal the original PIDL
			CPidlHandle pidl;
			pidl = pidlTest;
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest), pidl.m_pidl );
		}

		::ILFree(pidlTest);
	}

	void testCPidlHandleCopyAssignment2NULL()
	{		
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			CPidlHandle pidl(pidlTest);
			pidl = NULL;
			CPPUNIT_ASSERT( pidl.m_pidl == NULL );
		}

		::ILFree(pidlTest);
	}

	void testCPidlHandleCopyConstruction()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			CPidlHandle pidl(pidlTest);
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest), pidl.m_pidl );

			// Initialising from another CPidlHandle should clone contents of the old
			// CPidlHandle leaving its m_pidl untouched
			CPidlHandle pidlCopy = pidl;
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest), pidlCopy.m_pidl );
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest), 
				pidl.m_pidl ); // member unchanged
		}

		::ILFree(pidlTest);
	}

	void testCPidlHandleCopyConstructionNULL()
	{
		CPidlHandle pidl;
		CPidlHandle pidlCopy = pidl;
		CPPUNIT_ASSERT( pidlCopy.m_pidl == NULL );
	}

	void testCopyTo()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			CPidlHandle pidl(pidlTest);
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest), pidl.m_pidl );

			// CopyTo should give us back a copy of the original PIDL, 
			// leaving the CPidlHandle untouched
			PidlType pidlCopy = pidl.CopyTo();
			CPPUNIT_ASSERT( pidlCopy != pidlTest );
			CPPUNIT_ASSERT( ::ILIsEqual(
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidlCopy),
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidlTest) 
			));
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest),
				pidl.m_pidl ); // member unchanged
			::ILFree(pidlCopy);
		}

		::ILFree(pidlTest);
	}

	void testCopyToNULL()
	{
		CPidlHandle pidlNull;
		PidlType pidlDest = NULL;
		pidlDest = pidlNull.CopyTo();
		CPPUNIT_ASSERT( pidlDest == NULL );
	}

	void testGetNext()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			CPidlHandle pidl(pidlTest);
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest), pidl.m_pidl );

			// GetNext should give us back a const pointer to the next SHITEMID 
			// leaving its member unchanged
			PCUIDLIST_RELATIVE pidlNext = pidl.GetNext();
			CPPUNIT_ASSERT( pidlNext != pidlTest );
			CPPUNIT_ASSERT_EQUAL( 
				(void *)((BYTE *)(pidlTest)+(pidlTest->mkid.cb)),
				(void *)pidlNext);
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest), 
				pidl.m_pidl ); // member unchanged
			// Don't free pidlNext - it is still part of pidlTest
		}

		::ILFree(pidlTest);
	}

	void testGetNextNULL()
	{
		CPidlHandle pidlNull;
		PCUIDLIST_RELATIVE pidlDest = pidlNull.GetNext();
		CPPUNIT_ASSERT( pidlDest == NULL );
	}

	void testGetNextAtEnd()
	{
		// Make sure we are at the end of a PIDL
		// (this is not technically correct for an absolute PIDL)
		PidlType pidlTest = static_cast<PidlType>(
			::ILClone(::ILFindLastID(m_pidlOriginal)));
		CPPUNIT_ASSERT( pidlTest != NULL );

		{
			CPidlHandle pidl(pidlTest);
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest), pidl.m_pidl );

			// GetNext should give us back NULL leaving its member unchanged
			PCUIDLIST_RELATIVE pidlNext = pidl.GetNext();
			CPPUNIT_ASSERT( pidlNext == NULL );
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest), 
				pidl.m_pidl ); // member unchanged
			// Don't free pidlNext - it is still part of pidlTest
		}

		::ILFree(pidlTest);
	}

	void testoperatorConstPidl()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			CPidlHandle pidl(pidlTest);
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest), 
				pidl.m_pidl );

			// Assigning to const PIDL should just pass the contained PIDL as-is
			ConstPidlType pidlConst = pidl;
			CPPUNIT_ASSERT_EQUAL(
				const_cast<ConstPidlType>(pidlTest),
				pidlConst
			);
			CPPUNIT_ASSERT_EQUAL(  
				const_cast<ConstPidlType>(pidlTest), 
				pidl.m_pidl ); // member unchanged
		}

		::ILFree(pidlTest);
	}

	void testoperatorConstPidlNULL()
	{
		// Test that assigning a NULL CPidlHandle to a constant PIDL should not fail
		CPidlHandle pidlNull;
		PidlType pidlDest = NULL;
		CPPUNIT_ASSERT( pidlDest == NULL );
	}

	void teststaticClone()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		// Test that cloning copies PIDL successfully
		PidlType pidlClone = CPidlHandle::Clone(pidlTest);

		CPPUNIT_ASSERT( pidlClone != pidlTest );
		CPPUNIT_ASSERT( ::ILIsEqual(
			reinterpret_cast<PIDLIST_ABSOLUTE>(pidlClone),
			reinterpret_cast<PIDLIST_ABSOLUTE>(pidlTest) 
		));

		::ILFree(pidlClone);

		::ILFree(pidlTest);
	}

	void teststaticCloneNULL()
	{
		// Test cloning a NULL pidl
		PidlType pidlNull = NULL;
		PidlType pidl = CPidlHandle::Clone(pidlNull);
		CPPUNIT_ASSERT( pidl == NULL );
	}


private:
	CPidlHandle *m_pPidl;

	PidlType m_pidlOriginal;
};

template <>
inline void CPidlHandle_test<ITEMID_CHILD>::testGetNext()
{
	PITEMID_CHILD pidlTest = ::ILCloneChild(m_pidlOriginal);

	{
		CPidlBase< CPidlConstData<ITEMID_CHILD> > pidl(pidlTest);
		CPPUNIT_ASSERT_EQUAL( const_cast<PCITEMID_CHILD>(pidlTest), pidl.m_pidl );

		// GetNext on a child pidl should always return NULL
		PCUIDLIST_RELATIVE pidlNext = pidl.GetNext();
		CPPUNIT_ASSERT( pidlNext == NULL );
		CPPUNIT_ASSERT_EQUAL( const_cast<PCITEMID_CHILD>(pidlTest), pidl.m_pidl ); // member unchanged
	}

	::ILFree(pidlTest);
}


#define PIDLPATH L"C:\\Windows\\System32\\notepad.exe"

template <>
inline void CPidlHandle_test<ITEMID_CHILD>::setUp()
{
	PIDLIST_ABSOLUTE pidl = ::ILCreateFromPath(PIDLPATH);
	CPPUNIT_ASSERT(!::ILIsEmpty(pidl));

	m_pidlOriginal = ::ILCloneChild(::ILFindLastID(pidl));
	::ILFree(pidl);
	CPPUNIT_ASSERT( m_pidlOriginal );
}

template <>
inline void CPidlHandle_test<ITEMIDLIST_RELATIVE>::setUp()
{
	PIDLIST_ABSOLUTE pidl = ::ILCreateFromPath(PIDLPATH);
	CPPUNIT_ASSERT(!::ILIsEmpty(pidl));

	m_pidlOriginal = ::ILClone(::ILGetNext(pidl));
	::ILFree(pidl);
	CPPUNIT_ASSERT( m_pidlOriginal );
}

template <>
inline void CPidlHandle_test<ITEMIDLIST_ABSOLUTE>::setUp()
{
	PIDLIST_ABSOLUTE pidl = ::ILCreateFromPath(PIDLPATH);
	CPPUNIT_ASSERT(!::ILIsEmpty(pidl));

	m_pidlOriginal = ::ILCloneFull(pidl);
	::ILFree(pidl);
	CPPUNIT_ASSERT( m_pidlOriginal );
}

typedef CPidlHandle_test<ITEMIDLIST_RELATIVE> CRelativePidlHandle_test;
typedef CPidlHandle_test<ITEMIDLIST_ABSOLUTE> CAbsolutePidlHandle_test;
typedef CPidlHandle_test<ITEMID_CHILD> CChildPidlHandle_test;

CPPUNIT_TEST_SUITE_REGISTRATION( CRelativePidlHandle_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CAbsolutePidlHandle_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CChildPidlHandle_test );


class CPidlHandle_assignment_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CPidlHandle_assignment_test );
		CPPUNIT_TEST( testAssignment );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {}

	void tearDown() {}

	void testAssignment()
	{
		CChildPidlHandle pidlC;
		CRelativePidlHandle pidlR;
		CAbsolutePidlHandle pidlA;

		PITEMID_CHILD pidlItemC = NULL;
		PIDLIST_RELATIVE pidlItemR = NULL;
		PIDLIST_ABSOLUTE pidlItemA = NULL;

		// Upcast CPidls
		pidlR = pidlC;
		pidlR = pidlA;
		pidlR = pidlItemC;
		pidlR = pidlItemA;
		pidlItemR = pidlC.CopyTo();
		pidlItemR = pidlA.CopyTo();

		// DownCast CPidls
		pidlC = static_cast<PCITEMID_CHILD>((PCUIDLIST_RELATIVE)pidlR);
		pidlC = static_cast<PCITEMID_CHILD>(
		                    static_cast<PCUIDLIST_RELATIVE>(pidlR));

		// CrossCast CPidls
		pidlA = static_cast<PCIDLIST_ABSOLUTE>((PCUIDLIST_RELATIVE)pidlC);
		pidlA = static_cast<PCIDLIST_ABSOLUTE>(
		                    static_cast<PCUIDLIST_RELATIVE>(pidlC));
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION( CPidlHandle_assignment_test );