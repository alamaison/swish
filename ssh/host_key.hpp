/**
    @file

    Host-key wrapper.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#pragma once

#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <sstream> // ostringstream
#include <stdexcept> // invalid_argument
#include <string>
#include <utility> // pair
#include <vector>

#include <libssh2.h>

namespace ssh {
namespace host_key {

namespace detail {
	namespace libssh2 {
	namespace session {

		/**
		 * Thin wrapper around libssh2_session_hostkey.
		 */
		inline std::pair<std::string, int> hostkey(
			boost::shared_ptr<LIBSSH2_SESSION> session)
		{
			size_t len = 0;
			int type = LIBSSH2_HOSTKEY_TYPE_UNKNOWN;
			const char* key = libssh2_session_hostkey(
				session.get(), &len, &type);

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
		template<typename T>
		inline T hostkey_hash(
			boost::shared_ptr<LIBSSH2_SESSION> session, int hash_type)
		{
			const T::value_type* hash_bytes = 
				reinterpret_cast<const T::value_type*>(
					libssh2_hostkey_hash(session.get(), hash_type));

			size_t len = 0;
			if (hash_type == LIBSSH2_HOSTKEY_HASH_MD5)
				len = 16;
			else if (hash_type == LIBSSH2_HOSTKEY_HASH_SHA1)
				len = 20;
			else
				BOOST_THROW_EXCEPTION(
					std::invalid_argument("Unknown hash type"));

			if (hash_bytes)
				return T(hash_bytes, hash_bytes + len);
			else
				return T();
		}

		/**
		 * Thin wrapper around libssh2_session_methods.
		 */
		inline std::string method(
			boost::shared_ptr<LIBSSH2_SESSION> session, int method_type)
		{
			const char* key_type = libssh2_session_methods(
				session.get(), method_type);
			
			if (key_type)
				return std::string(key_type);
			else
				return std::string();
		}

	}}
}

/**
 * Possible types of host-key algorithm.
 */
enum hostkey_type
{
	unknown,
	rsa1,
	ssh_rsa,
	ssh_dss
};

namespace detail {
	
	/**
	 * Convert the returned key-type from libssh2_session_hostkey into a 
	 * value from the hostkey_type enum.
	 */
	inline hostkey_type type_to_hostkey_type(int type)
	{
		switch (type)
		{
		case LIBSSH2_HOSTKEY_TYPE_RSA:
			return ssh_rsa;
		case LIBSSH2_HOSTKEY_TYPE_DSS:
			return ssh_dss;
		default:
			return unknown;
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
	explicit host_key(boost::shared_ptr<LIBSSH2_SESSION> session)
		: m_session(session),
		  m_key(detail::libssh2::session::hostkey(session)) {}

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
	bool is_base64() const { return false; }

	/**
	 * Type of the key algorithm e.g., ssh-dss.
	 */
	hostkey_type algorithm() const
	{
		return detail::type_to_hostkey_type(m_key.second);
	}
	
	/**
	 * Printable name of the method negotiated for the key algorithm.
	 */
	std::string algorithm_name() const
	{
		return detail::libssh2::session::method(
			m_session, LIBSSH2_METHOD_HOSTKEY);
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
		return detail::libssh2::session::hostkey_hash<
			std::vector<unsigned char> >(
			m_session, LIBSSH2_HOSTKEY_HASH_MD5);
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
		return detail::libssh2::session::hostkey_hash<
			std::vector<unsigned char> >(
			m_session, LIBSSH2_HOSTKEY_HASH_SHA1);
	}

private:
	boost::shared_ptr<LIBSSH2_SESSION> m_session;
	std::pair<std::string, int> m_key;
};

/**
 * Turn a collection of bytes into a printable hexidecimal string.
 *
 * @param bytes       Collection of bytes.
 * @param nibble_sep  String to place between each pair of hexidecimal
 *                    characters.
 * @param uppercase   Whether to use uppercase or lowercase hexidecimal.
 */
template<typename T>
std::string hexify(
	const T& bytes, const std::string& nibble_sep=":", bool uppercase=false)
{
	std::ostringstream hex_hash;

	if (uppercase)
		hex_hash << std::uppercase;
	else
		hex_hash << std::nouppercase;
	hex_hash << std::hex << std::setfill('0');

	BOOST_FOREACH(unsigned char b, bytes)
	{
		if (!hex_hash.str().empty())
			hex_hash << nibble_sep;

		unsigned int i = b;
		hex_hash << std::setw(2) << i;
	}

	return hex_hash.str();
}

}} // namespace ssh::host_key

#endif
