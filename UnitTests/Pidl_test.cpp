#include "stdafx.h"
#include "CppUnitExtensions.h"

// Redefine the 'protected' keyword to inject a friend declaration for this 
// test class directly into the target class's header
//template<typename PidlType> class CPidl_test;
//#undef protected
//#define protected \
//	friend class CPidl_test; protected
#include "../Pidl.h"
//#undef protected

template<typename PidlType>
class CPidl_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CPidl_test<PidlType> );
		CPPUNIT_TEST( testCPidlDefault );
		CPPUNIT_TEST( testCPidlDefaultNULL );
		CPPUNIT_TEST( testCPidlFromPidl );
		CPPUNIT_TEST( testCPidlFromPidlNULL );
		CPPUNIT_TEST( testCPidlCopyAssignment );
		CPPUNIT_TEST( testCPidlCopyAssignmentNULL );
		CPPUNIT_TEST( testCPidlCopyConstruction );
		CPPUNIT_TEST( testCPidlCopyConstructionNULL );
		CPPUNIT_TEST( testAttach1 );
		CPPUNIT_TEST( testAttach2 );
		CPPUNIT_TEST( testAttach3 );
		CPPUNIT_TEST( testCopyFrom );
		CPPUNIT_TEST( testCopyFromNULL );
		CPPUNIT_TEST( testDetach );
		CPPUNIT_TEST( testDetachNULL );
		CPPUNIT_TEST( testCopyTo );
		CPPUNIT_TEST( testCopyToNULL );
		CPPUNIT_TEST( testoperatorConstPidl );
		CPPUNIT_TEST( testoperatorConstPidlNULL );
		CPPUNIT_TEST( teststaticClone );
		CPPUNIT_TEST( teststaticCloneNULL );
	CPPUNIT_TEST_SUITE_END();

public:
	CPidl_test() : m_pPidl(NULL), m_pidlOriginal(NULL)
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

	void testCPidlDefault()
	{
		m_pPidl = new CPidl<PidlType>();
		CPPUNIT_ASSERT( m_pPidl );
		CPPUNIT_ASSERT( m_pPidl->m_pidl == NULL );
		delete m_pPidl;
		m_pPidl = NULL;
	}

	void testCPidlDefaultNULL()
	{
		CPidl<PidlType> pidl;
		CPPUNIT_ASSERT( pidl.m_pidl == NULL );
	}

	void testCPidlFromPidl()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));
		CPPUNIT_ASSERT( !ILIsEmpty(pidlTest) );

		// This constructor should make a copy of the PIDL and NOT 
		// take ownership
		m_pPidl = new CPidl<PidlType>(pidlTest);
		CPPUNIT_ASSERT( m_pPidl->m_pidl );
		CPPUNIT_ASSERT( m_pPidl->m_pidl != pidlTest );
		// XXX: This is a bit dodgy.  Why does ILIsEqual need absolute PIDLs?
		// reinterpret_cast needed for PITEMID_CHILD -> PIDLIST_ABSOLUTE
		CPPUNIT_ASSERT( ::ILIsEqual(
			reinterpret_cast<PIDLIST_ABSOLUTE>(m_pPidl->m_pidl),
			reinterpret_cast<PIDLIST_ABSOLUTE>(pidlTest) 
		));

		// So when we destroy it, the original PIDL should be unaffected
		delete m_pPidl;
		m_pPidl = NULL;
		CPPUNIT_ASSERT( pidlTest );
		CPPUNIT_ASSERT( ::ILGetSize(pidlTest) > 0 );
		CPPUNIT_ASSERT( !ILIsEmpty(pidlTest) );

		::ILFree(pidlTest);
	}

	void testCPidlFromPidlNULL()
	{
		PidlType pidlNull = NULL;
		m_pPidl = new CPidl<PidlType>(pidlNull);
		CPPUNIT_ASSERT( m_pPidl->m_pidl == NULL );
	}

	void testCPidlCopyAssignment()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			CPidl<PidlType> pidl;
			pidl.Attach(pidlTest);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl );

			// Assigning to another CPidl should clone contents of the old
			// CPidl leaving its m_pidl untouched
			CPidl<PidlType> pidlCopy;
			pidlCopy = pidl;
			CPPUNIT_ASSERT( pidlCopy.m_pidl != pidlTest );
			CPPUNIT_ASSERT( ::ILIsEqual(
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidlCopy.m_pidl),
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidlTest) 
			));
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl ); // member unchanged
		}

		// Don't ILFree pidlTest - it is destroyed when the CPidl dies
	}

	void testCPidlCopyAssignmentNULL()
	{
		CPidl<PidlType> pidl;
		CPidl<PidlType> pidlCopy;
		pidlCopy = pidl;
		CPPUNIT_ASSERT( pidlCopy.m_pidl == NULL );
	}

	void testCPidlCopyConstruction()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			CPidl<PidlType> pidl;
			pidl.Attach(pidlTest);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl );

			// Initialising from another CPidl should clone contents of the old
			// CPidl leaving its m_pidl untouched
			CPidl<PidlType> pidlCopy = pidl;
			CPPUNIT_ASSERT( pidlCopy.m_pidl != pidlTest );
			CPPUNIT_ASSERT( ::ILIsEqual(
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidlCopy.m_pidl),
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidlTest) 
			));
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl ); // member unchanged
		}

		// Don't ILFree pidlTest - it is destroyed when the CPidl dies
	}

	void testCPidlCopyConstructionNULL()
	{
		CPidl<PidlType> pidl;
		CPidl<PidlType> pidlCopy = pidl;
		CPPUNIT_ASSERT( pidlCopy.m_pidl == NULL );
	}

	void testAttach1()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		// Attach should take over ownership of the PIDL
		m_pPidl = new CPidl<PidlType>();
		m_pPidl->Attach(pidlTest);
		CPPUNIT_ASSERT_EQUAL( pidlTest, m_pPidl->m_pidl );

		// So when we destroy it, the original PIDL should no longer 
		// point to valid memory
		delete m_pPidl;
		m_pPidl = NULL;
		CPPUNIT_ASSERT( pidlTest );
		// Ideally we should check that it was freed here (how?)

		// Don't ILFree pidlTest
	}

	void testAttach2()
	{
		// Create an instance by taking ownership of a PIDL
		PidlType pidlFirst = static_cast<PidlType>(::ILClone(m_pidlOriginal));
		m_pPidl = new CPidl<PidlType>();
		m_pPidl->Attach(pidlFirst);

		// Attach should take ownership of the second PIDL and should destroy
		// the first PIDL
		PidlType pidlSecond = static_cast<PidlType>(::ILClone(m_pidlOriginal));
		m_pPidl->Attach(pidlSecond);
		CPPUNIT_ASSERT_EQUAL( pidlSecond, m_pPidl->m_pidl );
		CPPUNIT_ASSERT( pidlFirst );
		// Ideally we should check that it was freed here (how?)

		// When we destroy the CPidl, the second PIDL should also no 
		// longer point to valid memory
		delete m_pPidl;
		m_pPidl = NULL;
		CPPUNIT_ASSERT( pidlSecond );
		// Ideally we should check that it was freed here (how?)

		// Don't ILFree pidlFirst or pidlSecond
	}

	void testAttach3()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		// Create an instance by taking ownership of a PIDL
		CPidl<PidlType> pidl;
		pidl.Attach(pidlTest);
		CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl );

		// Attaching NULL should destroy the original PIDL and reset the CPidl
		PidlType pidlNull = NULL;
		pidl.Attach(pidlNull);
		CPPUNIT_ASSERT( pidlTest );
		// Ideally we should check that it was freed here (how?)

		// Don't ILFree pidlTest
	}

	void testCopyFrom()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		// CopyFrom should copy the PIDL into ourselves but NOT take ownership
		m_pPidl = new CPidl<PidlType>();
		m_pPidl->CopyFrom(pidlTest);
		CPPUNIT_ASSERT( m_pPidl->m_pidl != pidlTest );
		CPPUNIT_ASSERT( ::ILIsEqual(
			reinterpret_cast<PIDLIST_ABSOLUTE>(m_pPidl->m_pidl),
			reinterpret_cast<PIDLIST_ABSOLUTE>(pidlTest) 
		));

		// So when we destroy it, the original PIDL should be unaffected
		delete m_pPidl;
		m_pPidl = NULL;
		CPPUNIT_ASSERT( pidlTest );
		CPPUNIT_ASSERT( ::ILGetSize(pidlTest) > 0 );
		CPPUNIT_ASSERT( !ILIsEmpty(pidlTest) );

		::ILFree(pidlTest);
	}

	void testCopyFromNULL()
	{
		PidlType pidlNull = NULL;
		m_pPidl = new CPidl<PidlType>();
		m_pPidl->CopyFrom(pidlNull);
		CPPUNIT_ASSERT( m_pPidl->m_pidl == NULL );
	}

	void testDetach()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		m_pPidl = new CPidl<PidlType>();
		m_pPidl->Attach(pidlTest);
		CPPUNIT_ASSERT_EQUAL( pidlTest, m_pPidl->m_pidl );

		// Detaching the pointer should give us back the original and blank 
		// the CPidl
		PidlType pidl = m_pPidl->Detach();
		CPPUNIT_ASSERT_EQUAL( pidlTest, pidl );
		CPPUNIT_ASSERT( m_pPidl->m_pidl == NULL );
		
		delete m_pPidl;
		m_pPidl = NULL;

		::ILFree(pidlTest);
	}

	void testDetachNULL()
	{
		// Test that detaching a NULL PIDL should not fail
		CPidl<PidlType> pidlNull;
		pidlNull.Detach();
	}

	void testCopyTo()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			CPidl<PidlType> pidl;
			pidl.Attach(pidlTest);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl );

			// CopyTo should give us back a copy of the original PIDL, 
			// leaving the CPidl untouched
			PidlType pidlCopy = pidl.CopyTo();
			CPPUNIT_ASSERT( pidlCopy != pidlTest );
			CPPUNIT_ASSERT( ::ILIsEqual(
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidlCopy),
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidlTest) 
			));
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl ); // member unchanged
			::ILFree(pidlCopy);
		}

		// Don't ILFree pidlTest - it is destroyed when the CPidl dies
	}

	void testCopyToNULL()
	{
		CPidl<PidlType> pidlNull;
		PidlType pidlDest = NULL;
		pidlDest = pidlNull.CopyTo();
	}

	void testoperatorConstPidl()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			CPidl<PidlType> pidl;
			pidl.Attach(pidlTest);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl );

			// Assigning to const PIDL should just pass the contained PIDL as-is
			PidlType pidlConst = pidl;
			CPPUNIT_ASSERT_EQUAL(
				const_cast<const PidlType>(pidlTest),
				pidlConst
			);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl ); // member unchanged
		}

		// Don't ILFree pidlTest - it is destroyed when the CPidl dies
	}

	void testoperatorConstPidlNULL()
	{
		// Test that assigning a NULL CPidl to a constant PIDL should not fail
		CPidl<PidlType> pidlNull;
		PidlType pidlDest = NULL;
		CPPUNIT_ASSERT( pidlDest == NULL );
	}

	void teststaticClone()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		// Test that cloning copies PIDL successfully
		PidlType pidlClone = CPidl<PidlType>::Clone(pidlTest);

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
		PidlType pidl = CPidl<PidlType>::Clone(pidlNull);
		CPPUNIT_ASSERT( pidl == NULL );
	}


private:
	CPidl<PidlType> *m_pPidl;

	PidlType m_pidlOriginal;
};

template <>
void CPidl_test<PITEMID_CHILD>::setUp()
{
	HRESULT hr;
	PIDLIST_ABSOLUTE pidl;
	hr = ::SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl);
	CPPUNIT_ASSERT_OK(hr);

	m_pidlOriginal = ::ILCloneChild(::ILFindLastID(pidl));
	::ILFree(pidl);
	CPPUNIT_ASSERT( m_pidlOriginal );
}

template <>
void CPidl_test<PIDLIST_RELATIVE>::setUp()
{
	HRESULT hr;
	PIDLIST_ABSOLUTE pidl;
	hr = ::SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
	CPPUNIT_ASSERT_OK(hr);

	m_pidlOriginal = ::ILClone(::ILGetNext(pidl));
	::ILFree(pidl);
	CPPUNIT_ASSERT( m_pidlOriginal );
}

template <>
void CPidl_test<PIDLIST_ABSOLUTE>::setUp()
{
	HRESULT hr;
	hr = ::SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &m_pidlOriginal);
	CPPUNIT_ASSERT_OK(hr);
}

typedef CPidl_test<PIDLIST_RELATIVE> CRelativePidl_test;
typedef CPidl_test<PIDLIST_ABSOLUTE> CAbsolutePidl_test;
typedef CPidl_test<PITEMID_CHILD> CChildPidl_test;

CPPUNIT_TEST_SUITE_REGISTRATION( CRelativePidl_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CAbsolutePidl_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CChildPidl_test );
