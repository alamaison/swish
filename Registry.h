/*  Helper class for Swish registry access.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "HostPidl.h"
#include "RemotePidl.h"

#include <vector>
using std::vector;

class CRegistry
{
public:
	static vector<CHostItem> LoadConnectionsFromRegistry() throw(...);
	
private:
	static CHostItem _GetConnectionDetailsFromRegistry(__in PCWSTR pwszLabel)
		throw(...);

public:
	static HRESULT GetHostFolderAssocKeys(
		__out UINT *pcKeys, __deref_out_ecount(pcKeys) HKEY **paKeys) throw();
	static HRESULT GetRemoteFolderAssocKeys(
		__in CRemoteItemHandle pidl, 
		__out UINT *pcKeys, __deref_out_ecount(pcKeys) HKEY **paKeys) throw();

private:
	static vector<CString> _GetHostFolderAssocKeynames() throw();
	static vector<CString> _GetRemoteFolderAssocKeynames(
		__in CRemoteItemHandle pidl) throw(...);

	static vector<CString> _GetKeynamesForFolder() throw();
	static vector<CString> _GetKeynamesCommonToAll() throw();
	static vector<CString> _GetKeynamesForExtension(__in PCWSTR pwszExtension)
		throw();

	static HRESULT _GetHKEYArrayFromKeynames(
		__in const vector<CString> vecNames, 
		__out UINT *pcKeys, __deref_out_ecount(pcKeys) HKEY **paKeys) throw();

	static HRESULT _GetHKEYArrayFromVector(
		__in const vector<HKEY> vecKeys, 
		__out UINT *pcKeys, __deref_out_ecount(pcKeys) HKEY **paKeys) throw();

	static vector<HKEY> _GetKeysFromKeynames(
		__in const vector<CString> vecKeynames) throw();
};
