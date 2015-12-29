/**
    @file

    Host-key wrapper.

    @if license

    Copyright (C) 2010, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SSH_HOST_KEY_HPP
#define SSH_HOST_KEY_HPP

#include <ssh/detail/session_state.hpp>

#include <boost/foreach.hpp>         // BOOST_FOREACH
#include <boost/shared_ptr.hpp>      // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <sstream>   // ostringstream
#include <stdexcept> // invalid_argument
#include <string>
#include <utility> // pair
#include <vector>

#include <libssh2.h>

namespace ssh
{

namespace detail
{

/**
 * Thin wrapper around libssh2_session_hostkey.
 */
inline std::pair<std::string, int> hostkey(session_state& session)
{
    // Session owns the string.
    // Lock until we finish copying the key string from the session.  I
    // don't know if other calls to the session are currently able to
    // change it, but they might one day.
    // Locking it for the duration makes it thread-safe either way.

    detail::session_state::scoped_lock lock = session.aquire_lock();

    size_t len = 0;
    int type = LIBSSH2_HOSTKEY_TYPE_UNKNOWN;
    const char* key =
        libssh2_session_hostkey(session.session_ptr(), &len, &type);

    if (key)
        return std::make_pair(std::string(key, len), type);
    else
        return std::make_pair(std::string(), type);
}

/**
 * Thin wrapper around libssh2_hostkey_hash.
 *
 * @param T          Type of collection to return.  Sensible examples
 *                   include std::string or std::vector<unsigned char>.
 * @param session    libssh2 session pointer
 * @param hash_type  Hash method being requested.
 */
template <typename T>
inline T hostkey_hash(session_state& session, int hash_type)
{
    // Session owns the data.
    // Lock until we finish copying the key hash bytes from the session.  I
    // don't know if other calls to the session are currently able to
    // change it, but they might one day.
    // Locking it for the duration makes it thread-safe either way.

    detail::session_state::scoped_lock lock = session.aquire_lock();

    const T::value_type* hash_bytes = reinterpret_cast<const T::value_type*>(
        ::libssh2_hostkey_hash(session.session_ptr(), hash_type));

    size_t len = 0;
    if (hash_type == LIBSSH2_HOSTKEY_HASH_MD5)
        len = 16;
    else if (hash_type == LIBSSH2_HOSTKEY_HASH_SHA1)
        len = 20;
    else
        BOOST_THROW_EXCEPTION(std::invalid_argument("Unknown hash type"));

    if (hash_bytes)
        return T(hash_bytes, hash_bytes + len);
    else
        return T();
}

/**
 * Thin wrapper around libssh2_session_methods.
 */
inline std::string method(session_state& session, int method_type)
{
    // Session owns the string.
    // Lock until we finish copying the string from the session.  I
    // don't know if other calls to the session are currently able to
    // change it, but they might one day.
    // Locking it for the duration makes it thread-safe either way.

    detail::session_state::scoped_lock lock = session.aquire_lock();

    const char* key_type =
        libssh2_session_methods(session.session_ptr(), method_type);

    if (key_type)
        return std::string(key_type);
    else
        return std::string();
}
}

/**
 * Possible types of host-key algorithm.
 */
struct hostkey_type
{
    enum enum_t
    {
        unknown,
        rsa1,
        ssh_rsa,
        ssh_dss
    };
};

namespace detail
{

/**
 * Convert the returned key-type from libssh2_session_hostkey into a value from
 * the hostkey_type enum.
 */
inline hostkey_type::enum_t type_to_hostkey_type(int type)
{
    switch (type)
    {
    case LIBSSH2_HOSTKEY_TYPE_RSA:
        return hostkey_type::ssh_rsa;
    case LIBSSH2_HOSTKEY_TYPE_DSS:
        return hostkey_type::ssh_dss;
    default:
        return hostkey_type::unknown;
    }
}
}

/**
 * Class representing the session's current negotiated host-key.
 *
 * As well as the raw key itself, this class provides MD5 and SHA1 hashes and
 * key metadata.
 */
class host_key
{
public:
    explicit host_key(detail::session_state& session)
        : // We pull everything out of the session here and store it to avoid
          // instances of this class depending on the lifetime of the session
          m_key(detail::hostkey(session)),
          m_algorithm_name(detail::method(session, LIBSSH2_METHOD_HOSTKEY)),
          m_md5_hash(detail::hostkey_hash<std::vector<unsigned char>>(
              session, LIBSSH2_HOSTKEY_HASH_MD5)),
          m_sha1_hash(detail::hostkey_hash<std::vector<unsigned char>>(
              session, LIBSSH2_HOSTKEY_HASH_SHA1))
    {
    }

    /**
     * Host-key either raw or base-64 encoded.
     *
     * @see is_base64()
     */
    std::string key() const
    {
        return m_key.first;
    }

    /**
     * Is the key returned by key() base64-encoded (printable)?
     */
    bool is_base64() const
    {
        return false;
    }

    /**
     * Type of the key algorithm e.g., ssh-dss.
     */
    hostkey_type::enum_t algorithm() const
    {
        return detail::type_to_hostkey_type(m_key.second);
    }

    /**
     * Printable name of the method negotiated for the key algorithm.
     */
    std::string algorithm_name() const
    {
        return m_algorithm_name;
    }

    /**
     * Hostkey sent by the server to identify itself, hashed with the MD5
     * algorithm.
     *
     * @returns  Hash as binary data; it is not directly printable
     *           (@see hexify()).
     */
    std::vector<unsigned char> md5_hash() const
    {
        return m_md5_hash;
    }

    /**
     * Hostkey sent by the server to identify itself, hashed with the SHA1
     * algorithm.
     *
     * @returns  Hash as binary data; it is not directly printable
     *           (@see hexify()).
     */
    std::vector<unsigned char> sha1_hash() const
    {
        return m_sha1_hash;
    }

private:
    std::pair<std::string, int> m_key;
    std::string m_algorithm_name;
    std::vector<unsigned char> m_md5_hash;
    std::vector<unsigned char> m_sha1_hash;
};

/**
 * Turn a collection of bytes into a printable hexidecimal string.
 *
 * @param bytes       Collection of bytes.
 * @param nibble_sep  String to place between each pair of hexidecimal
 *                    characters.
 * @param uppercase   Whether to use uppercase or lowercase hexidecimal.
 */
template <typename T>
std::string hexify(const T& bytes, const std::string& nibble_sep = ":",
                   bool uppercase = false)
{
    std::ostringstream hex_hash;

    if (uppercase)
        hex_hash << std::uppercase;
    else
        hex_hash << std::nouppercase;
    hex_hash << std::hex << std::setfill('0');

    BOOST_FOREACH (unsigned char b, bytes)
    {
        if (!hex_hash.str().empty())
            hex_hash << nibble_sep;

        unsigned int i = b;
        hex_hash << std::setw(2) << i;
    }

    return hex_hash.str();
}

} // namespace ssh

#endif
