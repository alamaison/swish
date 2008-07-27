/*  Expose contents of remote folder as PIDLs via IEnumIDList interface.

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
#include "RemoteEnumIDList.h"
#include "RemotePidlManager.h"
#include <ATLComTime.h> // For COleDateTime wrapper class

/**
 * Saves back-reference to folder and handle to window.
 * This function increments the folder's reference count to ensure that
 * the folder remains alive for at least as long as the enumerator.
 * The window handle, @p hwndOwner, is used as the parent window for user
 * interaction.  If this is NULL, no user-interaction is allowed and
 * and methods that require it will fail silently
 *
 * @param pFolder    The back-reference to the folder we are enumerating.
 * @param hwndOwner  A handle to the window for user interaction.
 */
HRESULT CRemoteEnumIDList::Initialize(
	CConnection& conn, PCTSTR pszPath, SHCONTF grfFlags )
{
	ATLTRACE("CRemoteEnumIDList::Initialize called\n");

	if (m_fInitialised) // Already called this function
		return E_UNEXPECTED;

	// Save references to both end of the SftpConsumer/Provider connection
	m_pConsumer = conn.spConsumer;
	m_pConsumer->AddRef();
	m_pProvider = conn.spProvider;
	m_pProvider->AddRef();

	m_grfFlags = grfFlags;

	m_fInitialised = true;
	
	return _Fetch( pszPath );
}

#define S_IFMT     0170000 /* type of file */
#define S_IFDIR    0040000 /* directory 'd' */
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::_Fetch
 * Populates the enumerator by connecting to the remote server and fetching 
 * the file listing.
 *
 * TODO: Ideally, the final EnumIDList will deal with enumeration *only*.
 *       Connection and retrieval will be handled by other objects.
 *----------------------------------------------------------------------------*/
HRESULT CRemoteEnumIDList::_Fetch( PCTSTR pszPath )
{
	// Must call Initialize() first
	if (!m_fInitialised) return E_UNEXPECTED;

	HRESULT hr;

	// Interpret supported SHCONTF flags
	bool fIncludeFolders = (m_grfFlags & SHCONTF_FOLDERS) != 0;
	bool fIncludeNonFolders = (m_grfFlags & SHCONTF_NONFOLDERS) != 0;
	bool fIncludeHidden = (m_grfFlags & SHCONTF_INCLUDEHIDDEN) != 0;

	// Get listing enumerator
	IEnumListing *pEnum;
	CComBSTR bstrPath = pszPath;
	hr = m_pProvider->GetListing(bstrPath, &pEnum);
	if (SUCCEEDED(hr))
	{
		do {
			Listing lt;
			hr = pEnum->Next(1, &lt, NULL);
			if (hr == S_OK)
			{
				if (!fIncludeFolders && S_ISDIR(lt.uPermissions))
					continue;
				if (!fIncludeNonFolders && !S_ISDIR(lt.uPermissions))
					continue;
				if (!fIncludeHidden && (lt.bstrFilename[0] == OLECHAR('.')))
					continue;

				FILEDATA fd;
				fd.strFilename = lt.bstrFilename;
				fd.strOwner = lt.bstrOwner;
				fd.strGroup = lt.bstrGroup;
				fd.uSize = lt.cSize;
				fd.dtModified = _ConvertDate(lt.dateModified);
				fd.dwPermissions = lt.uPermissions;
				fd.fIsFolder = S_ISDIR(lt.uPermissions);
				m_vListing.push_back( fd );
			}
		} while (hr == S_OK);
		pEnum->Release();
	}

	// Release data provider component (should destroy psftp process)
	//pProvider->Release();

	return hr;
}

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::Next
 * Retrieves the specified number of item identifiers in the enumeration
 * sequence and advances the current position by the number of items retrieved.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteEnumIDList::Next(
	ULONG celt,
	__out_ecount_part(celt, *pceltFetched) PITEMID_CHILD *rgelt,
	__out_opt ULONG *pceltFetched )
{
	ATLTRACE("CRemoteEnumIDList::Next called\n");
	
	// Must call Initialize() first
	if (!m_fInitialised) return E_UNEXPECTED;
	if (!(pceltFetched || celt <= 1)) return E_INVALIDARG;

	HRESULT hr = S_OK;
	ULONG cFetched = 0;
	for (ULONG i = 0; i < celt; i++)
	{
		ULONG index = m_iPos + i; // Existing offset + current retrieval
		if (index >= m_vListing.size())
		{
			// Ran out of entries before requested number could be fetched
			hr = S_FALSE;
			break;
		}

		// Fetch data and create new PIDL from it
		ATLASSERT( index < m_vListing.size() );
		hr = m_PidlManager.Create(
			m_vListing[index].strFilename,
			m_vListing[index].strOwner,
			m_vListing[index].strGroup,
			m_vListing[index].dwPermissions,
			m_vListing[index].uSize,
			m_vListing[index].dtModified,
			m_vListing[index].fIsFolder,
			&rgelt[i]
		);
		if (FAILED(hr))
			break;
		cFetched++;
	}

	*pceltFetched = cFetched;
	m_iPos += cFetched;
	return hr;
}

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::Skip
 * Skips the specified number of elements in the enumeration sequence.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteEnumIDList::Skip( DWORD celt )
{
	ATLTRACE("CRemoteEnumIDList::Skip called\n");

	// Must call Initialize() first
	if (!m_fInitialised) return E_UNEXPECTED;

	m_iPos += celt;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::Reset
 * Returns to the beginning of the enumeration sequence.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteEnumIDList::Reset()
{
	ATLTRACE("CRemoteEnumIDList::Reset called\n");

	// Must call Initialize() first
	if (!m_fInitialised) return E_UNEXPECTED;

	m_iPos = 0;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::Clone
 * Creates a new item enumeration object with the same contents and state 
 * as the current one.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteEnumIDList::Clone( __deref_out IEnumIDList **ppEnum )
{
	ATLTRACE("CRemoteEnumIDList::Clone called\n");
	(void)ppEnum;

	// Must call Initialize() first
	if (!m_fInitialised) return E_UNEXPECTED;

	// TODO: Implement this

	return E_NOTIMPL;
}

/*----------------------------------------------------------------------------*
 * Private functions
 *----------------------------------------------------------------------------*/

/**
 * Converts DATE to time_t (__time64_t).
 * 
 * A time_t represents number of seconds elapsed since 1970-01-01T00:00:00Z GMT.
 * Valid range is [1970-01-01T00:00:00Z, 3000-12-31T23:59:59Z] GMT.
 *
 * @param dateValue The DATE to be converted.
 * @returns The specified calendar time encoded as a value of type time_t or 
 *          -1 if time is out of range.
 */
time_t CRemoteEnumIDList::_ConvertDate( DATE dateValue ) const
{
	tm tmResult;
	SYSTEMTIME  stTemp;

	if(!::VariantTimeToSystemTime(dateValue, &stTemp))
		return -1;

	tmResult.tm_sec   = stTemp.wSecond;     // seconds after the minute [0, 61]
	tmResult.tm_min   = stTemp.wMinute;     // minutes after the hour   [0, 59]
	tmResult.tm_hour  = stTemp.wHour;       // hours after 00:00:00     [0, 23]
	tmResult.tm_mday  = stTemp.wDay;        // day of the month         [1, 31]
	tmResult.tm_mon   = stTemp.wMonth - 1;  // month of the year        [0, 11]
	tmResult.tm_year  = stTemp.wYear - 1900;// years since 1900
	tmResult.tm_wday  = stTemp.wDayOfWeek;  // day of the week          [0,  6]
	tmResult.tm_isdst = -1;                 // Daylight saving time is unknown
	return _mktime64(&tmResult); // Convert the incomplete time to a normalised
						         // calendar value.
}

// CRemoteEnumIDList
