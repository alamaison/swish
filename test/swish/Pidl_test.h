#include "pch.h"
#include "../common/CppUnitExtensions.h"

#include <Pidl.h>

/**
 * Templated class for standard tests on non-const PIDL wrappers.
 */
template<typename PidlT, typename IdListType>
class CPidl_test : public CPPUNIT_NS::TestFixture
{
	typedef IdListType *PidlType;
	typedef const IdListType *ConstPidlType;
	typedef CPidl_test<PidlT, IdListType> TestSuite;

	CPPUNIT_TEST_SUITE( TestSuite );
		CPPUNIT_TEST( testSizeof );
		CPPUNIT_TEST( testPidlDefault );
		CPPUNIT_TEST( testPidlDefaultNULL );
		CPPUNIT_TEST( testPidlFromPidl );
		CPPUNIT_TEST( testPidlFromPidlNULL );
		CPPUNIT_TEST( testPidlCopyAssignment );
		CPPUNIT_TEST( testPidlCopyAssignmentNULL );
		CPPUNIT_TEST( testPidlCopyAssignment2 );
		CPPUNIT_TEST( testPidlCopyAssignment2NULL );
		CPPUNIT_TEST( testPidlCopyConstruction );
		CPPUNIT_TEST( testPidlCopyConstructionNULL );
		CPPUNIT_TEST( testAttach1 );
		CPPUNIT_TEST( testAttach2 );
		CPPUNIT_TEST( testAttach3 );
		CPPUNIT_TEST( testCopyFrom );
		CPPUNIT_TEST( testCopyFromNULL );
		CPPUNIT_TEST( testDetach );
		CPPUNIT_TEST( testDetachNULL );
		CPPUNIT_TEST( testCopyTo );
		CPPUNIT_TEST( testCopyToNULL );
		CPPUNIT_TEST( testAppend );
		CPPUNIT_TEST( testAppendNULL );
		CPPUNIT_TEST( testAppendNULL2 );
		CPPUNIT_TEST( testAppendNULL3 );
		CPPUNIT_TEST( testGetNext );
		CPPUNIT_TEST( testGetNextAtEnd );
		CPPUNIT_TEST( testGetNextNULL );
		CPPUNIT_TEST( testoperatorConstPidl );
		CPPUNIT_TEST( testoperatorConstPidlNULL );
		CPPUNIT_TEST( teststaticClone );
		CPPUNIT_TEST( teststaticCloneNULL );
	CPPUNIT_TEST_SUITE_END();

public:
	CPidl_test() : m_pPidl(NULL), m_pidlOriginal(NULL)
	{
	}

#define PIDLPATH L"C:\\Windows\\System32\\notepad.exe"

	void setUp()
	{
		PIDLIST_ABSOLUTE pidl = ::ILCreateFromPath(PIDLPATH);
		CPPUNIT_ASSERT(!::ILIsEmpty(pidl));

		m_pidlOriginal = _setUp(pidl);
		::ILFree(pidl);
		CPPUNIT_ASSERT( m_pidlOriginal );
	}

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

	void testPidlDefault()
	{
		m_pPidl = new PidlT();
		CPPUNIT_ASSERT( m_pPidl );
		CPPUNIT_ASSERT( m_pPidl->m_pidl == NULL );
		delete m_pPidl;
		m_pPidl = NULL;
	}

	void testPidlDefaultNULL()
	{
		PidlT pidl;
		CPPUNIT_ASSERT( pidl.m_pidl == NULL );
	}

	void testPidlFromPidl()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));
		CPPUNIT_ASSERT( !ILIsEmpty(pidlTest) );

		// This constructor should make a copy of the PIDL and NOT 
		// take ownership
		m_pPidl = new PidlT(pidlTest);
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

	void testPidlFromPidlNULL()
	{
		PidlType pidlNull = NULL;
		m_pPidl = new PidlT(pidlNull);
		CPPUNIT_ASSERT( m_pPidl->m_pidl == NULL );
	}

	void testPidlCopyAssignment()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			PidlT pidl;
			pidl.Attach(pidlTest);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl );

			// Assigning to another CPidl should clone contents of the old
			// CPidl leaving its m_pidl untouched
			PidlT pidlCopy;
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

	void testPidlCopyAssignmentNULL()
	{
		PidlT pidl;
		PidlT pidlCopy;
		pidlCopy = pidl;
		CPPUNIT_ASSERT( pidlCopy.m_pidl == NULL );
	}

	void testPidlCopyAssignment2()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			// Assigning a PIDL to a CPidl should clone contents of the
			// old PIDL leaving it untouched
			PidlT pidl;
			pidl = pidlTest;
			CPPUNIT_ASSERT( pidl.m_pidl != pidlTest );
			CPPUNIT_ASSERT( ::ILIsEqual(
				reinterpret_cast<PCIDLIST_ABSOLUTE>(pidl.m_pidl),
				reinterpret_cast<PCIDLIST_ABSOLUTE>(pidlTest) 
			));
		}

		::ILFree(pidlTest);
	}

	void testPidlCopyAssignment2NULL()
	{		
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			PidlT pidl;
			pidl.Attach(pidlTest);
			pidl = NULL; // Should destroy pidlTest
			CPPUNIT_ASSERT( pidl.m_pidl == NULL );
		}

		// Don't ILFree pidlTest - it is destroyed when we assign NULL to pidl
	}

	void testPidlCopyConstruction()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			PidlT pidl;
			pidl.Attach(pidlTest);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl );

			// Initialising from another CPidl should clone contents of the old
			// CPidl leaving its m_pidl untouched
			PidlT pidlCopy = pidl;
			CPPUNIT_ASSERT( pidlCopy.m_pidl != pidlTest );
			CPPUNIT_ASSERT( ::ILIsEqual(
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidlCopy.m_pidl),
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidlTest) 
			));
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl ); // member unchanged
		}

		// Don't ILFree pidlTest - it is destroyed when the CPidl dies
	}

	void testPidlCopyConstructionNULL()
	{
		PidlT pidl;
		PidlT pidlCopy = pidl;
		CPPUNIT_ASSERT( pidlCopy.m_pidl == NULL );
	}

	void testAttach1()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		// Attach should take over ownership of the PIDL
		m_pPidl = new PidlT();
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
		m_pPidl = new PidlT();
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
		PidlT pidl;
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
		m_pPidl = new PidlT();
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
		m_pPidl = new PidlT();
		m_pPidl->CopyFrom(pidlNull);
		CPPUNIT_ASSERT( m_pPidl->m_pidl == NULL );
	}

	void testDetach()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		m_pPidl = new PidlT();
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
		PidlT pidlNull;
		pidlNull.Detach();
	}

	void testCopyTo()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			PidlT pidl;
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
		PidlT pidlNull;
		PidlType pidlDest = NULL;
		pidlDest = pidlNull.CopyTo();
		CPPUNIT_ASSERT( pidlDest == NULL );
	}

	void testAppend()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));
		PITEMID_CHILD pidlChild = 
			::ILCloneChild(::ILFindLastID(m_pidlOriginal));
		CPPUNIT_ASSERT(::ILRemoveLastID(pidlTest));
		PIDLIST_ABSOLUTE pidlRecombined = ::ILCombine(
			reinterpret_cast<PIDLIST_ABSOLUTE>(pidlTest), pidlChild);

		{
			PidlT pidl;
			pidl.Attach(pidlTest);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl );

			// Append should replace the CPidl's member with a PIDL consisting of
			// the original and the second pidl appended
			pidl.Append(pidlChild);

			CPPUNIT_ASSERT( pidlTest != pidl.m_pidl ); // member changed
			CPPUNIT_ASSERT( ::ILIsEqual(
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidlRecombined),
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidl.m_pidl) 
			));
			::ILFree(pidlChild);
			::ILFree(pidlRecombined);
		}

		// Don't ILFree pidlTest - it is destroyed when the CPidl dies
	}

	void testAppendNULL()
	{
		// Appending NULL pidl to NULL pidl
		PidlT pidlNull;
		PidlType pidlDest = NULL;
		pidlNull.Append(pidlDest);
		CPPUNIT_ASSERT( pidlNull.m_pidl == NULL );

		// Appending non-NULL pidl to NULL pidl
		PidlType pidlRelative = reinterpret_cast<PidlType>(
			::ILCloneChild(::ILFindLastID(m_pidlOriginal)));
		PidlT pidl;
		pidl.Append(pidlRelative);
		CPPUNIT_ASSERT( ::ILIsEqual(
			reinterpret_cast<PIDLIST_ABSOLUTE>(pidl.m_pidl),
			reinterpret_cast<PIDLIST_ABSOLUTE>(pidlRelative) 
		));
		::ILFree(pidlRelative);
	}

	void testAppendNULL2()
	{
		PidlType pidlNULL = NULL;

		// Appending NULL pidl to non-NULL pidl
		PidlType pidlTest = reinterpret_cast<PidlType>(
			::ILCloneChild(::ILFindLastID(m_pidlOriginal)));
		PidlT pidl;
		pidl.Attach(pidlTest);
		pidl.Append(pidlNULL);
		CPPUNIT_ASSERT_EQUAL(pidlTest, (PidlType)pidl.m_pidl);
	}

	void testAppendNULL3()
	{
		PITEMID_CHILD pidlTerm = reinterpret_cast<PITEMID_CHILD>(
			::ILClone(::ILNext(::ILFindLastID(m_pidlOriginal))));
		CPPUNIT_ASSERT_EQUAL((USHORT)0, pidlTerm->mkid.cb);

		// Appending terminating pidl to non-NULL pidl
		PidlType pidlTest = reinterpret_cast<PidlType>(
			::ILCloneChild(::ILFindLastID(m_pidlOriginal)));
		PidlT pidl;
		pidl.Attach(pidlTest);
		pidl.Append(pidlTerm);
		::ILFree(pidlTerm);
		CPPUNIT_ASSERT_EQUAL(pidlTest, (PidlType)pidl.m_pidl);
	}

	void testGetNext()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			PidlT pidl;
			pidl.Attach(pidlTest);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl );

			// GetNext should give us back a const pointer to the next SHITEMID 
			// leaving its member unchanged
			PCUIDLIST_RELATIVE pidlNext = pidl.GetNext();
			_testGetNext(pidlTest, pidlNext);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl ); // member unchanged
			// Don't free pidlNext - it is still part of pidlTest
		}

		// Don't ILFree pidlTest - it is destroyed when the CPidl dies
	}

	void testGetNextNULL()
	{
		PidlT pidlNull;
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
			PidlT pidl;
			pidl.Attach(pidlTest);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl );

			// GetNext should give us back NULL leaving its member unchanged
			PCUIDLIST_RELATIVE pidlNext = pidl.GetNext();
			CPPUNIT_ASSERT( pidlNext == NULL );
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl ); // member unchanged
			// Don't free pidlNext - it is still part of pidlTest
		}

		// Don't ILFree pidlTest - it is destroyed when the CPidl dies
	}

	void testoperatorConstPidl()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		{
			PidlT pidl;
			pidl.Attach(pidlTest);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl );

			// Assigning to const PIDL should just pass the contained PIDL as-is
			ConstPidlType pidlConst = pidl;
			CPPUNIT_ASSERT_EQUAL(
				const_cast<ConstPidlType>(pidlTest),
				pidlConst
			);
			CPPUNIT_ASSERT_EQUAL( pidlTest, pidl.m_pidl ); // member unchanged
		}

		// Don't ILFree pidlTest - it is destroyed when the CPidl dies
	}

	void testoperatorConstPidlNULL()
	{
		// Test that assigning a NULL CPidl to a constant PIDL should not fail
		PidlT pidlNull;
		PidlType pidlDest = NULL;
		CPPUNIT_ASSERT( pidlDest == NULL );
	}

	void teststaticClone()
	{
		PidlType pidlTest = static_cast<PidlType>(::ILClone(m_pidlOriginal));

		// Test that cloning copies PIDL successfully
		PidlType pidlClone = PidlT::Clone(pidlTest);

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
		PidlType pidl = PidlT::Clone(pidlNull);
		CPPUNIT_ASSERT( pidl == NULL );
	}

private:
	// Specialisations
	inline PidlType _setUp(PIDLIST_ABSOLUTE pidl);
	inline void _testGetNext(
		PidlType pidlTest, PCUIDLIST_RELATIVE pidlNext) throw(...)
	{
		// GetNext should give us back a const pointer to the next SHITEMID 
		// leaving its member unchanged
		CPPUNIT_ASSERT( pidlNext != pidlTest );
		CPPUNIT_ASSERT_EQUAL( 
			(void *)((BYTE *)(pidlTest)+(pidlTest->mkid.cb)),
			(void *)pidlNext);
	}

	PidlT *m_pPidl;
	PidlType m_pidlOriginal;
};
