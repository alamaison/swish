// Declaration of mock SftpConsumer COM object

#pragma once

#include "stdafx.h"
#include "CppUnitExtensions.h"

// CMockSftpConsumer
/*
[
	coclass,
	default(Swish::ISftpConsumer),
	threading(apartment),
	vi_progid("SwishUnitTests.MockSftpConsumer"),
	progid("SwishUnitTests.MockSftpConsumer.1"),
	version(1.0),
	uuid("2FAC5B0C-EC40-408b-A839-892356A5BA47"),
	noncreatable
]
*/
class ATL_NO_VTABLE CMockSftpConsumer :
	public CComObjectRootEx<CComObjectThreadModel>,
	public Swish::ISftpConsumer
{
public:

BEGIN_COM_MAP(CMockSftpConsumer)
	COM_INTERFACE_ENTRY(Swish::ISftpConsumer)
END_COM_MAP()

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

	/**
	 * Possible behaviours of file overwrite confirmation handlers
	 * OnConfirmOverwrite and OnConfirmOverwriteEx.
	 */
	typedef enum tagConfirmOverwriteBehaviour {
		AllowOverwrite,         ///< Return S_OK
		PreventOverwrite,       ///< Return E_ABORT
		PreventOverwriteSFalse  ///< Return S_FALSE (to be extra safe we should
		                        ///< only overwrite if explicitly S_OK)
	} ConfirmOverwriteBehaviour;

private:
	CComBSTR m_bstrCustomPassword;
	PasswordBehaviour m_enumPasswordBehaviour;
	UINT m_cPasswordAttempts;    ///< Number of password requests so far
	UINT m_nMaxPasswordAttempts; ///< Max password requests before auto-fail
	YesNoCancelBehaviour m_enumYesNoCancelBehaviour;
	ConfirmOverwriteBehaviour m_enumConfirmOverwriteBehaviour;

public:
	// Set up default behaviours
	CMockSftpConsumer() :
		m_cPasswordAttempts(0), m_nMaxPasswordAttempts(1),
		m_enumPasswordBehaviour(EmptyPassword),
		m_enumYesNoCancelBehaviour(Cancel),
		m_enumConfirmOverwriteBehaviour(PreventOverwrite) {}

	void SetCustomPassword( PCTSTR pszPassword );
	void SetPasswordBehaviour( PasswordBehaviour enumBehaviour );
	void SetMaxPasswordAttempts( UINT nAttempts );
	void SetYesNoCancelBehaviour( YesNoCancelBehaviour enumBehaviour );
	void SetConfirmOverwriteBehaviour( 
		ConfirmOverwriteBehaviour enumBehaviour );
	
	// ISftpConsumer methods
	IFACEMETHODIMP OnPasswordRequest(
		__in BSTR bstrRequest, __out BSTR *pbstrPassword
	);
	IFACEMETHODIMP OnYesNoCancel(
		__in BSTR bstrMessage, __in_opt BSTR bstrYesInfo, 
		__in_opt BSTR bstrNoInfo, __in_opt BSTR bstrCancelInfo,
		__in BSTR bstrTitle, __out int *piResult
	);
	IFACEMETHODIMP OnConfirmOverwrite(
		__in BSTR bstrPrompt,
		__in BSTR bstrOldFile,
		__in BSTR bstrNewFile
	);
	IFACEMETHODIMP OnConfirmOverwriteEx(
		__in BSTR bstrPrompt,
		__in Swish::Listing ltOldFile,
		__in Swish::Listing ltNewFile
	);
	IFACEMETHODIMP OnReportError(
		__in BSTR bstrMessage
	);
};
