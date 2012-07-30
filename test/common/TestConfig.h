#pragma once

#include <atlstr.h>

class CTestConfig
{
public:
    CTestConfig();
    ~CTestConfig();

    ATL::CString GetHost() const;
    ATL::CString GetUser() const;
    USHORT GetPort() const;
    ATL::CString GetPassword() const;

private:
    ATL::CString m_strHost;
    ATL::CString m_strUser;
    ATL::CString m_strPassword;
    UINT m_uPort;
};
