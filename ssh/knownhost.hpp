/**
    @file

    Interface to known-host mechanism.

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

#ifndef SSH_KNOWNHOST_HPP
#define SSH_KNOWNHOST_HPP
#pragma once

#include <ssh/ssh_error.hpp> // last_error
#include <ssh/host_key.hpp>

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/errinfo_file_name.hpp> // errinfo_file_name
#include <boost/exception/info.hpp> // errinfo
#include <boost/filesystem.hpp> // path
#include <boost/filesystem/fstream.hpp> // path-enabled fstream
#include <boost/iterator/iterator_facade.hpp> // iterator_facade
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#undef min
#include <algorithm> // for_each, transform
#include <cassert> // assert
#include <iterator> // iterator_traits
#include <stdexcept> // invalid_argument, logic_error
#include <string>
#include <vector>

#include <libssh2.h>

namespace ssh {
namespace knownhost {

class knownhost;

namespace detail {

    /**
     * Create a new libssh2 knownhost collection.
     */
    inline boost::shared_ptr<LIBSSH2_KNOWNHOSTS> init(
        boost::shared_ptr<LIBSSH2_SESSION> session)
    {
        if (!session)
            BOOST_THROW_EXCEPTION(
                std::invalid_argument("NULL session pointer"));

        LIBSSH2_KNOWNHOSTS* hosts = libssh2_knownhost_init(session.get());
        if (!hosts)
            BOOST_THROW_EXCEPTION(
                ssh::detail::last_error(session) <<
                boost::errinfo_api_function("libssh2_knownhost_init"));

        return boost::shared_ptr<LIBSSH2_KNOWNHOSTS>(
            hosts, libssh2_knownhost_free);
    }

    /**
     * Entry-reading functor.
     */
    template<int TYPE, typename T>
    class read_entry : public std::unary_function<T, void>
    {
    public:
        read_entry(
            boost::shared_ptr<LIBSSH2_SESSION> session,
            boost::shared_ptr<LIBSSH2_KNOWNHOSTS> hosts)
            : m_session(session), m_hosts(hosts)
        {
        }

        /**
         * Read entry into libssh2 knownhost collection.
         */
        void operator()(const T& entry)
        {
            int rc = libssh2_knownhost_readline(
                m_hosts.get(), entry.data(), entry.length(), TYPE);
            if (rc != 0)
                BOOST_THROW_EXCEPTION(
                    ssh::detail::last_error(m_session) <<
                    boost::errinfo_api_function("libssh2_knownhost_readline"));
        }

    private:
        boost::shared_ptr<LIBSSH2_SESSION> m_session;
        boost::shared_ptr<LIBSSH2_KNOWNHOSTS> m_hosts;
    };

    /**
     * Entry-writing functor.
     */
    template<int TYPE>
    class write_entry
        : public std::unary_function<const knownhost&, std::string>
    {
    public:
        write_entry(boost::shared_ptr<LIBSSH2_KNOWNHOSTS> hosts)
            : m_hosts(hosts)
        {
        }

        /**
         * Write entry from collection to string.
         *
         * The return type must be able to create a writable char buffer of a
         * certain size by a call to its contructor like this:
         * T(size, default_val).  This is the case for both string and
         * vector<char>.
         */
        std::string operator()(const knownhost& host)
        {
            return host.to_string(TYPE);
        }

    private:
        boost::shared_ptr<LIBSSH2_KNOWNHOSTS> m_hosts;
    };

    /**
     * Line proxy to return strings line-by-line from istream_iterator.
     */
    class line
    {
    public:
        friend std::istream& operator>>(std::istream& is, line& l)
        {
            std::getline(is, l.m_data);
            return is;
        }

        friend std::ostream& operator<<(std::ostream& os, const line& l)
        {
            os << l.m_data;
            return os;
        }

        friend bool operator!=(const std::string& other, const line& l)
        {
            return other != l.m_data;
        }

        friend bool operator==(const line& l, const std::string& other)
        {
            return other == l.m_data;
        }

        std::string::size_type length() const { return m_data.length(); }
        std::string::const_pointer data() const { return m_data.data(); }

    private:
        std::string m_data;
    };

    /**
     * Thin exception wrapper around libssh2_knownhost_get.
     *
     * @returns NULL if finished.
     */
    inline libssh2_knownhost* next_host(
        boost::shared_ptr<LIBSSH2_SESSION> session,
        boost::shared_ptr<LIBSSH2_KNOWNHOSTS> hosts,
        libssh2_knownhost* current_position)
    {
        libssh2_knownhost* host = NULL;
        int rc = libssh2_knownhost_get(hosts.get(), &host, current_position);
        if (rc < 0)
        {
            BOOST_THROW_EXCEPTION(
                ssh::detail::last_error(session) <<
                boost::errinfo_api_function("libssh2_knownhost_get"));
        }
        
        if (rc == 1) // finished
        {
            assert(host == NULL);
            host = NULL;
        }

        return host;
    }

    /**
     * Thin exception wrapper around libssh2_knownhost_add.
     */
    inline libssh2_knownhost* add(
        boost::shared_ptr<LIBSSH2_SESSION> session,
        boost::shared_ptr<LIBSSH2_KNOWNHOSTS> hosts,
        const std::string& host_or_ip, const std::string& salt,
        const std::string& key, int type, bool base64_key)
    {
        if (base64_key)
            type |=    LIBSSH2_KNOWNHOST_KEYENC_BASE64;
        else
            type |= LIBSSH2_KNOWNHOST_KEYENC_RAW;

        libssh2_knownhost* host = NULL;
        int rc = libssh2_knownhost_add(
            hosts.get(), host_or_ip.c_str(),
            (salt.empty()) ? NULL : salt.c_str(),
            key.data(), key.length(), type, &host);
        if (rc)
            BOOST_THROW_EXCEPTION(
                ssh::detail::last_error(session) <<
                boost::errinfo_api_function("libssh2_knownhost_add"));

        return host;
    }

    /**
     * Return the libssh2 key string which may include a comment appended to
     * the end separated by whitespace.
     */
    inline std::string internal_key(const libssh2_knownhost* pos)
    {
        return (pos && pos->key) ? pos->key : std::string();
    }
}

class knownhost
{
private:

    friend class knownhost_collection;
    friend class knownhost_iterator;
    
    knownhost(
        boost::shared_ptr<LIBSSH2_SESSION> session,
        boost::shared_ptr<LIBSSH2_KNOWNHOSTS> hosts,
        libssh2_knownhost* pos)
        : m_session(session), m_hosts(hosts), m_pos(pos) {}

public:

    std::string name() const
    {
        return (m_pos && m_pos->name) ? m_pos->name : std::string();
    }

    std::string key() const
    {
        std::string k = detail::internal_key(m_pos);
        std::string::size_type space = k.find(' ');
        if (space != std::string::npos)
            return k.substr(0, space);
        else
            return k;
    }

    /**
     * Return the optional comment attached to the host entry.
     *
     * @todo Fetch comment properly once libssh2 API allows it.
     */
    std::string comment() const
    {
        std::string k = detail::internal_key(m_pos);
        std::string::size_type space = k.find(' ');
        if (space != std::string::npos && space + 1 != std::string::npos)
            return k.substr(space + 1);
        else
            return std::string();
    }

    std::string to_string(int type) const
    {
        // get minimum required buffer size (doesn't include null-term)
        size_t required_len = 0;
        int rc = libssh2_knownhost_writeline(
            m_hosts.get(), m_pos, NULL, 0, &required_len, type); 
        assert(rc == LIBSSH2_ERROR_BUFFER_TOO_SMALL);
        required_len++; // returned val doesn't include NULL-terminator

        // now repeat but with a properly allocated buffer
        std::vector<char> buf(required_len);
        rc = libssh2_knownhost_writeline(
            m_hosts.get(), m_pos, &buf[0], buf.size(), &required_len, type);
        assert(required_len == buf.size() - 1);
        if (rc != 0)
            BOOST_THROW_EXCEPTION(
                ssh::detail::last_error(m_session) <<
                boost::errinfo_api_function("libssh2_knownhost_writeline"));

        // Return line excluding '\n' and NULL-terminator
        return std::string(
            &buf[0], std::min(buf.size() - 2, required_len - 1));
    }

    /**
     * The key algorithm as an algorithm name.
     */
    std::string key_algo() const
    {
        switch (m_pos->typemask & LIBSSH2_KNOWNHOST_KEY_MASK)
        {
        case LIBSSH2_KNOWNHOST_KEY_RSA1:
            return "rsa1";
        case LIBSSH2_KNOWNHOST_KEY_SSHRSA:
            return "ssh-rsa";
        case LIBSSH2_KNOWNHOST_KEY_SSHDSS:
            return "ssh-dss";
        default:
            return "unknown";
        }
    }

    /** @name Predicate members. */
    // @{

    /**
     * Hostname is not encoded; it is plain-text.
     * e.g. hostname.example.com
     */
    bool is_name_plain() const
    {
        return (m_pos->typemask & LIBSSH2_KNOWNHOST_TYPE_MASK) ==
            LIBSSH2_KNOWNHOST_TYPE_PLAIN;
    }

    /**
     * Hostname and salt is hashed using sha1 and base64-encoded.
     *
     * When this predicate is true, name() returns an empty string as the
     * hash can't be converted back to a hostname.
     */
    bool is_name_sha1() const
    {
        return (m_pos->typemask & LIBSSH2_KNOWNHOST_TYPE_MASK) ==
            LIBSSH2_KNOWNHOST_TYPE_SHA1;
    }

    /**
     * Hostname encoded with some user-defined encoding.
     */
    bool is_name_custom() const
    {
        return (m_pos->typemask & LIBSSH2_KNOWNHOST_TYPE_MASK) ==
            LIBSSH2_KNOWNHOST_TYPE_CUSTOM;
    }

    // @}

private:

    boost::shared_ptr<LIBSSH2_SESSION> m_session;
    boost::shared_ptr<LIBSSH2_KNOWNHOSTS> m_hosts;
    libssh2_knownhost* m_pos;
};

/**
 * @todo  Should libssh2_knownhost_get take a const.
 */
class knownhost_iterator : public boost::iterator_facade<
    knownhost_iterator, const knownhost, boost::forward_traversal_tag,
    knownhost>
{
public:
    /**
     * Create an iterator to the end of the collection.
     */
    knownhost_iterator() : m_pos(NULL) {}
    
    /**
     * Create an iterator to the beginning of the collection.
     */
    knownhost_iterator(
        boost::shared_ptr<LIBSSH2_SESSION> session,
        boost::shared_ptr<LIBSSH2_KNOWNHOSTS> hosts)
        : m_session(session), m_hosts(hosts),
          m_pos(detail::next_host(m_session, m_hosts, NULL)) {}

    /**
     * Create an iterator to a point in the collection indicated by pos.
     */
    knownhost_iterator(
        boost::shared_ptr<LIBSSH2_KNOWNHOSTS> hosts, libssh2_knownhost* pos)
        : m_hosts(hosts), m_pos(pos) {}


    /**
     * Remove a host at the given iterator position from the collection.
     *
     * After this function returns, any iterators that pointed to the removed
     * item (including the given one) will be invalidated.  Attempting to
     * call any of their member functions results in undefined behaviour.
     *
     * @returns  Iterator to the next item in the collection or the end of the
     *           collection if there are no more items.
     */
    friend knownhost_iterator erase(knownhost_iterator it)
    {
        knownhost_iterator next = it;
        next++;

        // this call invalidates the given iterator
        int rc = libssh2_knownhost_del(it.m_hosts.get(), it.m_pos);
        if (rc)
            BOOST_THROW_EXCEPTION(
                ssh::detail::last_error(it.m_session) <<
                boost::errinfo_api_function("libssh2_knownhost_del"));

        return next;
    }

private:
    friend class boost::iterator_core_access;

    void increment()
    {
        if (m_pos == NULL)
            BOOST_THROW_EXCEPTION(
                std::logic_error(
                    "Can't increment past the end of a collection"));
        m_pos = detail::next_host(m_session, m_hosts, m_pos);
    }

    bool equal(knownhost_iterator const& other) const
    {
        return this->m_pos == other.m_pos;
    }

    knownhost dereference() const
    {
        if (m_pos == NULL)
            BOOST_THROW_EXCEPTION(
                std::logic_error("Can't dereference the end of a collection"));
        return knownhost(m_session, m_hosts, m_pos);
    }

    boost::shared_ptr<LIBSSH2_SESSION> m_session;
    boost::shared_ptr<LIBSSH2_KNOWNHOSTS> m_hosts;
    libssh2_knownhost* m_pos;
};

/**
 * Result returned by knownhost_collection::find().
 */
class find_result
{
public:
    find_result(
        const knownhost_iterator& it, const knownhost_iterator& end,
        bool match)
        : m_host(it), m_end(end), m_match(match)
    {
        assert(!match || (m_host != m_end)); 
    }

    knownhost_iterator host() const { return m_host; }
    bool mismatch() const { return !m_match && (m_host != m_end); }
    bool match() const { return m_match && (m_host != m_end); }
    bool not_found() const { return m_host == m_end; }

private:
    knownhost_iterator m_host;
    knownhost_iterator m_end;
    bool m_match;
};

namespace detail {
    
    /**
     * Convert a value from the hostkey_type enum into an integer suitable
     * for use by libssh2_knownhost_add.
     */
    inline int hostkey_type_to_add_type(ssh::host_key::hostkey_type type)
    {
        switch (type)
        {
        case ssh::host_key::rsa1:
            return LIBSSH2_KNOWNHOST_KEY_RSA1;
        case ssh::host_key::ssh_rsa:
            return LIBSSH2_KNOWNHOST_KEY_SSHRSA;
        case ssh::host_key::ssh_dss:
            return LIBSSH2_KNOWNHOST_KEY_SSHDSS;
        case ssh::host_key::unknown:
        default:
            BOOST_THROW_EXCEPTION(
                std::invalid_argument("Unrecognised key algorithm"));
        }
    }
}

/**
 * Collection of known-host entries.
 */
class knownhost_collection
{
public:
    explicit knownhost_collection(boost::shared_ptr<LIBSSH2_SESSION> session)
        : m_session(session), m_hosts(detail::init(session))
    {
    }

    knownhost_iterator begin() const
    {
        return knownhost_iterator(m_session, m_hosts);
    }

    knownhost_iterator end() const
    {
        return knownhost_iterator();
    }

    find_result find(
        const std::string& host, const std::string& key, bool base64_key)
    const
    {
        int type = LIBSSH2_KNOWNHOST_TYPE_PLAIN;
        if (base64_key)
            type |=    LIBSSH2_KNOWNHOST_KEYENC_BASE64;
        else
            type |= LIBSSH2_KNOWNHOST_KEYENC_RAW;

        libssh2_knownhost* match = NULL;
        int rc = libssh2_knownhost_check(
            m_hosts.get(), host.c_str(), key.data(), key.length(), type,
            &match);

        switch (rc)
        {
        case LIBSSH2_KNOWNHOST_CHECK_MATCH:
            return find_result(
                knownhost_iterator(m_hosts, match), end(), true);

        case LIBSSH2_KNOWNHOST_CHECK_MISMATCH:
            return find_result(
                knownhost_iterator(m_hosts, match), end(), false);
            
        case LIBSSH2_KNOWNHOST_CHECK_NOTFOUND:
            return find_result(end(), end(), false);

        default:
            BOOST_THROW_EXCEPTION(
                ssh::detail::last_error(m_session) <<
                boost::errinfo_api_function("libssh2_knownhost_check"));
        }
    }
    
    find_result find(
        const std::string& host, const ssh::host_key::host_key& key)
    {
        return find(host, key.key(), key.is_base64());
    }

    knownhost add(
        const std::string& host_or_ip,
        const std::string& key,    ssh::host_key::hostkey_type algorithm,
        bool base64_key)
    {
        int type = LIBSSH2_KNOWNHOST_TYPE_PLAIN |
            detail::hostkey_type_to_add_type(algorithm);

        libssh2_knownhost* host = detail::add(
            m_session, m_hosts, host_or_ip, std::string(), key, type,
            base64_key);

        return knownhost(m_session, m_hosts, host);
    }

    knownhost add(
        const std::string& host_or_ip, const ssh::host_key::host_key& key)
    {
        return add(host_or_ip, key.key(), key.algorithm(), key.is_base64());
    }

    knownhost add_hashed(
        const std::string& host_or_ip, const std::string& salt,
        const std::string& key, ssh::host_key::hostkey_type algorithm,
        bool base64_key)
    {
        int type = LIBSSH2_KNOWNHOST_TYPE_SHA1 |
            detail::hostkey_type_to_add_type(algorithm);

        libssh2_knownhost* host = detail::add(
            m_session, m_hosts, host_or_ip, salt, key, type, base64_key);

        return knownhost(m_session, m_hosts, host);
    }

    knownhost add_hashed(
        const std::string& host_or_ip, const std::string& salt,
        const ssh::host_key::host_key& key)
    {
        return add_hashed(
            host_or_ip, salt, key.key(), key.algorithm(), key.is_base64());
    }

    knownhost add_custom(
        const std::string& host_or_ip,
        const std::string& key, ssh::host_key::hostkey_type algorithm,
        bool base64_key)
    {
        int type = LIBSSH2_KNOWNHOST_TYPE_CUSTOM |
            detail::hostkey_type_to_add_type(algorithm);

        libssh2_knownhost* host = detail::add(
            m_session, m_hosts, host_or_ip, std::string(), key, type,
            base64_key);

        return knownhost(m_session, m_hosts, host);
    }

    knownhost add_custom(
        const std::string& host_or_ip, const ssh::host_key::host_key& key)
    {
        return add_custom(
            host_or_ip, key.key(), key.algorithm(), key.is_base64());
    }

protected:

    /**
     * Initialise the known-hosts collection from a range of entries.
     *
     * @param begin  Iterator to the start of the range.
     * @param end    Iterator indicating termination (half-closed).
     * @param TYPE   Type of entry to read.  Currently the only supported type
     *               is LIBSSH2_KNOWNHOST_FILE_OPENSSH in which case the
     *               entry must be in OpenSSH known_hosts format (hashed or
     *               unhashed).
     * @param It     InputIterator to range of entries in known_hosts format.
     */
    template<int TYPE, typename InputIt>
    void load_entries(const InputIt& begin, const InputIt& end)
    {
        typedef std::iterator_traits<InputIt>::value_type value_t;

        std::for_each(
            begin, end, detail::read_entry<TYPE, value_t>(m_session, m_hosts));
    }

    /**
     * Initialise the known-hosts collection from a range of entries.
     *
     * @param begin  Iterator to the start of the range.
     * @param end    Iterator indicating termination (half-closed).
     * @param type   Type of entry to read.  Currently the only supported type
     *               is LIBSSH2_KNOWNHOST_FILE_OPENSSH in which case the
     *               entry must be in OpenSSH known_hosts format (hashed or
     *               unhashed).
     */
    template<int TYPE, typename OutputIt>
    OutputIt save_entries(
        const knownhost_iterator& begin, const knownhost_iterator& end,
        OutputIt output) const
    {
        return std::transform(
            begin, end, output, detail::write_entry<TYPE>(m_hosts));
    }

private:
    boost::shared_ptr<LIBSSH2_SESSION> m_session;
    boost::shared_ptr<LIBSSH2_KNOWNHOSTS> m_hosts;
};

knownhost add(
    knownhost_collection& hosts, const std::string& host_or_ip,
    const ssh::host_key::host_key& key)
{
    return hosts.add(host_or_ip, key.key(), key.algorithm(), key.is_base64());
}

knownhost add_hashed(
    knownhost_collection& hosts, const std::string& host_or_ip,
    const std::string& salt, const ssh::host_key::host_key& key)
{
    return hosts.add_hashed(
        host_or_ip, salt, key.key(), key.algorithm(), key.is_base64());
}

knownhost add_custom(
    knownhost_collection& hosts, const std::string& host_or_ip,
    const ssh::host_key::host_key& key)
{
    return hosts.add_custom(
        host_or_ip, key.key(), key.algorithm(), key.is_base64());
}

knownhost update(
    knownhost_collection& hosts, const std::string& host_or_ip,
    const ssh::host_key::host_key& key, const find_result& entry)
{
    erase(entry.host());
    return add(hosts, host_or_ip, key);
}

/**
 * Collection of known-host entries stored in OpenSSH known_hosts format.
 *
 * In the absence of changes, entries are written back exactly as they
 * were read, with the following exceptions:
 *  - ip,hostname combinations are split onto two lines, ip first
 *  - tabs in seperators are replaced by a single space
 */
class openssh_knownhost_collection : public knownhost_collection
{
public:

    /** Initialise collection from a range of OpenSSH known_hosts lines. */
    template<typename InputIt>
    openssh_knownhost_collection(
        boost::shared_ptr<LIBSSH2_SESSION> session, InputIt begin, InputIt end)
        : knownhost_collection(session)
    {
        load_entries<LIBSSH2_KNOWNHOST_FILE_OPENSSH>(begin, end);
    }

    /** Initialise collection from an OpenSSH known_hosts file. */
    openssh_knownhost_collection(
        boost::shared_ptr<LIBSSH2_SESSION> session,
        const boost::filesystem::path& filename)
        : knownhost_collection(session)
    {
        boost::filesystem::ifstream file(filename);
        if (!file)
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error(
                        "Could not read from known-hosts file")) <<
                boost::errinfo_file_name(filename.external_file_string()));

        load_entries<LIBSSH2_KNOWNHOST_FILE_OPENSSH>(
            std::istream_iterator<detail::line>(file),
            std::istream_iterator<detail::line>());
    }

    /**
     * Initialise collection from an OpenSSH known_hosts file.
     *
     * @TODO  Make errinfo work with wide paths.
     */
    openssh_knownhost_collection(
        boost::shared_ptr<LIBSSH2_SESSION> session,
        const boost::filesystem::wpath& filename)
        : knownhost_collection(session)
    {
        boost::filesystem::ifstream file(filename);
        if (!file)
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error(
                        "Could not read from known-hosts file")));/* <<
                boost::errinfo_file_name(filename.external_file_string()));*/

        load_entries<LIBSSH2_KNOWNHOST_FILE_OPENSSH>(
            std::istream_iterator<detail::line>(file),
            std::istream_iterator<detail::line>());
    }

    /**
     * Save range of entries to an output iterator in OpenSSH known_hosts
     * format.
     *
     * Entries do @b not end in a newline character.
     */
    template<typename OutputIt>
    OutputIt save(
        const knownhost_iterator& begin, const knownhost_iterator& end,
        OutputIt output) const
    {
        return save_entries<LIBSSH2_KNOWNHOST_FILE_OPENSSH, OutputIt>(
            begin, end, output);
    }

    /** Save all entires to an OpenSSH known_hosts file. */
    void save(const boost::filesystem::path& filename) const
    {
        boost::filesystem::ofstream file(filename);
        if (!file)
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error(
                        "Could not write to known-hosts file")) <<
                boost::errinfo_file_name(filename.external_file_string()));

        save(
            begin(), end(), std::ostream_iterator<std::string>(file, "\n"));
    }

    /**
     * Save all entires to an OpenSSH known_hosts file.
     *
     * @TODO  Make errinfo work with wide paths.
     */
    void save(const boost::filesystem::wpath& filename) const
    {
        boost::filesystem::ofstream file(filename);
        if (!file)
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error(
                        "Could not write to known-hosts file"))/* <<
            boost::errinfo_file_name(filename.external_file_string())*/);

        save(
            begin(), end(), std::ostream_iterator<std::string>(file, "\n"));
    }
};

}} // namespace ssh::knownhost

#endif
