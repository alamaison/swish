/**
    @file

    SFTP backend filesystem item.

    @if license

    Copyright (C) 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    If you modify this Program, or any covered work, by linking or
    combining it with the OpenSSL project's OpenSSL library (or a
    modified version of that library), containing parts covered by the
    terms of the OpenSSL or SSLeay licenses, the licensors of this
    Program grant you additional permission to convey the resulting work.

    @endif
*/

#ifndef SWISH_PROVIDER_SFTP_FILESYSTEM_ITEM_HPP
#define SWISH_PROVIDER_SFTP_FILESYSTEM_ITEM_HPP

#include <OleAuto.h> // BSTR, DATE

namespace swish {
namespace provider {

/**
 * An entry in an SFTP directory.
 */
class sftp_filesystem_item
{
public:

    sftp_filesystem_item();

    sftp_filesystem_item(const sftp_filesystem_item& other);

    sftp_filesystem_item& operator=(const sftp_filesystem_item& other);

    ~sftp_filesystem_item();

    bool operator<(const sftp_filesystem_item& other) const;

    bool operator==(const sftp_filesystem_item& other) const;

    bool operator==(const comet::bstr_t& name) const;

    friend void swap(sftp_filesystem_item& lhs, sftp_filesystem_item& rhs)
    {
        std::swap(lhs.bstrFilename, rhs.bstrFilename);
        std::swap(lhs.uPermissions, rhs.uPermissions);
        std::swap(lhs.bstrOwner, rhs.bstrOwner);
        std::swap(lhs.bstrGroup, rhs.bstrGroup);
        std::swap(lhs.uUid, rhs.uUid);
        std::swap(lhs.uGid, rhs.uGid);
        std::swap(lhs.uSize, rhs.uSize);
        std::swap(lhs.dateModified, rhs.dateModified);
        std::swap(lhs.dateAccessed, rhs.dateAccessed);
        std::swap(lhs.fIsDirectory, rhs.fIsDirectory);
        std::swap(lhs.fIsLink, rhs.fIsLink);
    }

    BSTR bstrFilename;    ///< Directory-relative filename (e.g. README.txt)
    ULONG uPermissions;   ///< Unix file permissions
    BSTR bstrOwner;       ///< The user name of the file's owner
    BSTR bstrGroup;       ///< The name of the group to which the file belongs
    ULONG uUid;           ///< Numerical ID of file's owner
    ULONG uGid;           ///< Numerical ID of group to which the file belongs
    ULONGLONG uSize;      ///< The file's size in bytes
    DATE dateModified;    ///< The date and time at which the file was 
                          ///< last modified in automation-compatible format
    DATE dateAccessed;    ///< The date and time at which the file was 
                          ///< last accessed in automation-compatible format
    BOOL fIsDirectory;    ///< This filesystem item can be listed for items
                          ///< under it.
    BOOL fIsLink;         ///< This file is a link to another file or directory
};

}}

#endif
