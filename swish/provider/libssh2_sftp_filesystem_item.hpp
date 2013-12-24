/**
    @file

    SFTP filesystem item using libssh2 backend.

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

#ifndef SWISH_PROVIDER_LIBSSH2_SFTP_FILESYSTEM_ITEM_HPP
#define SWISH_PROVIDER_LIBSSH2_SFTP_FILESYSTEM_ITEM_HPP

#include "swish/provider/sftp_filesystem_item.hpp"
#include "swish/provider/sftp_provider_path.hpp"

#include <boost/cstdint.hpp> // uint64_t
#include <boost/optional.hpp>

#include <comet/datetime.h> // datetime_t

#include <string>

namespace ssh {
namespace filesystem {

class file_attributes;
class sftp_file;

}
}

namespace swish {
namespace provider {

/**
 * An entry in an SFTP directory retrieved by the libssh2 backend.
 */
class libssh2_sftp_filesystem_item : public sftp_filesystem_item_interface
{
public:

    /**
     * Create filesystem entry from libssh2 filesystem item representation.
     */
    static sftp_filesystem_item create_from_libssh2_file(
        const ssh::filesystem::sftp_file& file);

    /**
     * Create filesystem entry from libssh2 filesystem item representation using
     * only the attributes and filename.
     *
     * This constructor is for use with a stat-style situation where the full
     * file info isn't available.
     *
     * Items created with this constructor will *not* be able to return the
     * user name or group name as a string.
     *
     * @param char_blob_file_name
     *        Filename which is usually a UTF-8 string but that's not
     *        guaranteed. At this point all we know is it is a binary blob of
     *        chars.
     *
     * @param attributes
     *        Object containing the file's details.
     */
    static sftp_filesystem_item create_from_libssh2_attributes(
        const std::string& char_blob_file_name,
        const ssh::filesystem::file_attributes& attributes);

    virtual BOOST_SCOPED_ENUM(type) type() const;
    virtual sftp_provider_path filename() const;
    virtual unsigned long permissions() const;
    virtual boost::optional<std::wstring> owner() const;
    virtual unsigned long uid() const;
    virtual boost::optional<std::wstring> group() const;
    virtual unsigned long gid() const;
    virtual boost::uint64_t size_in_bytes() const;
    virtual comet::datetime_t last_accessed() const;
    virtual comet::datetime_t last_modified() const;

private:

    libssh2_sftp_filesystem_item(
        const ssh::filesystem::sftp_file& file);

    libssh2_sftp_filesystem_item(
        const std::string& char_blob_file_name,
        const ssh::filesystem::file_attributes& attributes);

    void common_init(
        const std::string& char_blob_file_name,
        const ssh::filesystem::file_attributes& attributes);

    BOOST_SCOPED_ENUM(type) m_type;
    sftp_provider_path m_path;
    unsigned long m_permissions;
    boost::optional<std::wstring> m_owner;
    boost::optional<std::wstring> m_group;
    unsigned long m_uid;
    unsigned long m_gid;
    boost::uint64_t m_size;
    comet::datetime_t m_modified;
    comet::datetime_t m_accessed;
};

}}

#endif
