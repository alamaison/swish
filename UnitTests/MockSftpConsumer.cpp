// Mock SftpConsumer COM object implementation

#include "stdafx.h"
#include "MockSftpConsumer.h"

void CMockSftpConsumer::SetCustomPassword( PCTSTR pszPassword )
{
	m_bstrCustomPassword = pszPassword;
}

void CMockSftpConsumer::SetPasswordBehaviour( PasswordBehaviour enumBehaviour )
{
	m_enumPasswordBehaviour = enumBehaviour;
}

void CMockSftpConsumer::SetMaxPasswordAttempts( UINT nAttempts )
{
	m_nMaxPasswordAttempts = nAttempts;
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
	default:
		return E_FAIL;
	}

	// Return password BSTR
	*pbstrPassword = bstrPassword;
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
	default:
		return E_FAIL;
	}
}

STDMETHODIMP CMockSftpConsumer::OnConfirmOverwrite(
	BSTR bstrPrompt, BSTR bstrOldFile, BSTR bstrNewFile )
{
	CPPUNIT_ASSERT( CComBSTR(bstrPrompt).Length() > 0 );
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
	default:
		UNREACHABLE;
		return E_UNEXPECTED;
	}
}

STDMETHODIMP CMockSftpConsumer::OnConfirmOverwriteEx(
	BSTR bstrPrompt, Swish::Listing ltOldFile, Swish::Listing ltNewFile )
{
	CPPUNIT_ASSERT( CComBSTR(bstrPrompt).Length() > 0 );
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
	default:
		UNREACHABLE;
		return E_UNEXPECTED;
	}
}

STDMETHODIMP CMockSftpConsumer::OnReportError( BSTR bstrMessage )
{
	CPPUNIT_ASSERT( CComBSTR(bstrMessage).Length() > 0 );
	return S_OK;
}