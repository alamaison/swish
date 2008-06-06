/* @file Swish-specific extensions to the CppUnit facilities */

#pragma once

#include <cppunit/extensions/HelperMacros.h>

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

/**
 * COM HRESULT-specific assertions
 * @{
 */
#define CPPUNIT_ASSERT_OK(hresult) CPPUNIT_ASSERT_EQUAL((HRESULT)S_OK, hresult)
#define CPPUNIT_ASSERT_SUCCEEDED(hresult) CPPUNIT_ASSERT(SUCCEEDED(hresult))
#define CPPUNIT_ASSERT_FAILED(hresult) CPPUNIT_ASSERT(FAILED(hresult))
/* @} */
CPPUNIT_NS_END
