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

#include "SftpProvider.h" // ISftpProvider/ISftpConsumer interfaces

#include "common/atl.hpp"        // Common ATL setup

class CPool
{
public:

	ATL::CComPtr<ISftpProvider> GetSession(
		__in ISftpConsumer *pConsumer, __in PCWSTR pszHost, __in PCWSTR pszUser, 
		UINT uPort) throw(...);

private:

	ATL::CComPtr<IMoniker> _CreateMoniker(
		__in PCWSTR pszHost, __in PCWSTR pszUser, UINT uPort) throw(...);

	__checkReturn ATL::CComPtr<ISftpProvider> _GetSessionFromROT(
		__in PCWSTR pszHost, __in PCWSTR pszUser, UINT uPort) throw(...);

	void _StoreSessionInROT(
		__in ISftpProvider *pProvider, __in PCWSTR pszHost, __in PCWSTR pszUser, 
		UINT uPort) throw(...);

	ATL::CComPtr<ISftpProvider> _CreateNewSession(
		ISftpConsumer *pConsumer, PCWSTR pszHost, PCWSTR pszUser, UINT uPort)
		throw(...);
};
