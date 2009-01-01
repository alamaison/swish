/* @file Swish-specific extensions to the CppUnit facilities */

#pragma once

#include <cppunit/extensions/HelperMacros.h>
#include <atlstr.h>

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

#define STRMESSAGE_CASE(hr) case hr: strMessage = #hr; break;
inline std::string GetErrorFromHResult(HRESULT hResult)
{
	std::string strMessage;
	switch (hResult)
	{
		STRMESSAGE_CASE(S_OK);
		STRMESSAGE_CASE(S_FALSE);
		STRMESSAGE_CASE(E_ABORT);
		STRMESSAGE_CASE(E_ACCESSDENIED);
		STRMESSAGE_CASE(E_UNEXPECTED);
		STRMESSAGE_CASE(E_NOTIMPL);
		STRMESSAGE_CASE(E_OUTOFMEMORY);
		STRMESSAGE_CASE(E_INVALIDARG);
		STRMESSAGE_CASE(E_NOINTERFACE);
		STRMESSAGE_CASE(E_POINTER);
		STRMESSAGE_CASE(E_HANDLE);
		STRMESSAGE_CASE(E_FAIL);
		STRMESSAGE_CASE(E_PENDING);
	default:
		strMessage = "<unknown>";
	}

	char *pszMessage = NULL;
	if (::FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		hResult, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		reinterpret_cast<char*>(&pszMessage), 0, NULL) && (pszMessage != NULL))
	{
		strMessage += ": ";
		strMessage += pszMessage;
		::LocalFree(reinterpret_cast<HLOCAL>(pszMessage));
	}

	return strMessage;
}

/**
 * COM HRESULT-specific assertions
 * @{
 */
#define CPPUNIT_ASSERT_OK(hresult) \
	do { \
		HRESULT hrCopy; \
		hrCopy = hresult; \
		std::string str("COM return code was "); \
		str += CPPUNIT_NS::GetErrorFromHResult(hrCopy); \
		CPPUNIT_ASSERT_MESSAGE(str, hrCopy == S_OK); \
	} while(0);
#define CPPUNIT_ASSERT_SUCCEEDED(hresult) CPPUNIT_ASSERT(SUCCEEDED(hresult))
#define CPPUNIT_ASSERT_FAILED(hresult) CPPUNIT_ASSERT(FAILED(hresult))
/* @} */

/**
 * Convenience macros
 * @{
 */
template <class T>
void assertZero( const T& actual,
                 SourceLine sourceLine,
                 const std::string &message )
{
	CPPUNIT_NS::assertEquals(static_cast<T>(0), actual, sourceLine, message);
}
#define CPPUNIT_ASSERT_ZERO(actual) \
	do { \
		CPPUNIT_NS::assertZero((actual), CPPUNIT_SOURCELINE(), \
			""#actual " != 0"); \
	} while(0);
/* @} */

CPPUNIT_NS_END
