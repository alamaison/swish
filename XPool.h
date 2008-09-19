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

#include "resource.h"       // main symbols
#include "SftpProvider.h"

[
	coclass,
	default(IUnknown),
	threading(apartment),
	vi_progid("Swish.XPool"),
	progid("Swish.XPool.1"),
	version(1.0),
	uuid("b816a84c-5022-11dc-9153-0090f5284f85"),
	helpstring("XPool Class")
]
class ATL_NO_VTABLE CXPool
{
public:

	HRESULT GetConnection(
		__in ISftpConsumer *pConsumer, __in BSTR bstrHost, __in BSTR bstrUser, UINT uPort,
		__deref_out ISftpProvider **ppProvider );

private:

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

