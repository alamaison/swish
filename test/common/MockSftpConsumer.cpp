/**
 * @file Mock ISftpConsumer COM object implementation.
 */

#include "pch.h"              // Precompiled header
#include "standard.h"

#include "debug.h"            // Debug macros
#include "MockSftpConsumer.h"

#include <atlsafe.h>          // CComSafeArray

using namespace ATL;

void CMockSftpConsumer::SetCustomPassword( PCTSTR pszPassword )
{
	m_bstrCustomPassword = pszPassword;
}

void CMockSftpConsumer::SetPasswordBehaviour( PasswordBehaviour enumBehaviour )
{
	m_enumPasswordBehaviour = enumBehaviour;
}

void CMockSftpConsumer::SetKeyboardInteractiveBehaviour(
	KeyboardInteractiveBehaviour enumBehaviour )
{
	m_enumKeyboardInteractiveBehaviour = enumBehaviour;
}

void CMockSftpConsumer::SetMaxPasswordAttempts( UINT nAttempts )
{
	m_nMaxPasswordAttempts = nAttempts;
}

void CMockSftpConsumer::SetMaxKeyboardAttempts( UINT nAttempts )
{
	m_nMaxKbdAttempts = nAttempts;
}

void CMockSftpConsumer::SetYesNoCancelBehaviour(
	YesNoCancelBehaviour enumBehaviour )
{
	m_enumYesNoCancelBehaviour = enumBehaviour;
}

void CMockSftpConsumer::SetConfirmOverwriteBehaviour(
	ConfirmOverwriteBehaviour enumBehaviour )
{
	m_enumConfirmOverwriteBehaviour = enumBehaviour;
}

void CMockSftpConsumer::SetReportErrorBehaviour(
	ReportErrorBehaviour enumBehaviour )
{
	m_enumReportErrorBehaviour = enumBehaviour;
}	

// ISftpConsumer methods
STDMETHODIMP CMockSftpConsumer::OnPasswordRequest(
	BSTR bstrRequest, BSTR *pbstrPassword )
{
	m_cPasswordAttempts++;

	CComBSTR bstrPrompt = bstrRequest;
	CPPUNIT_ASSERT( bstrPrompt.Length() > 0 );
	CPPUNIT_ASSERT( pbstrPassword );
	CPPUNIT_ASSERT_EQUAL( (OLECHAR)NULL, (OLECHAR)*pbstrPassword );

	// Perform chosen test behaviour
	// The three password cases which should never succeed will try to send
	// their 'reply' up to m_nMaxPassword time to simulate a user repeatedly
	// trying the wrong password and then giving up. The custom password case
	// should never need a retry and will signal failure if there has been 
	// more than one attempt.
	CComBSTR bstrPassword;
	switch (m_enumPasswordBehaviour)
	{
	case CustomPassword:
		CPPUNIT_ASSERT_EQUAL( (UINT)1, m_cPasswordAttempts );
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
		return E_FAIL;
	case AbortPassword:
		return E_ABORT;
	case ThrowPassword:
		CPPUNIT_FAIL("Unexpected call to " __FUNCTION__);
		return E_FAIL;
	default:
		UNREACHABLE;
		return E_UNEXPECTED;
	}

	// Return password BSTR
	*pbstrPassword = bstrPassword.Detach();
	return S_OK;
}

STDMETHODIMP CMockSftpConsumer::OnKeyboardInteractiveRequest(
	BSTR bstrName, BSTR bstrInstruction, SAFEARRAY *psaPrompts, 
	SAFEARRAY *psaShowResponses, SAFEARRAY **ppsaResponses
)
{
	UNREFERENCED_PARAMETER(bstrName);        // These two are optional
	UNREFERENCED_PARAMETER(bstrInstruction);

	m_cKbdAttempts++;

	CComSafeArray<BSTR> saPrompts(psaPrompts);
	CComSafeArray<VARIANT_BOOL> saShowResponses(psaShowResponses);

	for (int i = saPrompts.GetLowerBound(); i <= saPrompts.GetUpperBound(); i++)
	{
		CComBSTR bstrPrompt = saPrompts[i];
		CPPUNIT_ASSERT( bstrPrompt.Length() > 0 );
	}

	CPPUNIT_ASSERT_EQUAL(
		saPrompts.GetLowerBound(), saShowResponses.GetLowerBound() );
	CPPUNIT_ASSERT_EQUAL(
		saPrompts.GetUpperBound(), saShowResponses.GetUpperBound() );

	// Perform chosen test behaviour
	// The three response cases which should never succeed will try to send
	// their 'reply' up to m_nMaxKbdAttempts time to simulate a user repeatedly
	// trying the wrong password and then giving up. The custom password case
	// should never need a retry and will signal failure if there has been 
	// more than one attempt.
	CComBSTR bstrResponse;
	switch (m_enumKeyboardInteractiveBehaviour)
	{
	case CustomResponse:
		CPPUNIT_ASSERT_EQUAL( (UINT)1, m_cKbdAttempts );
		bstrResponse = m_bstrCustomPassword;
		break;
	case WrongResponse:
		if (m_cKbdAttempts > m_nMaxKbdAttempts) return E_FAIL;
		bstrResponse = _T("WrongPasswordXyayshdkhjhdk");
		break;
	case EmptyResponse:
		if (m_cKbdAttempts > m_nMaxKbdAttempts) return E_FAIL;
		bstrResponse = _T("");
		break;
	case NullResponse:
		if (m_cKbdAttempts > m_nMaxKbdAttempts) return E_FAIL;
		break;
	case FailResponse:
		return E_FAIL;
	case AbortResponse:
		return E_ABORT;
	case ThrowResponse:
		CPPUNIT_FAIL("Unexpected call to " __FUNCTION__);
		return E_FAIL;
	default:
		UNREACHABLE;
		return E_UNEXPECTED;
	}

	// Create responses.  Return password BSTR as first response.  Any other 
	// prompts are responded to with an empty string.
	CComSafeArray<BSTR> saResponses(
		saPrompts.GetCount(), saPrompts.GetLowerBound());
	int i = saResponses.GetLowerBound();
	saResponses.SetAt(i++, bstrResponse.Detach(), FALSE);
	while (i <= saResponses.GetUpperBound())
	{
		saResponses.SetAt(i++, ::SysAllocString(OLESTR("")), FALSE);
	}

	*ppsaResponses = saResponses.Detach();
	return S_OK;
}

STDMETHODIMP CMockSftpConsumer::OnYesNoCancel(
	BSTR bstrMessage, BSTR bstrYesInfo, 
	BSTR bstrNoInfo, BSTR bstrCancelInfo,
	BSTR bstrTitle, int *piResult )
{
	UNREFERENCED_PARAMETER( bstrYesInfo );
	UNREFERENCED_PARAMETER( bstrNoInfo );
	UNREFERENCED_PARAMETER( bstrCancelInfo );
	UNREFERENCED_PARAMETER( bstrTitle );

	CPPUNIT_ASSERT( CComBSTR(bstrMessage).Length() > 0 );
	CPPUNIT_ASSERT( piResult );

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
	case ThrowYNC:
		CPPUNIT_FAIL("Unexpected call to " __FUNCTION__);
		return E_FAIL;
	default:
		UNREACHABLE;
		return E_UNEXPECTED;
	}
}

STDMETHODIMP CMockSftpConsumer::OnConfirmOverwrite(
	BSTR bstrOldFile, BSTR bstrNewFile )
{
	CPPUNIT_ASSERT( CComBSTR(bstrOldFile).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(bstrNewFile).Length() > 0 );

	// Perform chosen test behaviour
	switch (m_enumConfirmOverwriteBehaviour)
	{
	case AllowOverwrite:
		return S_OK;
	case PreventOverwrite:
		return E_ABORT;
	case PreventOverwriteSFalse:
		return S_FALSE;
	case ThrowOverwrite:
		CPPUNIT_FAIL("Unexpected call to " __FUNCTION__);
		return E_FAIL;
	default:
		UNREACHABLE;
		return E_UNEXPECTED;
	}
}

STDMETHODIMP CMockSftpConsumer::OnConfirmOverwriteEx(
	Listing ltOldFile, Listing ltNewFile )
{
	CPPUNIT_ASSERT( CComBSTR(ltOldFile.bstrFilename).Length() > 0 );
	CPPUNIT_ASSERT( CComBSTR(ltNewFile.bstrFilename).Length() > 0 );

	// Perform chosen test behaviour
	switch (m_enumConfirmOverwriteBehaviour)
	{
	case AllowOverwrite:
		return S_OK;
	case PreventOverwrite:
		return E_ABORT;
	case PreventOverwriteSFalse:
		return S_FALSE;
	case ThrowOverwrite:
		CPPUNIT_FAIL("Unexpected call to " __FUNCTION__);
		return E_FAIL;
	default:
		UNREACHABLE;
		return E_UNEXPECTED;
	}
}

STDMETHODIMP CMockSftpConsumer::OnReportError( BSTR bstrMessage )
{
	switch (m_enumReportErrorBehaviour)
	{
	case ErrorOK:
		CPPUNIT_ASSERT( CComBSTR(bstrMessage).Length() > 0 );
		return S_OK;
	case ThrowReport:
		{
			std::string strMessage = "Unexpected call to " __FUNCTION__ ": ";
			strMessage += CStringA(bstrMessage);
			CPPUNIT_FAIL(strMessage);
			return E_FAIL;
		}
	default:
		UNREACHABLE;
		return E_FAIL;
	}
}
