/**
    @file

    SFTP directory listing helper functions.

    @if license

    Copyright (C) 2009, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "listing.hpp"

#include "swish/provider/sftp_provider.hpp" // SmartListing
#include "swish/utils.hpp" // Utf8StringToWideString

#include <ATLComTime.h>    // COleDateTime

#include <boost/regex.hpp> // Regular expressions

#include <exception> // exception

using ATL::COleDateTime;

using swish::utils::Utf8StringToWideString;

using comet::bstr_t;

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
bstr_t parse_user_from_long_entry(const string& long_entry)
{
    boost::smatch match;
    if (regex_match(long_entry, match, regex) && match[USER_MATCH].matched)
        return Utf8StringToWideString(match[USER_MATCH].str());
    else
        return comet::bstr_t();
}

/**
 * Get the group name part of an SFTP 'ls -l'-style long entry.
 *
 * @see parse_user_from_long_entry() for more information.
 */
bstr_t parse_group_from_long_entry(const string& long_entry)
{
    boost::smatch match;
    if (regex_match(long_entry, match, regex) && match[GROUP_MATCH].matched)
        return Utf8StringToWideString(match[GROUP_MATCH].str());
    else
        return comet::bstr_t();
}

/**
 * Create Listing file entry object from filename, long entry and attributes.
 *
 * @param utf8_file_name   Filename as UTF-8 string.
 * @param utf8_long_entry  Long (ls -l) form of the file's attributes from
 *                         which we, naughtily, parse the username and
 *                         group.  The standard says we shouldn't do this but
 *                         there is no other way.  UTF-8 encoding.
 * @param attributes       Reference to the LIBSSH2_SFTP_ATTRIBUTES containing
 *                         the file's details.
 *
 * @returns A listing object representing the file.
 */
SmartListing fill_listing_entry(
    const string& utf8_file_name, const string& utf8_long_entry,
    const LIBSSH2_SFTP_ATTRIBUTES& attributes)
{
    SmartListing lt = SmartListing();

    bstr_t file_name;
    bstr_t owner;
    bstr_t group;

    // This function isn't allowed to fail because it would leak BSTRs in
    // a Listing so we catch any exceptions and return and empty string for
    // any fields not yet completed
    try
    {
        // Filename
        file_name = Utf8StringToWideString(utf8_file_name);

        // Permissions
        if (attributes.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS)
        {
            lt.uPermissions = attributes.permissions;
            lt.fIsLink = LIBSSH2_SFTP_S_ISLNK(attributes.permissions);
            lt.fIsDirectory = LIBSSH2_SFTP_S_ISDIR(attributes.permissions);
        }
        else
        {
            lt.uPermissions = 0U;
            lt.fIsLink = FALSE;
            lt.fIsDirectory = FALSE;
        }

        // User & Group
        if (attributes.flags & LIBSSH2_SFTP_ATTR_UIDGID)
        {
            // To be on the safe side assume that the long entry doesn't hold
            // valid owner and group info if the UID and GID aren't valid

            owner = parse_user_from_long_entry(utf8_long_entry);
            group = parse_group_from_long_entry(utf8_long_entry);

            // Numerical fields (UID and GID)
            lt.uUid = attributes.uid;
            lt.uGid = attributes.gid;
        }
        else
        {
            lt.uUid = 0U;
            lt.uGid = 0U;
        }

        // Size of file
        if (attributes.flags & LIBSSH2_SFTP_ATTR_SIZE)
        {
            lt.uSize = attributes.filesize;
        }
        else
        {
            lt.uSize = 0;
        }

        // Access & Modification time
        if (attributes.flags & LIBSSH2_SFTP_ATTR_ACMODTIME)
        {
            COleDateTime dateModified(static_cast<time_t>(attributes.mtime));
            COleDateTime dateAccessed(static_cast<time_t>(attributes.atime));
            lt.dateModified = dateModified;
            lt.dateAccessed = dateAccessed;
        }
        else
        {
            lt.dateAccessed = 0U;
            lt.dateModified = 0U;
        }
    }
    catch (const std::exception&) { /* ignore */ }

    lt.bstrFilename = bstr_t::detach(file_name);
    lt.bstrOwner = bstr_t::detach(owner);
    lt.bstrGroup = bstr_t::detach(group);

    return lt;
}

}}} // namespace swish::provider::listing