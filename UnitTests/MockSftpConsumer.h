// Declaration of mock SftpConsumer COM object

#pragma once

#include "stdafx.h"
#include "CppUnitExtensions.h"

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
		FailPassword,     ///< Return E_FAIL
		ThrowPassword     ///< Throw exception if password requested
	} PasswordBehaviour;

	/**
	 * Possible behaviours of mock Yes/No/Cancel handler OnYesNoCancel.
	 */
	typedef enum tagYesNoCancelBehaviour {
		Yes,     ///< Return yes (1)
		No,      ///< Return no (0)
		Cancel,  ///< Return cancel (-1)
		ThrowYNC ///< Throw exception if OnYesNoCancel called
	} YesNoCancelBehaviour;

	/**
	 * Possible behaviours of file overwrite confirmation handlers
	 * OnConfirmOverwrite and OnConfirmOverwriteEx.
	 */
	typedef enum tagConfirmOverwriteBehaviour {
		AllowOverwrite,         ///< Return S_OK
		PreventOverwrite,       ///< Return E_ABORT
		PreventOverwriteSFalse, ///< Return S_FALSE (to be extra safe we should
		                        ///< only overwrite if explicitly S_OK)
		ThrowOverwrite          ///< Throw exception if confirmation requested
	} ConfirmOverwriteBehaviour;

	/**
	 * Possible behaviours when error reported to mock user via OnReportError.
	 */
	typedef enum tagReportErrorBehaviour {
		ErrorOK,       ///< Return S_OK
		ThrowReport    ///< Throw exception if error reported to user
	} ReportErrorBehaviour;

private:
	CComBSTR m_bstrCustomPassword;
	PasswordBehaviour m_enumPasswordBehaviour;
	UINT m_cPasswordAttempts;    ///< Number of password requests so far
	UINT m_nMaxPasswordAttempts; ///< Max password requests before auto-fail
	YesNoCancelBehaviour m_enumYesNoCancelBehaviour;
	ConfirmOverwriteBehaviour m_enumConfirmOverwriteBehaviour;
	ReportErrorBehaviour m_enumReportErrorBehaviour;

public:
	// Set up default behaviours
	CMockSftpConsumer() :
		m_cPasswordAttempts(0), m_nMaxPasswordAttempts(1),
		m_enumPasswordBehaviour(ThrowPassword),
		m_enumYesNoCancelBehaviour(ThrowYNC),
		m_enumConfirmOverwriteBehaviour(ThrowOverwrite),
		m_enumReportErrorBehaviour(ThrowReport) {}

	void SetCustomPassword( PCTSTR pszPassword );
	void SetPasswordBehaviour( PasswordBehaviour enumBehaviour );
	void SetMaxPasswordAttempts( UINT nAttempts );
	void SetYesNoCancelBehaviour( YesNoCancelBehaviour enumBehaviour );
	void SetConfirmOverwriteBehaviour( 
		ConfirmOverwriteBehaviour enumBehaviour );
	void SetReportErrorBehaviour( ReportErrorBehaviour enumBehaviour );
	
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
		__in BSTR bstrOldFile,
		__in BSTR bstrNewFile
	);
	IFACEMETHODIMP OnConfirmOverwriteEx(
		__in Swish::Listing ltOldFile,
		__in Swish::Listing ltNewFile
	);
	IFACEMETHODIMP OnReportError(
		__in BSTR bstrMessage
	);
};
