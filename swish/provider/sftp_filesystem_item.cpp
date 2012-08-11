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

#include "sftp_filesystem_item.hpp"

#include <OleAuto.h> // SysAllocStringLen, SysStringLen, SysFreeString,
                     // VarBstrCmp

namespace swish {
namespace provider {

sftp_filesystem_item::sftp_filesystem_item()
{
    bstrFilename = NULL;
    uPermissions = 0U;
    bstrOwner = NULL;
    bstrGroup = NULL;
    uUid = 0U;
    uGid = 0U;
    uSize = 0U;
    dateModified = 0;
    dateAccessed = 0;
    fIsDirectory = FALSE;
    fIsLink = FALSE;
}

sftp_filesystem_item::sftp_filesystem_item(const sftp_filesystem_item& other)
{
    bstrFilename = ::SysAllocStringLen(
        other.bstrFilename, ::SysStringLen(other.bstrFilename));
    uPermissions = other.uPermissions;
    bstrOwner = ::SysAllocStringLen(
        other.bstrOwner, ::SysStringLen(other.bstrOwner));
    bstrGroup = ::SysAllocStringLen(
        other.bstrGroup, ::SysStringLen(other.bstrGroup));
    uUid = other.uUid;
    uGid = other.uGid;
    uSize = other.uSize;
    dateModified = other.dateModified;
    dateAccessed = other.dateAccessed;
    fIsDirectory = other.fIsDirectory;
    fIsLink = other.fIsLink;
}

sftp_filesystem_item& sftp_filesystem_item::operator=(
    const sftp_filesystem_item& other)
{
    sftp_filesystem_item copy(other);
    swap(*this, copy);
    return *this;
}

sftp_filesystem_item::~sftp_filesystem_item()
{
    ::SysFreeString(bstrFilename);
    ::SysFreeString(bstrGroup);
    ::SysFreeString(bstrOwner);
}

bool sftp_filesystem_item::operator<(const sftp_filesystem_item& other) const
{
    if (bstrFilename == 0)
        return other.bstrFilename != 0;

    if (other.bstrFilename == 0)
        return false;

    return ::VarBstrCmp(
        bstrFilename, other.bstrFilename,
        ::GetThreadLocale(), 0) == VARCMP_LT;
}

bool sftp_filesystem_item::operator==(const sftp_filesystem_item& other) const
{
    if (bstrFilename == 0 && other.bstrFilename == 0)
        return true;

    return ::VarBstrCmp(
        bstrFilename, other.bstrFilename,
        ::GetThreadLocale(), 0) == VARCMP_EQ;
}

bool sftp_filesystem_item::operator==(const comet::bstr_t& name) const
{
    return bstrFilename == name;
}

}}
