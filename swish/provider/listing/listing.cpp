/**
    @file

    SFTP directory listing helper functions.

    @if licence

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.

    @endif
*/

#include "pch.h"
#include "listing.hpp"

#include <ATLComTime.h>    // COleDateTime

#include <boost/regex.hpp> // Regular expressions

using ATL::CComBSTR;
using ATL::COleDateTime;

using std::string;


namespace { // private

	const boost::regex regex("\\S{10,}\\s+\\d+\\s+(\\S+)\\s+(\\S+)\\s+.+");
	const unsigned int USER_MATCH = 1;
	const unsigned int GROUP_MATCH = 2;
}


namespace swish {
namespace provider {
namespace listing {

/**
 * Get the username part of an SFTP 'ls -l'-style long entry.
 *
 * According to the specification
 * (http://www.openssh.org/txt/draft-ietf-secsh-filexfer-02.txt):
 *
 * The recommended format for the longname field is as follows:
 *
 *     -rwxr-xr-x   1 mjos     staff      348911 Mar 25 14:29 t-filexfer
 *     1234567890 123 12345678 12345678 12345678 123456789012
 *
 * where the second line shows the @b minimum number of characters.
 *
 * @warning
 * The spec specifically forbids parsing this long entry by it is the
 * only way to get the user @b name rather than the user @b ID.
 */
CComBSTR ParseUserFromLongEntry(string longentry)
{
	boost::smatch match;
	if (regex_match(longentry, match, regex) 
		&& match[USER_MATCH].matched)
	{
		return CComBSTR(match[USER_MATCH].str().c_str());
	}

	return L"";
}

/**
 * Get the group name part of an SFTP 'ls -l'-style long entry.
 *
 * @see ParseUserFromLongEntry() for more information.
 */
CComBSTR ParseGroupFromLongEntry(string longentry)
{
	boost::smatch match;
	if (regex_match(longentry, match, regex) 
		&& match[GROUP_MATCH].matched)
	{
		return CComBSTR(match[GROUP_MATCH].str().c_str());
	}

	return L"";
}

/**
 * Create Listing file entry object from filename, long entry and attributes.
 *
 * @param strFilename   Filename as an ANSI string.
 * @param pszLongEntry  Long (ls -l) form of the file's attributes from
 *                      which we, naughtily, parse the username and
 *                      group.  The standard says we shouldn't do this but
 *                      there is no other way.
 * @param attrs         Reference to the LIBSSH2_SFTP_ATTRIBUTES containing
 *                      the file's details.
 *
 * @returns A listing object representing the file.
 */
Listing FillListingEntry(
	const string& strFilename, const string& strLongEntry,
	LIBSSH2_SFTP_ATTRIBUTES& attrs)
{
	Listing lt;
	::ZeroMemory(&lt, sizeof(Listing));

	// Filename
	CComBSTR bstrFile(strFilename.c_str());
	HRESULT hr = bstrFile.CopyTo(&(lt.bstrFilename));
	ATLASSERT(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		lt.bstrFilename = ::SysAllocString(OLESTR(""));
	}

	// Permissions
	if (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS)
	{
		lt.uPermissions = attrs.permissions;
	}

	// User & Group
	if (attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID)
	{
		// String fields
		lt.bstrOwner = ::SysAllocString(ParseUserFromLongEntry(strLongEntry));
		lt.bstrGroup = ::SysAllocString(ParseGroupFromLongEntry(strLongEntry));

		// Numerical fields (UID and GID)
		lt.uUid = attrs.uid;
		lt.uGid = attrs.gid;
	}

	// Size of file
	if (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE)
	{
		lt.uSize = attrs.filesize;
	}

	// Access & Modification time
	if (attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME)
	{
		COleDateTime dateModified(static_cast<time_t>(attrs.mtime));
		COleDateTime dateAccessed(static_cast<time_t>(attrs.atime));
		lt.dateModified = dateModified;
		lt.dateAccessed = dateAccessed;
	}

	return lt;
}

}}} // namespace swish::provider::listing