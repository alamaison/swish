/**
    @file

    SFTP backend filesystem item interface.

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

#include <ssh/path.hpp>

#include <boost/cstdint.hpp> // uint64_t
#include <boost/detail/scoped_enum_emulation.hpp> // BOOST_SCOPED_ENUM
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include <comet/datetime.h> // datetime_t

#include <string>


namespace swish {
namespace provider {

/**
 * Interface to Swish's representation of an SFTP file's properties.
 *
 * All attributes are technically optional according to the SFTP standard
 * (i.e. the server could set the flags to say the returned value isn't valid),
 * but, to simplify things inside Swish, we only make this optionality
 * explicit for `owner` and `group` as they are the only ones with a realistic
 * prospect of not being supported.  The others have sensible defaults.
 */
class sftp_filesystem_item_interface
{
public:

    BOOST_SCOPED_ENUM_START(type)
    {
        /// File that can be opened and whose contents can be accessed
        /// (permissions permitting).
        file,

        /// This filesystem item can be listed for items under it.
        directory,

        /// This file is a link to another item
        link,

        /// An item of a type we don't recognise or the server didn't send any
        /// information about the type
        unknown
    };
    BOOST_SCOPED_ENUM_END();

    /// Type of item represented by this object.
    virtual BOOST_SCOPED_ENUM(type) type() const = 0;

    /// Filename relative to directory (e.g. `README.txt`).
    virtual ssh::filesystem::path filename() const = 0;

    /// Unix file permissions.
    virtual unsigned long permissions() const = 0;

    /// The user name of the file's owner.
    /// This may not exist if the server doesn't report named users.  This may
    /// also be incorrect if the server responds in an unusual way so should
    /// only be used for information.
    virtual boost::optional<std::wstring> owner() const = 0;

    /// Numeric ID of file's owner.
    virtual unsigned long uid() const = 0;

    /// The name of the user group to which the file belongs
    /// This may not exist if the server doesn't report named groups.  This may
    /// also be incorrect if the server responds in an unusual way so should
    /// only be used for information.
    virtual boost::optional<std::wstring> group() const = 0;

    /// Numeric ID of group to which the file belongs.
    virtual unsigned long gid() const = 0;

    /// The file's size in bytes.
    virtual boost::uint64_t size_in_bytes() const = 0;

    /// The date and time at which the file was last accessed.
    virtual comet::datetime_t last_accessed() const = 0;

    /// The date and time at which the file was last modified.
    virtual comet::datetime_t last_modified() const = 0;
};

/**
 * Type erasure interface to SFTP representation implementations.
 */
class sftp_filesystem_item : public sftp_filesystem_item_interface
{
public:

    BOOST_SCOPED_ENUM(type) type() const
    { return m_inner->type(); }

    ssh::filesystem::path filename() const
    { return m_inner ->filename(); }

    unsigned long permissions() const
    { return m_inner->permissions(); }

    boost::optional<std::wstring> owner() const
    { return m_inner->owner(); }

    unsigned long uid() const
    { return m_inner->uid(); }

    boost::optional<std::wstring> group() const
    { return m_inner->group(); }

    unsigned long gid() const
    { return m_inner->gid(); }

    boost::uint64_t size_in_bytes() const
    { return m_inner->size_in_bytes(); }

    comet::datetime_t last_accessed() const
    { return m_inner->last_accessed(); }

    comet::datetime_t last_modified() const
    { return m_inner->last_modified(); }

    explicit sftp_filesystem_item(
        boost::shared_ptr<sftp_filesystem_item_interface> inner)
        : m_inner(inner) {}

private:
    boost::shared_ptr<sftp_filesystem_item_interface> m_inner;
};

}}

#endif
