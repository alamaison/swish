#pragma once

class CTestConfig
{
public:
	CTestConfig();
	~CTestConfig();

	CString GetHost() const;
	CString GetUser() const;
	USHORT GetPort() const;
	CString GetPassword() const;

private:
	CString m_strHost;
	CString m_strUser;
	CString m_strPassword;
	UINT m_uPort;
};
