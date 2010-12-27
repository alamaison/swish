/**
    @file

    Helper class for Swish registry access.

    @if license

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    @endif
*/

#pragma once

#include "RemotePidl.h"

#include <vector>

class CRegistry
{

public:
	static HRESULT GetHostFolderAssocKeys(
		__out UINT *pcKeys, __deref_out_ecount(pcKeys) HKEY **paKeys) throw();
	static HRESULT GetRemoteFolderAssocKeys(
		__in CRemoteItemHandle pidl, 
		__out UINT *pcKeys, __deref_out_ecount(pcKeys) HKEY **paKeys) throw();

private:
	typedef std::vector<ATL::CString> KeyNames;

	static KeyNames _GetHostFolderAssocKeynames() throw();
	static KeyNames _GetRemoteFolderAssocKeynames(
		__in CRemoteItemHandle pidl) throw(...);

	static KeyNames _GetKeynamesForFolder() throw();
	static KeyNames _GetKeynamesCommonToAll() throw();
	static KeyNames _GetKeynamesForExtension(__in PCWSTR pwszExtension)
		throw();

	static HRESULT _GetHKEYArrayFromKeynames(
		__in const KeyNames vecNames, 
		__out UINT *pcKeys, __deref_out_ecount(pcKeys) HKEY **paKeys) throw();

	static HRESULT _GetHKEYArrayFromVector(
		__in const std::vector<HKEY> vecKeys, 
		__out UINT *pcKeys, __deref_out_ecount(pcKeys) HKEY **paKeys) throw();

	static std::vector<HKEY> _GetKeysFromKeynames(
		__in const KeyNames vecKeynames) throw();
};
