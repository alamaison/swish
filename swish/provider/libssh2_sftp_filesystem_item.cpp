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

#include "libssh2_sftp_filesystem_item.hpp"

#include "swish/utils.hpp" // Utf8StringToWideString

#include <ssh/sftp.hpp> // file_attributes, sftp_file

#include <boost/regex.hpp> // Regular expressions
#include <boost/shared_ptr.hpp>

using swish::utils::Utf8StringToWideString;

using comet::datetime_t;

using ssh::sftp::file_attributes;
using ssh::sftp::sftp_file;

using boost::optional;
using boost::shared_ptr;
using boost::uint64_t;

using std::string;
using std::wstring;

namespace {

    const boost::regex regex("\\S{10,}\\s+\\d+\\s+(\\S+)\\s+(\\S+)\\s+.+");
    const unsigned int USER_MATCH = 1;
    const unsigned int GROUP_MATCH = 2;
        
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
     * where the second line shows the *minimum* number of characters.
     *
     * @warning
     * The spec specifically forbids parsing this long entry by it is the
     * only way to get the user @b name rather than the user @b ID.
     */
    optional<wstring> parse_user_from_long_entry(const string& long_entry)
    {
        boost::smatch match;
        if (regex_match(long_entry, match, regex) && match[USER_MATCH].matched)
        {
            return Utf8StringToWideString(match[USER_MATCH].str());
        }
        else
        {
            return optional<wstring>();
        }
    }

    /**
     * Get the group name part of an SFTP 'ls -l'-style long entry.
     *
     * @see parse_user_from_long_entry() for more information.
     */
    optional<wstring> parse_group_from_long_entry(const string& long_entry)
    {
        boost::smatch match;
        if (regex_match(long_entry, match, regex) && match[GROUP_MATCH].matched)
        {
            return Utf8StringToWideString(match[GROUP_MATCH].str());
        }
        else
        {
            return optional<wstring>();
        }
    }

}

namespace swish {
namespace provider {

sftp_filesystem_item
libssh2_sftp_filesystem_item::create_from_libssh2_attributes(
    const string& char_blob_file_name, const file_attributes& attributes)
{
    return sftp_filesystem_item(
        shared_ptr<sftp_filesystem_item_interface>(
            new libssh2_sftp_filesystem_item(char_blob_file_name, attributes)));
}

sftp_filesystem_item
libssh2_sftp_filesystem_item::create_from_libssh2_file(const sftp_file& file)
{
    return sftp_filesystem_item(
        shared_ptr<sftp_filesystem_item_interface>(
            new libssh2_sftp_filesystem_item(file)));
}


void libssh2_sftp_filesystem_item::common_init(
    const string& char_blob_file_name, const file_attributes& attributes)
{
    // FIXME: this filename may not be UTF-8 but we're blindly treating
    // it as though it were - should autodetect if possible
    m_path = Utf8StringToWideString(char_blob_file_name);

    switch (attributes.type())
    {
    case file_attributes::normal_file:
        m_type = type::file;
        break;

    case file_attributes::directory:
        m_type = type::directory;
        break;

    case file_attributes::symbolic_link:
        m_type = type::link;
        break;

    default:
        m_type = type::unknown;
    }

    if (attributes.permissions())
    {
        m_permissions = *attributes.permissions();
    }

    if (attributes.size())
    {
        m_size = *attributes.size();
    }

    if (attributes.uid())
    {
        m_uid = *attributes.uid();
    }

    if (attributes.gid())
    {
        m_gid = *attributes.gid();
    }

    if (attributes.last_accessed())
    {
        m_accessed.from_unixtime(
            static_cast<time_t>(*attributes.last_accessed()),
            datetime_t::ucm_none);
    }

    if (attributes.last_modified())
    {
        m_modified.from_unixtime(
            static_cast<time_t>(*attributes.last_modified()),
            datetime_t::ucm_none);
    }
}

libssh2_sftp_filesystem_item::libssh2_sftp_filesystem_item(
    const sftp_file& file)
    :
m_type(type::unknown), m_permissions(0U), m_uid(0U), m_gid(0U), m_size(0U)
{
    file_attributes attributes = file.attributes();

    common_init(file.name(), attributes);

    // Naughtily, we parse the long (ls -l) form of the file's attributes
    // for the username and group.  The standard says we shouldn't but
    // there's no other way to get them as text.  Although it contains a copy
    // the filename, which may not be in UTF-8 encoding, we treat this
    // long form as a UTF-8 string as the other info /should/ be UTF-8 and we
    // don't use the filename.

    // To be on the safe side assume that the long entry doesn't hold
    // valid owner and group info if the UID and GID aren't valid

    string utf8_long_entry = file.long_entry();

    if (attributes.uid())
    {
        m_owner = parse_user_from_long_entry(utf8_long_entry);
    }

    if (attributes.gid())
    {
        m_group = parse_group_from_long_entry(utf8_long_entry);
    }
}

libssh2_sftp_filesystem_item::libssh2_sftp_filesystem_item(
    const string& char_blob_file_name, const file_attributes& attributes)
    :
m_type(type::unknown), m_permissions(0U), m_uid(0U), m_gid(0U), m_size(0U)
{
    common_init(char_blob_file_name, attributes);
}

BOOST_SCOPED_ENUM(sftp_filesystem_item_interface::type)
libssh2_sftp_filesystem_item::type() const
{
    return m_type;
}

sftp_provider_path libssh2_sftp_filesystem_item::filename() const
{
    return m_path.filename();
}

unsigned long libssh2_sftp_filesystem_item::permissions() const
{
    return m_permissions;
}

optional<wstring> libssh2_sftp_filesystem_item::owner() const
{
    return m_owner;
}

unsigned long libssh2_sftp_filesystem_item::uid() const
{
    return m_uid;
}

optional<wstring> libssh2_sftp_filesystem_item::group() const
{
    return m_group;
}

unsigned long libssh2_sftp_filesystem_item::gid() const
{
    return m_gid;
}

uint64_t libssh2_sftp_filesystem_item::size_in_bytes() const
{
    return m_size;
}

datetime_t libssh2_sftp_filesystem_item::last_accessed() const
{
    return m_accessed;
}

datetime_t libssh2_sftp_filesystem_item::last_modified() const
{
    return m_modified;
}

/*
bool libssh2_sftp_filesystem_item::operator<(const libssh2_sftp_filesystem_item& other) const
{
    if (bstrFilename == 0)
        return other.bstrFilename != 0;

    if (other.bstrFilename == 0)
        return false;

    return ::VarBstrCmp(
        bstrFilename, other.bstrFilename,
        ::GetThreadLocale(), 0) == VARCMP_LT;
}

bool libssh2_sftp_filesystem_item::operator==(const libssh2_sftp_filesystem_item& other) const
{
    if (bstrFilename == 0 && other.bstrFilename == 0)
        return true;

    return ::VarBstrCmp(
        bstrFilename, other.bstrFilename,
        ::GetThreadLocale(), 0) == VARCMP_EQ;
}

bool libssh2_sftp_filesystem_item::operator==(const comet::bstr_t& name) const
{
    return bstrFilename == name;
}
*/

}}
