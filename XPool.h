/*  Pool of reusuable SFTP connections.

    Copyright (C) 2007, 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once
#pragma warning (disable: 4503) // Decorated name length exceeded, trunacted

#include "resource.h"       // main symbols

#include "SftpProvider.h"

#include <string>
using std::wstring;

#include <hash_map>
using stdext::hash_map;

#include <map>
using std::map;

[
	object,
	uuid("b816a84c-5022-11dc-9153-0090f5284f85"),
	oleautomation,	helpstring("IXPool Interface"),
	pointer_default(unique)
]
__interface IXPool : IUnknown
{
};


[
	coclass,
	default(IXPool),
	threading(apartment),
	vi_progid("Swish.XPool"),
	progid("Swish.XPool.1"),
	version(1.0),
	uuid("b816a84d-5022-11dc-9153-0090f5284f85"),
	helpstring("XPool Class")
]
class ATL_NO_VTABLE CXPool :
	public IXPool
{
public:
	CXPool()
	{
	}
	DECLARE_PROTECT_FINAL_CONSTRUCT()
	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	void FinalRelease()
	{
	}

	HRESULT GetConnection(
		__in ISftpConsumer *pConsumer, __in BSTR bstrHost, __in BSTR bstrUser, UINT uPort,
		__deref_out ISftpProvider **ppProvider );

private:
	typedef hash_map<UINT, CComPtr<ISftpProvider> > PortMap;
	typedef hash_map<std::wstring, PortMap> UserPortMap;
	typedef hash_map<std::wstring, UserPortMap> HostUserPortMap;
	HostUserPortMap m_mapSessions;

	CComPtr<IMoniker> _CreateMoniker(
		__in BSTR bstrHost, __in BSTR bstrUser, UINT uPort) throw(...);
	__checkReturn CComPtr<ISftpProvider> _GetConnectionFromROT(
		__in BSTR bstrHost, __in BSTR bstrUser, UINT uPort) throw(...);
	void _StoreConnectionInROT(
		__in ISftpProvider *pProvider, __in BSTR bstrHost, __in BSTR bstrUser, 
		UINT uPort) throw(...);
	CComPtr<ISftpProvider> _CreateNewConnection(
		ISftpConsumer *pConsumer, BSTR bstrHost, BSTR bstrUser, UINT uPort)
		throw(...);
};

