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

// ISftpConsumer methods
STDMETHODIMP CMockSftpConsumer::OnPasswordRequest(
	BSTR bstrRequest, BSTR *pbstrPassword )
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

STDMETHODIMP CMockSftpConsumer::OnYesNoCancel(
	BSTR bstrMessage, BSTR bstrYesInfo, 
	BSTR bstrNoInfo, BSTR bstrCancelInfo,
	BSTR bstrTitle, int *piResult )
{
	UNREFERENCED_PARAMETER( bstrMessage );
	UNREFERENCED_PARAMETER( bstrYesInfo );
	UNREFERENCED_PARAMETER( bstrNoInfo );
	UNREFERENCED_PARAMETER( bstrCancelInfo );
	UNREFERENCED_PARAMETER( bstrTitle );

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