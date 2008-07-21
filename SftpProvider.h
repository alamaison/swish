/*  Declaration of the ISftpProvider and ISftpConsumer interfaces.

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

#ifndef SFTPPROVIDER_H
#define SFTPPROVIDER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma once
#include "stdafx.h"
#include "resource.h"       // main symbols

/**
 * The record structure returned by the GetListing() method of the SFTPProvider.
 *
 * This structure represents a single file contained in the directory 
 * specified to GetListing().
 */
[export, library_block]
struct Listing {
	BSTR bstrFilename;    ///< Directory-relative filename (e.g. README.txt)
	ULONG uPermissions;   ///< Unix file permissions
	BSTR bstrOwner;       ///< The user name of the file's owner
	BSTR bstrGroup;       ///< The name of the group to which the file belongs
	ULONG cSize;          ///< The file's size in bytes
	ULONG cHardLinks;     ///< The number of hard links referencing this file
	DATE dateModified;    ///< The date and time at which the file was 
	                      ///< last modified in automation-compatible format
};

// IEnumListing
[
	object, oleautomation, library_block,
	uuid("b816a843-5022-11dc-9153-0090f5284f85"),
	helpstring("IEnumListing Interface"),
	pointer_default(unique)
]
__interface IEnumListing : IUnknown
{
	/* Will need local/remote hack if used remotely */
	HRESULT Next (
		[in] ULONG celt,
		[out] Listing *rgelt,
		[out] ULONG *pceltFetched);
	HRESULT Skip ([in] ULONG celt);
	HRESULT Reset ();
	HRESULT Clone ([out] IEnumListing **ppEnum);
};

// While interfaces are still in flux use these redefinitions to point
// the identifiers to a temporary but unique version of the interface
// for each release.
#define ISftpProvider ISftpProviderUnstable
#define IID_ISftpProvider __uuidof(ISftpProviderUnstable)
#define ISftpConsumer ISftpConsumerUnstable
#define IID_ISftpConsumer __uuidof(ISftpConsumerUnstable)

// ISftpConsumer
[
	object, oleautomation, library_block,
	uuid("99293E0D-C3AB-4b50-8132-329E30216E14"),
	//uuid("b816a844-5022-11dc-9153-0090f5284f85"),
	helpstring("ISftpConsumer Interface"),
	pointer_default(unique)
]
__interface ISftpConsumer
{
	HRESULT OnPasswordRequest(
		[in]          BSTR bstrRequest,
		[out, retval] BSTR *pbstrPassword
	);
	HRESULT OnYesNoCancel(
		[in]          BSTR bstrMessage,
		[in]          BSTR bstrYesInfo,
		[in]          BSTR bstrNoInfo,
		[in]          BSTR bstrCancelInfo,
		[in]          BSTR bstrTitle,
		[out, retval] int *piResult
	);
};

// ISftpProvider
[
	object, oleautomation, library_block,
	uuid("93874AB6-D2AE-47c0-AFB7-F59A7507FADA"),
	//uuid("b816a841-5022-11dc-9153-0090f5284f85"),
	helpstring("ISftpProvider Interface"),
	pointer_default(unique)
]
__interface ISftpProvider
{
	HRESULT Initialize(
		[in] ISftpConsumer *pConsumer,
		[in] BSTR bstrUser,
		[in] BSTR bstrHost,
		// TODO short is too small
		[in] short uPort
	);
	HRESULT GetListing(
		[in] BSTR bstrDirectory,
		[out, retval] IEnumListing **ppEnum
	);
	
};

#endif // SFTPPROVIDER_H
