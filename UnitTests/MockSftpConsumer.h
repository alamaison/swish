// Declaration of mock SftpConsumer COM object

#ifndef MOCKSFTPCONSUMER_H
#define MOCKSFTPCONSUMER_H

#include "stdafx.h"
#include "CppUnitExtensions.h"

//#define ISftpProvider ISftpProviderUnstable
#define ISftpConsumer ISftpConsumerUnstable

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

	void SetCustomPassword( PCTSTR pszPassword );
	void SetPasswordBehaviour( PasswordBehaviour enumBehaviour );
	void SetMaxPasswordAttempts( UINT nAttempts );
	void SetYesNoCancelBehaviour( YesNoCancelBehaviour enumBehaviour );
	
	// ISftpConsumer methods
	IFACEMETHODIMP OnPasswordRequest(
		__in BSTR bstrRequest, __out BSTR *pbstrPassword );
	IFACEMETHODIMP OnYesNoCancel(
		__in BSTR bstrMessage, __in_opt BSTR bstrYesInfo, 
		__in_opt BSTR bstrNoInfo, __in_opt BSTR bstrCancelInfo,
		__in BSTR bstrTitle, __out int *piResult );
};

#endif
