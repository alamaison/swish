// Declaration of mock SftpConsumer COM object

#pragma once

#include "CppUnitExtensions.h"

#include <atlbase.h>   // ATL base classes
#include <atlcom.h>    // ATL CComObject et. al.

#include "swish/provider/SftpProvider.h" // ISftpProvider/Consumer

class ATL_NO_VTABLE CMockSftpConsumer :
    public ATL::CComObjectRootEx<ATL::CComObjectThreadModel>,
    public ISftpConsumer
{
public:

BEGIN_COM_MAP(CMockSftpConsumer)
    COM_INTERFACE_ENTRY(ISftpConsumer)
END_COM_MAP()

    /**
     * Creates a CMockSftpConsumer and returns smart pointers to its CComObject
     * as well as its ISftpConsumer interface.
     */
    static void CMockSftpConsumer::Create(
        __out ATL::CComPtrBase<CMockSftpConsumer>& spCoConsumer,
        __out ATL::CComPtrBase<ISftpConsumer>& spConsumer
    )
    {
        HRESULT hr;

        // Create mock object coclass instance
        ATL::CComObject<CMockSftpConsumer> *pCoConsumer = NULL;
        hr = ATL::CComObject<CMockSftpConsumer>::CreateInstance(&pCoConsumer);
        CPPUNIT_ASSERT_OK(hr);

        pCoConsumer->AddRef();
        spCoConsumer.Attach(pCoConsumer);
        pCoConsumer = NULL;

        // Get ISftpConsumer interface
        hr = spCoConsumer.QueryInterface(&spConsumer);
        pCoConsumer = NULL;
        CPPUNIT_ASSERT_OK(hr);
    }

    /**
     * Possible behaviours of mock password request handler OnPasswordRequest.
     */
    typedef enum tagPasswordBehaviour {
        EmptyPassword,    ///< Return an empty BSTR (not NULL, "")
        CustomPassword,   ///< Return the string set with SetPassword
        WrongPassword,    ///< Return a very unlikely sequence of characters
        NullPassword,     ///< Return NULL and S_OK (catastrophic failure)
        FailPassword,     ///< Return E_FAIL
        AbortPassword,    ///< Return E_ABORT (simulate user cancelled)
        ThrowPassword     ///< Throw exception if password requested
    } PasswordBehaviour;

    /**
     * Possible behaviours of mock keyboard-interactive request handler 
     * OnKeyboardInteractiveRequest.
     */
    typedef enum tagKeyboardInteractiveBehaviour {
        EmptyResponse,    ///< Return an empty BSTR (not NULL, "")
        CustomResponse,   ///< Return the string set with SetPassword
        WrongResponse,    ///< Return a very unlikely sequence of characters
        NullResponse,     ///< Return NULL and S_OK (catastrophic failure)
        FailResponse,     ///< Return E_FAIL
        AbortResponse,    ///< Return E_ABORT (simulate user cancelled)
        ThrowResponse     ///< Throw exception if kb-interaction requested
    } KeyboardInteractiveBehaviour;

    /**
     * Possible behaviours of file overwrite confirmation handlers
     * OnConfirmOverwrite.
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
    ATL::CComBSTR m_bstrCustomPassword;
    PasswordBehaviour m_enumPasswordBehaviour;
    UINT m_cPasswordAttempts;    ///< Number of password requests so far
    UINT m_nMaxPasswordAttempts; ///< Max password requests before auto-fail
    UINT m_cKbdAttempts;    ///< Number of kbd-interactive requests so far
    UINT m_nMaxKbdAttempts; ///< Max kbd-interactive requests before auto-fail
    KeyboardInteractiveBehaviour m_enumKeyboardInteractiveBehaviour;
    ConfirmOverwriteBehaviour m_enumConfirmOverwriteBehaviour;
    ReportErrorBehaviour m_enumReportErrorBehaviour;

public:
    // Set up default behaviours
    CMockSftpConsumer() :
        m_cPasswordAttempts(0), m_nMaxPasswordAttempts(1),
        m_cKbdAttempts(0), m_nMaxKbdAttempts(1),
        m_enumPasswordBehaviour(ThrowPassword),
        m_enumKeyboardInteractiveBehaviour(ThrowResponse),
        m_enumConfirmOverwriteBehaviour(ThrowOverwrite),
        m_enumReportErrorBehaviour(ThrowReport) {}

    void SetCustomPassword( PCTSTR pszPassword );
    void SetPasswordBehaviour( PasswordBehaviour enumBehaviour );
    void SetKeyboardInteractiveBehaviour( 
        KeyboardInteractiveBehaviour enumBehaviour );
    void SetMaxPasswordAttempts( UINT nAttempts );
    void SetMaxKeyboardAttempts( UINT nAttempts );
    void SetConfirmOverwriteBehaviour( 
        ConfirmOverwriteBehaviour enumBehaviour );
    void SetReportErrorBehaviour( ReportErrorBehaviour enumBehaviour );
    
    // ISftpConsumer methods
    IFACEMETHODIMP OnPasswordRequest(
        __in BSTR bstrRequest, __out BSTR *pbstrPassword
    );
    IFACEMETHODIMP OnKeyboardInteractiveRequest(
        __in BSTR bstrName, __in BSTR bstrInstruction,
        __in SAFEARRAY *psaPrompts,
        __in SAFEARRAY *psaShowResponses,
        __deref_out SAFEARRAY **ppsaResponses
    );
    IFACEMETHODIMP OnPrivateKeyFileRequest(
        __out BSTR *pbstrPrivateKeyFile
    );
    IFACEMETHODIMP OnPublicKeyFileRequest(
        __out BSTR *pbstrPublicKeyFile
    );
    IFACEMETHODIMP OnConfirmOverwrite(
        __in BSTR bstrOldFile,
        __in BSTR bstrNewFile
    );
    IFACEMETHODIMP OnReportError(
        __in BSTR bstrMessage
    );
    IFACEMETHODIMP OnHostkeyMismatch(
        __in BSTR bstrHostName,
        __in BSTR bstrHostKey,
        __in BSTR bstrHostKeyType
    );
    IFACEMETHODIMP OnHostkeyUnknown(
        __in BSTR bstrHostName,
        __in BSTR bstrHostKey,
        __in BSTR bstrHostKeyType
    );
};
