//PuttyProvider_Test.h    -   declares the class PuttyProvider_Test

#ifndef CPP_UNIT_PuttyProvider_TEST_H
#define CPP_UNIT_PuttyProvider_TEST_H

#include "stdafx.h"
#include "CppUnitExtensions.h"
#include "_Swish.h"

#define ISftpProvider ISftpProviderUnstable
#define IID_ISftpProvider IID_ISftpProviderUnstable
#define ISftpConsumer ISftpConsumerUnstable
#define IID_ISftpConsumer IID_ISftpConsumerUnstable

struct testFILEDATA
{
	BOOL fIsFolder;
	CString strPath;
	CString strOwner;
	CString strGroup;
	CString strAuthor;

	ULONGLONG uSize; // 64-bit allows files up to 16 Ebibytes (a lot)
	time_t dtModified;
	DWORD dwPermissions;
};

// CMockSftpConsumer
[
	coclass,
	default(ISftpConsumer),
	threading(apartment),
	vi_progid("Swish.UnitTests.MockSftpConsumer"),
	progid("Swish.UnitTests.MockSftpConsumer.1"),
	version(1.0),
	uuid("2FAC5B0C-EC40-408b-A839-892356A5BA47")
]
class ATL_NO_VTABLE CMockSftpConsumer :
	public ISftpConsumer
{
public:
	
	/**
	 * Possible behaviours of mock password request handler OnPasswordRequest.
	 */
	typedef enum tagPasswordBehaviour {
		EmptyPassword,    ///< Return an empty BSTR (not NULL, "")
		CustomPassword,   ///< Return the string set with SetPassword
		WrongPassword,    ///< Return a very unlikely sequence of characters
		NullPassword,     ///< Return NULL and S_OK (catastrophic failure)
		FailPassword      ///< Return E_FAIL
	} PasswordBehaviour;

	/**
	 * Possible behaviours of mock Yes/No/Cancel handler OnYesNoCancel.
	 */
	typedef enum tagYesNoCancelBehaviour {
		Yes,    ///< Return yes (1)
		No,     ///< Return no (0)
		Cancel  ///< Return cancel (-1)
	} YesNoCancelBehaviour;

private:
	CComBSTR m_bstrCustomPassword;
	PasswordBehaviour m_enumPasswordBehaviour;
	UINT m_cPasswordAttempts;    ///< Number of password requests so far
	UINT m_nMaxPasswordAttempts; ///< Max password requests before auto-fail
	YesNoCancelBehaviour m_enumYesNoCancelBehaviour;

public:
	CMockSftpConsumer() :
	  m_cPasswordAttempts(0), m_nMaxPasswordAttempts(1),
	  m_enumPasswordBehaviour(EmptyPassword),
	  m_enumYesNoCancelBehaviour(Cancel) {}

	void SetCustomPassword( PCTSTR pszPassword )
	{
		m_bstrCustomPassword = pszPassword;
	}

	void SetPasswordBehaviour( PasswordBehaviour enumBehaviour )
	{
		m_enumPasswordBehaviour = enumBehaviour;
	}
	
	void SetMaxPasswordAttempts( UINT nAttempts )
	{
		m_nMaxPasswordAttempts = nAttempts;
	}
	
	void SetYesNoCancelBehaviour( YesNoCancelBehaviour enumBehaviour )
	{
		m_enumYesNoCancelBehaviour = enumBehaviour;
	}
	
	// ISftpConsumer methods
	IFACEMETHODIMP OnPasswordRequest(
		__in BSTR bstrRequest, __out BSTR *pbstrPassword )
	{
		m_cPasswordAttempts++;

		CComBSTR bstrPrompt = bstrRequest;
		ATLENSURE_RETURN_HR( bstrPrompt.Length() > 0, E_INVALIDARG );
		ATLENSURE_RETURN_HR( pbstrPassword, E_POINTER );
		ATLENSURE_RETURN_HR( *pbstrPassword == NULL, E_FAIL );

		// Perform chosen test behaviour
		CComBSTR bstrPassword;
		switch (m_enumPasswordBehaviour)
		{
		case CustomPassword:
			ATLENSURE_RETURN_HR(
				m_cPasswordAttempts <= m_nMaxPasswordAttempts,
				E_UNEXPECTED
			);
			bstrPassword = m_bstrCustomPassword;
			break;
		case WrongPassword:
			if (m_cPasswordAttempts > m_nMaxPasswordAttempts) return E_FAIL;
			bstrPassword = _T("WrongPasswordXyayshdkhjhdk");
			break;
		case EmptyPassword:
			if (m_cPasswordAttempts > m_nMaxPasswordAttempts) return E_FAIL;
			bstrPassword = _T("");
			break;
		case NullPassword:
			if (m_cPasswordAttempts > m_nMaxPasswordAttempts) return E_FAIL;
			break;
		case FailPassword:
		default:
			return E_FAIL;
		}

		// Return password BSTR
		*pbstrPassword = bstrPassword;
		return S_OK;
	}

	IFACEMETHODIMP OnYesNoCancel(
		__in BSTR bstrMessage, __in_opt BSTR bstrYesInfo, 
		__in_opt BSTR bstrNoInfo, __in_opt BSTR bstrCancelInfo,
		__in BSTR bstrTitle, __out int *piResult )
	{
		CComBSTR bstrPrompt = bstrMessage;
		ATLENSURE_RETURN_HR( bstrPrompt.Length() > 0, E_INVALIDARG );
		ATLENSURE_RETURN_HR( piResult, E_POINTER );

		// Perform chosen test behaviour
		switch (m_enumYesNoCancelBehaviour)
		{
		case Yes:
			*piResult = 1;
			return S_OK;
		case No:
			*piResult = 0;
			return S_OK;
		case Cancel:
			*piResult = -1;
			return E_ABORT;
		default:
			return E_FAIL;
		}
	}
};

class CPuttyProvider_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CPuttyProvider_test );
		CPPUNIT_TEST( testQueryInterface );
		CPPUNIT_TEST( testInitialize );
		CPPUNIT_TEST( testGetListing );
	CPPUNIT_TEST_SUITE_END();

public:
	CPuttyProvider_test() : m_pProvider(NULL) {}
	void setUp();
	void tearDown();

protected:
	void testQueryInterface();
	void testInitialize();
	void testGetListing();
	void testGetListing_WrongPassword();

private:
	CComObject<CMockSftpConsumer> *m_pCoConsumer;
	ISftpConsumer *m_pConsumer;
	ISftpProvider *m_pProvider;

	void CreateMockSftpConsumer(
		__out CComObject<CMockSftpConsumer> **ppCoConsumer,
		__out ISftpConsumer **ppConsumer
	) const;
	void testListingFormat(__in IEnumListing *pEnum) const;
	void testRegistryStructure() const;
	CString GetHostName() const;
	CString GetUserName() const;
	USHORT GetPort() const;
	CString GetPassword() const;
};

#endif
