/*  Manage remote directory as a collection of PIDLs.

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
#include "SftpDirectory.h"

/**
 * Initialises directory instance with Connection to be used and enum flags.
 *
 * @param conn      The connection to be used to communicate with server.
 * @param grfFlags  The type of files to include in the enumeration as well
 *                  as other enumeration properties.
 */
HRESULT CSftpDirectory::Initialize( CConnection& conn, SHCONTF grfFlags )
{
	if (m_fInitialised) // Already called this function
		return E_UNEXPECTED;

	// Save references to both end of the SftpConsumer/Provider connection
	m_spConsumer = conn.spConsumer;
	m_spProvider = conn.spProvider;

	m_grfFlags = grfFlags;

	m_fInitialised = true;
	
	return S_OK;
}

#define S_IFMT     0170000 /* type of file */
#define S_IFDIR    0040000 /* directory 'd' */
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

/**
 * Fetches directory listing from the server.
 *
 * This listing is stored as a collection of PIDLs in the m_vecPidls member.
 *
 * @param pszPath  The remote filesystem path whose listing is to be fetched.
 */
HRESULT CSftpDirectory::Fetch( PCTSTR pszPath )
{
	if (!m_fInitialised) return E_UNEXPECTED; // Must call Initialize() first

	HRESULT hr;

	// Interpret supported SHCONTF flags
	bool fIncludeFolders = (m_grfFlags & SHCONTF_FOLDERS) != 0;
	bool fIncludeNonFolders = (m_grfFlags & SHCONTF_NONFOLDERS) != 0;
	bool fIncludeHidden = (m_grfFlags & SHCONTF_INCLUDEHIDDEN) != 0;

	// Get listing enumerator
	CComPtr<IEnumListing> spEnum;
	hr = m_spProvider->GetListing(CComBSTR(pszPath), &spEnum);
	if (SUCCEEDED(hr))
	{
		do {
			Listing lt;
			hr = spEnum->Next(1, &lt, NULL);
			if (hr == S_OK)
			{
				if (!fIncludeFolders && S_ISDIR(lt.uPermissions))
					continue;
				if (!fIncludeNonFolders && !S_ISDIR(lt.uPermissions))
					continue;
				if (!fIncludeHidden && (lt.bstrFilename[0] == OLECHAR('.')))
					continue;

				PITEMID_CHILD pidl;
				hr = m_PidlManager.Create(
					CString(lt.bstrFilename),
					CString(lt.bstrOwner),
					CString(lt.bstrGroup),
					lt.uPermissions,
					lt.cSize,
					_ConvertDate(lt.dateModified),
					S_ISDIR(lt.uPermissions),
					&pidl
				);
				ATLASSERT(SUCCEEDED(hr));
				if (SUCCEEDED(hr))
				{
					m_vecPidls.push_back(pidl);
				}

				::SysFreeString(lt.bstrFilename);
				::SysFreeString(lt.bstrGroup);
				::SysFreeString(lt.bstrOwner);
			}
		} while (hr == S_OK);
	}

	return hr;
}

/**
 * Retrieve an IEnumIDList to enumerate directory contents.
 *
 * This function returns an enumerator which can be used to iterate through
 * the contents of the directory as a series of PIDLs.  This contents
 * must have been previously obtaqined from the server by a call to Fetch().
 *
 * @param [out]  The location in which to return the IEnumIDList.
 */
HRESULT CSftpDirectory::GetEnum( IEnumIDList **ppEnumIDList )
{
	typedef CComEnumOnSTL<IEnumIDList, &__uuidof(IEnumIDList), PITEMID_CHILD,
	                      _CopyChildPidl, vector<PITEMID_CHILD> >
	        CComEnumIDList;

	HRESULT hr;

	CComObject<CComEnumIDList> *pEnumIDList;
	hr = CComObject<CComEnumIDList>::CreateInstance(&pEnumIDList);
	ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);

	pEnumIDList->AddRef();

	hr = pEnumIDList->Init( this->GetUnknown(), m_vecPidls );
	ATLASSERT(SUCCEEDED(hr));
	if (SUCCEEDED(hr))
	{
		hr = pEnumIDList->QueryInterface(ppEnumIDList);
		ATLASSERT(SUCCEEDED(hr));
	}

	pEnumIDList->Release();

	return hr;
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
time_t CSftpDirectory::_ConvertDate( DATE dateValue ) const
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

// CSftpDirectory

