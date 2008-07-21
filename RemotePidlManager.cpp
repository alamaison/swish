/*  Manage creation/manipulation of PIDLs for files/folder in remote directory.

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

#include "stdafx.h"
#include "RemotePidlManager.h"

/**
 * Create a new terminated PIDL using the passed-in file information.
 * 
 * @param[in] pwszPath      Path of the file on the remote file-system
 * @param[in] pwszOwner     Name of the file owner on the remote system
 * @param[in] pwszGroup     Name of the file group on the remote system
 * @param[in] dwPermissions Value of the file's Unix permissions bits
 * @param[in] uSize         Size of the file in bytes
 * @param[in] dtModified    Date that the file was last modified as a Unix
 *                          timestamp
 * @param[in] fIsFolder     Indicates if the file is folder
 * @param[out] ppidlOut     Location that the resultant PIDL should be 
 *                          stored at
 * 
 * @returns Whether the PIDL was successfully created
 *   @retval S_OK if PIDL successful created at location @a ppidlOut
 *   @retval E_OUTOFMEMORY otherwise
 */
HRESULT CRemotePidlManager::Create(
	__in LPCWSTR pwszFilename, __in LPCWSTR pwszOwner, __in LPCWSTR pwszGroup,
	__in DWORD dwPermissions, __in ULONGLONG uSize, __in time_t dtModified,
	__in bool fIsFolder, __deref_out PITEMID_CHILD *ppidlOut )
{
	ATLASSERT(sizeof(REMOTEPIDL) % sizeof(DWORD) == 0); // DWORD-aligned

	PITEMID_CHILD pidl = NULL;

	// Allocate enough memory to hold a REMOTEPIDL structure plus terminator
    pidl = (PITEMID_CHILD)CoTaskMemAlloc(sizeof(REMOTEPIDL) + sizeof(USHORT));
    if(!pidl)
		return E_OUTOFMEMORY;

	// Use first PIDL member as a REMOTEPIDL structure
	PREMOTEPIDL pidlRemote = (PREMOTEPIDL)pidl;

	// Fill members of the PIDL with data
	pidlRemote->cb = sizeof REMOTEPIDL;
	pidlRemote->dwFingerprint = REMOTEPIDL_FINGERPRINT; // Sign with fingerprint
	CopyWSZString(pidlRemote->wszFilename, 
		ARRAYSIZE(pidlRemote->wszFilename), pwszFilename);
	CopyWSZString(pidlRemote->wszOwner, 
		ARRAYSIZE(pidlRemote->wszOwner), pwszOwner);
	CopyWSZString(pidlRemote->wszGroup, 
		ARRAYSIZE(pidlRemote->wszGroup), pwszGroup);
	pidlRemote->dwPermissions = dwPermissions;
	pidlRemote->uSize = uSize;
	pidlRemote->dtModified = dtModified;
	pidlRemote->fIsFolder = fIsFolder;

	// Create the terminating null PIDL by setting cb field to 0.
	LPITEMIDLIST pidlNext = GetNextItem(pidl);
	ATLASSERT(pidlNext != NULL);
	pidlNext->mkid.cb = 0;

	*ppidlOut = pidl;
	ATLASSERT(SUCCEEDED(IsValid(*ppidlOut)));
	ATLASSERT(ILGetNext(ILGetNext(*ppidlOut)) == NULL); // PIDL is terminated
	ATLASSERT(ILGetSize(*ppidlOut) == sizeof(REMOTEPIDL) + sizeof(USHORT));

    return S_OK;
}

/**
 * Validate that @a pidl is a REMOTEPIDL and return correctly typecast.
 * 
 * @param pidl PIDL to be validated
 * 
 * @returns Whether @a pidl is a valid REMOTEPIDL
 *   @retval pidl as a REMOTEPIDL if fingerprint matches #REMOTEPIDL_FINGERPRINT
 *   @retval NULL otherwise
 *       @n or if the function is being used as a simple boolean check
 *   @retval true if fingerprint matches #REMOTEPIDL_FINGERPRINT
 *   @retval false otherwise
 * 
 * Namespace extensions are heavily dependent on passing around
 * various types of PIDL (relative, absolute, single-level, multi-level,
 * created by us, created by others). Being very primitive structures,
 * they are not inherently type-safe and therefore it is sensible to check
 * that a PIDL we are about to use is what we expect it to be. 
 *
 * Validate() checks that @a pidl is a REMOTEPIDL by comparing its 
 * stored fingerprint with that which it should have given when created.
 * If validation succeeds, @a pidl is returned appropriately typecast giving
 * easy access to fields of REMOTEPIDL.
 *
 * @warning This is not a debug-only function. If the checking should only
 * be performed in debug builds, it should be wrapped in another debug function:
 *     @code ATLASSERT(Validate(pidl)); @endcode
 * 
 * @see IsValid() | REMOTEPIDL
 */
PREMOTEPIDL CRemotePidlManager::Validate( PCUIDLIST_RELATIVE pidl )
{
	PREMOTEPIDL pRemotePidl = NULL;

    if (pidl)
    {
        pRemotePidl = (PREMOTEPIDL)pidl;
		if (pRemotePidl->cb == sizeof(REMOTEPIDL) && 
			pRemotePidl->dwFingerprint == REMOTEPIDL_FINGERPRINT)
			return pRemotePidl; // equivalent to true
	}

    return NULL; // equal to false
}

/**
 * Check if the fingerprint stored in the PIDL corresponds to a REMOTEPIDL.
 * 
 * @param pidl PIDL to be validated
 * 
 * @returns Whether @a pidl is a valid REMOTEPIDL
 *     @retval S_OK if @a pidl is of type REMOTEPIDL
 *     @retval E_INVALIDARG otherwise
 * 
 * This function is very similar to Validate() except that a standard
 * COM success code is returned rather than a PIDL or a boolean.
 * 
 * @see Validate() | REMOTEPIDL
 */
HRESULT CRemotePidlManager::IsValid(
	PCUIDLIST_RELATIVE pidl, PM_VALIDMODE mode )
{
	PREMOTEPIDL pRemotePidl;

	if (mode == PM_LAST_PIDL)
		pRemotePidl = Validate(GetLastItem(pidl));
	else
		pRemotePidl = Validate(pidl);

    return pRemotePidl ? S_OK : E_FAIL;
}


/**
 * Get the filename from a PIDL.
 * 
 * @param pidl The PIDL from which to retrieve the filename
 * 
 * @returns The filename as a @c CString or an empty @c CString
 *          if @a pidl was not a valid REMOTEPIDL.
 * 
 * @pre Assumes that the pidl is a REMOTEPIDL.
 */
CString CRemotePidlManager::GetFilename( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszFilename;
}

/**
 * Get the name of the file's owner from a PIDL.
 * 
 * @param pidl The PIDL from which to retrieve the name
 * 
 * @returns The file owner's username as a @c CString or an empty @c CString
 *          if @a pidl was not a valid REMOTEPIDL.
 * 
 * @pre Assumes that the pidl is a REMOTEPIDL.
 */
CString CRemotePidlManager::GetOwner( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszOwner;
}

/**
 * Get the name of the file's group name from a PIDL.
 * 
 * @param pidl The PIDL from which to retrieve the name
 * 
 * @returns The file's group name as a @c CString or an empty @c CString
 *          if @a pidl was not a valid REMOTEPIDL.
 * 
 * @pre Assumes that the pidl is a REMOTEPIDL.
 */
CString CRemotePidlManager::GetGroup( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszGroup;
}

/**
 * Get the Unix file permissions from a PIDL.
 * 
 * @param pidl The PIDL from which to retrieve the permissions
 * 
 * @returns The file's Unix permissions as a numeric value or 0
 *          if @a pidl was not a valid REMOTEPIDL.
 * 
 * @pre Assumes that the pidl is a REMOTEPIDL.
 */
DWORD CRemotePidlManager::GetPermissions( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return 0;

	return GetDataSegment(pidl)->dwPermissions;
}

/**
 * Get the Unix file permissions in typical @c drwxrwxrwx form from a PIDL.
 * 
 * @param pidl The PIDL from which to retrieve the permissions
 * 
 * @returns The file's Unix permissions as a @c CString in @c drwxrwxrwx 
 *          form or an empty @c CString if @a pidl was not a valid REMOTEPIDL.
 *
 * @pre Assumes that the pidl is a REMOTEPIDL.
 *
 * @todo Implement this by calling function created for early Swish prototype.
 *       Currently, 'todo' is returned.
 */
CString CRemotePidlManager::GetPermissionsStr( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return _T("");

	// TODO: call old permissions function from swish prototype
	return _T("todo");
}

/**
 * Get the file's size from a PIDL.
 * 
 * @param pidl The PIDL from which to retrieve the size
 * 
 * @returns The file's size in bytes or 0 if the @a pidl was not a 
 *          valid REMOTEPIDL.
 * 
 * @pre Assumes that the pidl is a valid REMOTEPIDL.
 */
ULONGLONG CRemotePidlManager::GetFileSize( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return 0;

	return GetDataSegment(pidl)->uSize;
}

/**
 * Get the time and date that the file was last changed from a PIDL.
 * 
 * @param pidl The PIDL from which to retrieve the date
 * 
 * @returns The date of last file modification as a @c CTime or 0 if
 *          @a pidl was not a valid REMOTEPIDL.
 * 
 * @pre Assumes that the pidl is a REMOTEPIDL.
 */
CTime CRemotePidlManager::GetLastModified( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return 0;

	return GetDataSegment(pidl)->dtModified;
}

/**
 * Determine if the file represented by the PIDL is actually a folder.
 * 
 * @param pidl The PIDL to be checked
 * 
 * @returns @c TRUE if the PIDL represents a folder (directory) rather than a
 *          file or other filesystem object and @c FALSE otherwise.
 * 
 * @pre Assumes that the pidl is a REMOTEPIDL.
 */
bool CRemotePidlManager::IsFolder( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return 0;

	return GetDataSegment(pidl)->fIsFolder;
}

/**
 * Returns the relative PIDL as a REMOTEPIDL.
 * 
 * @param pidl The PIDL to retrieve the REMOTEPIDL data segment from.
 * 
 * @returns The @a pidl (as a REMOTEPIDL) if it is a valid REMOTEPIDL or
 *          @c NULL otherwise.
 * 
 * @pre Assumes that the pidl is a REMOTEPIDL.
 */
PREMOTEPIDL CRemotePidlManager::GetDataSegment( PCUIDLIST_RELATIVE pidl )
{
	ATLASSERT(SUCCEEDED(IsValid( pidl )));
	// If not, this is unexpected behviour. Why were we passed this PIDL?

	return Validate( pidl );
}
