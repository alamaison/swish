/**
    @file

    SSH SFTP subsystem.

    @if license

    Copyright (C) 2010, 2012, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SSH_SFTP_HPP
#define SSH_SFTP_HPP
#pragma once

#include <ssh/ssh_error.hpp> // last_error, ssh_error
#include <ssh/session.hpp>

#include <boost/cstdint.hpp> // uint64_t, uintmax_t
#include <boost/exception/errinfo_file_name.hpp> // errinfo_file_name
#include <boost/exception/info.hpp> // errinfo_api_function
#include <boost/filesystem/path.hpp> // path
#include <boost/iterator/iterator_facade.hpp> // iterator_facade
#include <boost/optional/optional.hpp>
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION, throw_exception

#include <algorithm> // min
#include <cassert> // assert
#include <exception> // bad_alloc
#include <string>
#include <vector>

#include <libssh2_sftp.h>

namespace ssh {
namespace sftp {

namespace detail {

inline const char* sftp_part_of_error_message(unsigned long error)
{
    switch (error)
    {
    case LIBSSH2_FX_OK:
        return ": FX_OK";
    case LIBSSH2_FX_EOF:
        return ": FX_EOF";
    case LIBSSH2_FX_NO_SUCH_FILE:
        return ": FX_NO_SUCH_FILE";
    case LIBSSH2_FX_PERMISSION_DENIED:
        return ": FX_PERMISSION_DENIED";
    case LIBSSH2_FX_FAILURE:
        return ": FX_FAILURE";
    case LIBSSH2_FX_BAD_MESSAGE:
        return ": FX_BAD_MESSAGE";
    case LIBSSH2_FX_NO_CONNECTION:
        return ": FX_NO_CONNECTION";
    case LIBSSH2_FX_CONNECTION_LOST:
        return ": FX_CONNECTION_LOST";
    case LIBSSH2_FX_OP_UNSUPPORTED:
        return ": FX_OP_UNSUPPORTED";
    case LIBSSH2_FX_INVALID_HANDLE:
        return ": FX_INVALID_HANDLE";
    case LIBSSH2_FX_NO_SUCH_PATH:
        return ": FX_NO_SUCH_PATH";
    case LIBSSH2_FX_FILE_ALREADY_EXISTS:
        return ": FX_FILE_ALREADY_EXISTS";
    case LIBSSH2_FX_WRITE_PROTECT:
        return ": FX_WRITE_PROTECT";
    case LIBSSH2_FX_NO_MEDIA:
        return ": FX_NO_MEDIA";
    case LIBSSH2_FX_NO_SPACE_ON_FILESYSTEM:
        return ": FX_NO_SPACE_ON_FILESYSTEM";
    case LIBSSH2_FX_QUOTA_EXCEEDED:
        return ": FX_QUOTA_EXCEEDED";
    case LIBSSH2_FX_UNKNOWN_PRINCIPAL:
        return ": FX_UNKNOWN_PRINCIPAL";
    case LIBSSH2_FX_LOCK_CONFLICT:
        return ": FX_LOCK_CONFLICT";
    case LIBSSH2_FX_DIR_NOT_EMPTY:
        return ": FX_DIR_NOT_EMPTY";
    case LIBSSH2_FX_NOT_A_DIRECTORY:
        return ": FX_NOT_A_DIRECTORY";
    case LIBSSH2_FX_INVALID_FILENAME:
        return ": FX_INVALID_FILENAME";
    case LIBSSH2_FX_LINK_LOOP:
        return ": FX_LINK_LOOP";
    default:
        return "Unrecognised SFTP error value";
    }
}

}

class sftp_error : public ::ssh::ssh_error
{
public:
    sftp_error(
        const ssh::ssh_error& error, unsigned long sftp_error_code)
        : ssh_error(error), m_sftp_error(sftp_error_code)
    {
        message() += detail::sftp_part_of_error_message(m_sftp_error);
    }

    unsigned long sftp_error_code() const
    {
        return m_sftp_error;
    }
    
private:
    unsigned long m_sftp_error;
};

namespace detail {

    template<typename T>
    inline void throw_error(
        T& error, const char* current_function, const char* source_file,
        int source_line, const char* api_function,
        const char* path, size_t path_len)
    {
        error <<
            boost::errinfo_api_function(api_function) <<
            boost::throw_function(current_function) <<
            boost::throw_file(source_file) <<
            boost::throw_line(source_line);
        if (path && path_len > 0)
            error << boost::errinfo_file_name(std::string(path, path_len));

        boost::throw_exception(error);
    }

    /**
     * Throw whatever the most appropriate type of exception is.
     *
     * libssh2_sftp_* functions can return either a standard SSH error or a
     * SFTP error.  This function checks and throws the appropriate object.
     */
    inline void throw_last_error(
        boost::shared_ptr<LIBSSH2_SESSION> session,
        boost::shared_ptr<LIBSSH2_SFTP> sftp, const char* current_function,
        const char* source_file, int source_line, const char* api_function,
        const char* path=NULL, size_t path_len=0U)
    {
        ::ssh::ssh_error error =
            ::ssh::detail::last_error(session);

        if (error.error_code() == LIBSSH2_ERROR_SFTP_PROTOCOL)
        {
            sftp_error derived_error = 
                sftp_error(error, libssh2_sftp_last_error(sftp.get()));
            throw_error(
                derived_error, current_function, source_file, source_line,
                api_function, path, path_len);
        }
        else
        {
            throw_error(
                error, current_function, source_file, source_line, api_function,
                path, path_len);
        }
    }
}

#define SSH_THROW_LAST_SFTP_ERROR(session, sftp_session, api_function) \
    ::ssh::sftp::detail::throw_last_error( \
        session,sftp_session,BOOST_CURRENT_FUNCTION,__FILE__,__LINE__, \
        api_function)

#define SSH_THROW_LAST_SFTP_ERROR_WITH_PATH( \
    session, sftp_session, api_function, path, path_len) \
    ::ssh::sftp::detail::throw_last_error( \
        session,sftp_session,BOOST_CURRENT_FUNCTION,__FILE__,__LINE__, \
        api_function, path, path_len)

namespace detail {

    namespace libssh2 {
    namespace sftp {

        /**
         * Thin exception wrapper around libssh2_sftp_init.
         */
        inline boost::shared_ptr<LIBSSH2_SFTP> init(
            boost::shared_ptr<LIBSSH2_SESSION> session)
        {
            LIBSSH2_SFTP* sftp = libssh2_sftp_init(session.get());
            if (!sftp)
                BOOST_THROW_EXCEPTION(
                    ssh::detail::last_error(session) <<
                    boost::errinfo_api_function("libssh2_sftp_init"));

            return boost::shared_ptr<LIBSSH2_SFTP>(
                sftp, libssh2_sftp_shutdown);
        }

        /**
         * Thin exception wrapper around libssh2_sftp_open_ex
         */
        inline boost::shared_ptr<LIBSSH2_SFTP_HANDLE> open(
            boost::shared_ptr<LIBSSH2_SESSION> session,
            boost::shared_ptr<LIBSSH2_SFTP> sftp, const char* filename,
            unsigned int filename_len, unsigned long flags, long mode,
            int open_type)
        {
            LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open_ex(
                sftp.get(), filename, filename_len, flags, mode, open_type);
            if (!handle)
                SSH_THROW_LAST_SFTP_ERROR_WITH_PATH(
                    session, sftp, "libssh2_sftp_open_ex", filename,
                    filename_len);

            return boost::shared_ptr<LIBSSH2_SFTP_HANDLE>(
                handle, libssh2_sftp_close_handle);
        }

        /**
         * Thin exception wrapper around libssh2_sftp_symlink_ex.
         *
         * Slightly odd treatment of the return value because it's overloaded.
         * Callers of this function never need to check it for success but do
         * need to check if for buffer validity in the @c LIBSSH2_SFTP_READLINK
         * and @c LIBSSH2_SFTP_REALPATH cases.
         */
        inline int symlink_ex(
            boost::shared_ptr<LIBSSH2_SESSION> session,
            boost::shared_ptr<LIBSSH2_SFTP> sftp,
            const char* path, unsigned int path_len,
            char* target, unsigned int target_len, int resolve_action)
        {
            int rc = libssh2_sftp_symlink_ex(
                sftp.get(), path, path_len, target, target_len,
                resolve_action);
            switch (resolve_action)
            {
            case LIBSSH2_SFTP_READLINK:
            case LIBSSH2_SFTP_REALPATH:
                if (rc < 0)
                    SSH_THROW_LAST_SFTP_ERROR(
                        session, sftp, "libssh2_sftp_symlink_ex");
                break;

            case LIBSSH2_SFTP_SYMLINK:
            default:
                if (rc != 0)
                {
                    assert(rc < 0);
                    SSH_THROW_LAST_SFTP_ERROR(
                        session, sftp, "libssh2_sftp_symlink_ex");
                }

                break;
            }

            return rc;
        }

        /**
         * Thin exception wrapper around libssh2_sftp_symlink.
         *
         * Uses the raw libssh2_sftp_symlink_ex function so we aren't forced
         * to use strlen.
         */
        inline void symlink(
            boost::shared_ptr<LIBSSH2_SESSION> session,
            boost::shared_ptr<LIBSSH2_SFTP> sftp,
            const char* path, unsigned int path_len,
            const char* target, unsigned int target_len)
        {
            symlink_ex(
                session, sftp, path, path_len,
                const_cast<char*>(target), target_len,
                LIBSSH2_SFTP_SYMLINK);
        }

        namespace detail {

            /**
             * Common parts of readlink and realpath.
             */
            inline boost::filesystem::path symlink_resolve(
                boost::shared_ptr<LIBSSH2_SESSION> session,
                boost::shared_ptr<LIBSSH2_SFTP> sftp,
                const char* path, unsigned int path_len, int resolve_action)
            {
                // yuk! hardcoded buffer sizes. unfortunately, libssh2 doesn't
                // give us a choice so we allocate massive buffers here and then
                // take measures later to reduce the footprint

                std::vector<char> target_path_buffer(1024, '\0');

                int len = symlink_ex(session, sftp, path, path_len,
                    &target_path_buffer[0], target_path_buffer.size(),
                    resolve_action);

                return boost::filesystem::path(
                    &target_path_buffer[0], &target_path_buffer[0] + len);
            }

        }

        /**
         * Thin exception wrapper around libssh2_sftp_realpath.
         *
         * Uses the raw libssh2_sftp_symlink_ex function so we aren't forced
         * to use strlen.
         */
        inline boost::filesystem::path realpath(
            boost::shared_ptr<LIBSSH2_SESSION> session,
            boost::shared_ptr<LIBSSH2_SFTP> sftp,
            const char* path, unsigned int path_len)
        {
            return detail::symlink_resolve(
                session, sftp, path, path_len, LIBSSH2_SFTP_REALPATH);
        }

        /**
         * Thin exception wrapper around libssh2_sftp_readlink.
         *
         * Uses the raw libssh2_sftp_symlink_ex function so we aren't forced
         * to use strlen.
         */
        inline boost::filesystem::path readlink(
            boost::shared_ptr<LIBSSH2_SESSION> session,
            boost::shared_ptr<LIBSSH2_SFTP> sftp,
            const char* path, unsigned int path_len)
        {
            return detail::symlink_resolve(
                session, sftp, path, path_len, LIBSSH2_SFTP_READLINK);
        }

        /**
         * Thin exception wrapper around libssh2_sftp_stat_ex.
         */
        inline void stat(
            boost::shared_ptr<LIBSSH2_SESSION> session,
            boost::shared_ptr<LIBSSH2_SFTP> sftp,
            const char* path, unsigned int path_len, int stat_type,
            LIBSSH2_SFTP_ATTRIBUTES* attributes)
        {
            int rc = libssh2_sftp_stat_ex(
                sftp.get(), path, path_len, stat_type, attributes);
            if (rc < 0)
                SSH_THROW_LAST_SFTP_ERROR_WITH_PATH(
                    session, sftp, "libssh2_sftp_stat_ex", path, path_len);
        }

        /**
         * Thin exception wrapper around libssh2_sftp_unlink_ex.
         */
        inline void unlink_ex(
            boost::shared_ptr<LIBSSH2_SESSION> session,
            boost::shared_ptr<LIBSSH2_SFTP> sftp,
            const char* path, unsigned int path_len)
        {
            int rc = libssh2_sftp_unlink_ex(sftp.get(), path, path_len);
            if (rc < 0)
                SSH_THROW_LAST_SFTP_ERROR_WITH_PATH(
                    session, sftp, "libssh2_sftp_unlink_ex", path, path_len);
        }

        /**
         * Thin exception wrapper around libssh2_sftp_rmdir_ex.
         */
        inline void rmdir_ex(
            boost::shared_ptr<LIBSSH2_SESSION> session,
            boost::shared_ptr<LIBSSH2_SFTP> sftp,
            const char* path, unsigned int path_len)
        {
            int rc = libssh2_sftp_rmdir_ex(sftp.get(), path, path_len);
            if (rc < 0)
                SSH_THROW_LAST_SFTP_ERROR_WITH_PATH(
                    session, sftp, "libssh2_sftp_rmdir_ex", path, path_len);
        }
    }}
}

class sftp_channel
{
public:

    /**
     * Open a new SFTP channel in an SSH session.
     */
    explicit sftp_channel(::ssh::session session)
        :
        m_session(session),
        m_sftp(detail::libssh2::sftp::init(session.get())) {}

    boost::shared_ptr<LIBSSH2_SFTP> get() { return m_sftp; }
    ssh::session session() { return m_session; }

private:
    ssh::session m_session;
    boost::shared_ptr<LIBSSH2_SFTP> m_sftp;
};

namespace detail {

    inline boost::shared_ptr<LIBSSH2_SFTP_HANDLE> open_directory(
        boost::shared_ptr<LIBSSH2_SESSION> session,
        boost::shared_ptr<LIBSSH2_SFTP> channel,
        const boost::filesystem::path& path)
    {
        std::string path_string = path.string();

        return detail::libssh2::sftp::open(
            session, channel, path_string.data(), path_string.size(),
            0, 0, LIBSSH2_SFTP_OPENDIR);
    }
}

class file_attributes
{
public:

    enum file_type
    {
        normal_file,
        symbolic_link,
        directory,
        character_device,
        block_device,
        named_pipe,
        socket,
        unknown
    };

    file_type type() const
    {
        if (is_valid_attribute(LIBSSH2_SFTP_ATTR_PERMISSIONS))
        {
            switch (m_attributes.permissions & LIBSSH2_SFTP_S_IFMT)
            {
            case LIBSSH2_SFTP_S_IFIFO:
                return named_pipe;
            case LIBSSH2_SFTP_S_IFCHR:
                return character_device;
            case LIBSSH2_SFTP_S_IFDIR:
                return directory;
            case LIBSSH2_SFTP_S_IFBLK:
                return block_device;
            case LIBSSH2_SFTP_S_IFREG:
                return normal_file;
            case LIBSSH2_SFTP_S_IFLNK:
                return symbolic_link;
            case LIBSSH2_SFTP_S_IFSOCK:
                return socket;
            default:
                return unknown;
            }
        }
        else
        {
            return unknown;
        }
    }

    boost::optional<unsigned long> permissions() const
    {
        if (is_valid_attribute(LIBSSH2_SFTP_ATTR_PERMISSIONS))
        {
            return m_attributes.permissions;
        }
        else
        {
            return boost::optional<unsigned long>();
        }
    }

    boost::optional<boost::uint64_t> size() const
    {
        if (is_valid_attribute(LIBSSH2_SFTP_ATTR_SIZE))
        {
            return m_attributes.filesize;
        }
        else
        {
            return boost::optional<boost::uint64_t>();
        }
    }

    boost::optional<unsigned long> uid() const
    {
        if (is_valid_attribute(LIBSSH2_SFTP_ATTR_UIDGID))
        {
            return m_attributes.uid;
        }
        else
        {
            return boost::optional<unsigned long>();
        }
    }

    boost::optional<unsigned long> gid() const
    {
        if (is_valid_attribute(LIBSSH2_SFTP_ATTR_UIDGID))
        {
            return m_attributes.gid;
        }
        else
        {
            return boost::optional<unsigned long>();
        }
    }

    /**
     * @todo  Use Boost.DateTime or other decent datatype.
     */
    boost::optional<unsigned long> last_accessed() const
    {
        if (is_valid_attribute(LIBSSH2_SFTP_ATTR_ACMODTIME))
        {
            return m_attributes.atime;
        }
        else
        {
            return boost::optional<unsigned long>();
        }
    }

    /**
     * @todo  Use Boost.DateTime or other decent datatype.
     */
    boost::optional<unsigned long> last_modified() const
    {
        if (is_valid_attribute(LIBSSH2_SFTP_ATTR_ACMODTIME))
        {
            return m_attributes.mtime;
        }
        else
        {
            return boost::optional<unsigned long>();
        }
    }

private:
    friend class sftp_file;
    friend file_attributes attributes(
        sftp_channel channel, const boost::filesystem::path& file,
        bool follow_links);

    explicit file_attributes(const LIBSSH2_SFTP_ATTRIBUTES& raw_attributes) :
       m_attributes(raw_attributes) {}

    bool is_valid_attribute(unsigned long attribute_type) const
    {
        return (m_attributes.flags & attribute_type) != 0;
    }

    LIBSSH2_SFTP_ATTRIBUTES m_attributes;
};

/**
 * Query a file for its attributes.
 *
 * If @a follow_links is @c true, the file that is queried is the target of
 * any chain of links.  Otherwise, it is the link itself.
 */
inline file_attributes attributes(
    sftp_channel channel, const boost::filesystem::path& file,
    bool follow_links)
{
    std::string file_path = file.string();
    LIBSSH2_SFTP_ATTRIBUTES attributes = LIBSSH2_SFTP_ATTRIBUTES();
    detail::libssh2::sftp::stat(
        channel.session().get(), channel.get(), file_path.data(),
        file_path.size(),
        (follow_links) ? LIBSSH2_SFTP_STAT : LIBSSH2_SFTP_LSTAT, &attributes);

    return file_attributes(attributes);
}

class sftp_file
{
public:
    sftp_file(
        const boost::filesystem::path& file, const std::string& long_entry,
        const LIBSSH2_SFTP_ATTRIBUTES& attributes)
        :
        m_file(file), m_long_entry(long_entry), m_attributes(attributes) {}

    std::string name() const { return m_file.filename(); }
    const boost::filesystem::path& path() const { return m_file; }
    const std::string& long_entry() const { return m_long_entry; }

    /**
     * @todo Get rid of this ASAP!
     */
    const LIBSSH2_SFTP_ATTRIBUTES& raw_attributes() const
    {
        return m_attributes.m_attributes;
    }

    const file_attributes& attributes() const
    {
        return m_attributes;
    }

private:
    boost::filesystem::path m_file;
    std::string m_long_entry;
    file_attributes m_attributes;
};

inline boost::filesystem::path resolve_link_target(
    sftp_channel channel, const boost::filesystem::path& link)
{
    std::string link_string = link.string();
    return detail::libssh2::sftp::readlink(
        channel.session().get(), channel.get(), link_string.data(),
        link_string.size());
}

inline boost::filesystem::path resolve_link_target(
    sftp_channel channel, const sftp_file& link)
{
    return resolve_link_target(channel, link.path());
}

inline boost::filesystem::path canonical_path(
    sftp_channel channel, const boost::filesystem::path& link)
{
    std::string link_string = link.string();
    return detail::libssh2::sftp::realpath(
        channel.session().get(), channel.get(), link_string.data(),
        link_string.size());
}

inline boost::filesystem::path canonical_path(
    sftp_channel channel, const sftp_file& link)
{
    return canonical_path(channel, link.path());
}

/**
 * Create a symbolic link.
 *
 * @param channel  SFTP connection.
 * @param link     Path to the new link on the remote filesystem. Must not
 *                 already exist.
 * @param target   Path of file or directory to be linked to.
 *
 * @WARNING  All versions of OpenSSH and probably many other servers are
 *           implemented incorrectly and swap the order of the @p link and
 *           @p target parameters.  To connect to these servers you will have
 *           to pass the parameters to this function in the wrong order!
 */
inline void create_symlink(
    sftp_channel channel, const boost::filesystem::path& link,
    const boost::filesystem::path& target)
{
    std::string link_string = link.string();
    std::string target_string = target.string();

    detail::libssh2::sftp::symlink(
        channel.session().get(), channel.get(), link_string.data(),
        link_string.size(), target_string.data(), target_string.size());
}

/**
 * List the files and directories in a directory.
 *
 * The iterator is copyable but all copies are linked so that incrementing
 * one will increment all the copies.
 */
class directory_iterator : public boost::iterator_facade<
    directory_iterator, const sftp_file, boost::forward_traversal_tag,
    sftp_file>
{
public:

    directory_iterator(
        sftp_channel channel, const boost::filesystem::path& path)
        :
        m_session(channel.session().get()),
        m_channel(channel.get()),
        m_directory(path),
        m_handle(detail::open_directory(m_session, m_channel, path)),
        m_attributes(LIBSSH2_SFTP_ATTRIBUTES())
        {
            next_file();
        }

    /**
     * End-of-directory marker.
     */
    directory_iterator() {}

private:
    friend class boost::iterator_core_access;

    void increment()
    {
        if (m_handle == NULL)
            BOOST_THROW_EXCEPTION(std::range_error("No more files"));
        next_file();
    }

    bool equal(directory_iterator const& other) const
    {
        return this->m_handle == other.m_handle;
    }

    void next_file()
    {    
        // yuk! hardcoded buffer sizes. unfortunately, libssh2 doesn't
        // give us a choice so we allocate massive buffers here and then
        // take measures later to reduce the footprint

        std::vector<char> filename_buffer(1024, '\0');
        std::vector<char> longentry_buffer(1024, '\0');
        LIBSSH2_SFTP_ATTRIBUTES attrs = LIBSSH2_SFTP_ATTRIBUTES();

        int rc = libssh2_sftp_readdir_ex(
            m_handle.get(), &filename_buffer[0], filename_buffer.size(),
            &longentry_buffer[0], longentry_buffer.size(), &attrs);

        if (rc == 0) // end of files
            m_handle.reset();
        else if (rc < 0)
            SSH_THROW_LAST_SFTP_ERROR_WITH_PATH(
                m_session, m_channel, "libssh2_sftp_readdir_ex",
                m_directory.string().data(), m_directory.string().size());
        else
        {
            // copy attributes to member one we know we're overwriting the
            // last-retrieved file's properties
            m_attributes = attrs;

            // we don't assume that the filename is null-terminated but rc
            // holds the number of bytes written to the buffer so we can shrink
            // the filename string to that size
            m_file_name = std::string(
                &filename_buffer[0],
                (std::min)(static_cast<size_t>(rc), filename_buffer.size()));

            // the long entry must be usable in an ls -l listing according to
            // the standard so I'm interpreting this to mean it can't contain
            // embedded NULLs so we force NULL-termination and then allocate
            // the string to be the NULL-terminated size which will likely be
            // much smaller than the buffer
            longentry_buffer[longentry_buffer.size() - 1] = '\0';
            m_long_entry = std::string(&longentry_buffer[0]);
        }
    }

    sftp_file dereference() const
    {
        if (m_handle == NULL)
            BOOST_THROW_EXCEPTION(
                std::logic_error("Can't dereference the end of a collection"));

        return sftp_file(m_directory / m_file_name, m_long_entry, m_attributes);
    }

    boost::shared_ptr<LIBSSH2_SESSION> m_session;
    boost::shared_ptr<LIBSSH2_SFTP> m_channel;
    boost::filesystem::path m_directory;
    boost::shared_ptr<LIBSSH2_SFTP_HANDLE> m_handle;

    /// @name Properties of last successfully listed file.
    // @{
    std::string m_file_name;
    std::string m_long_entry;
    LIBSSH2_SFTP_ATTRIBUTES m_attributes;
    // @}
};

namespace detail {

    inline bool do_remove(
        sftp_channel channel, const boost::filesystem::path& target,
        bool is_directory)
    {
        std::string target_string = target.string();

        try
        {
            if (is_directory)
            {
                detail::libssh2::sftp::rmdir_ex(
                    channel.session().get(), channel.get(),
                    target_string.data(), target_string.size());
            }
            else
            {
                detail::libssh2::sftp::unlink_ex(
                    channel.session().get(), channel.get(),
                    target_string.data(), target_string.size());
            }
        }    
        catch (const sftp_error& e)
        {
            // Process errors by catching the exception, rather than
            // intercepting the error code directly, so as not to
            // duplicate the sftp_error processing logic.

            if (e.sftp_error_code() == LIBSSH2_FX_NO_SUCH_FILE)
            {
                // Mirror the Boost.Filesystem API which doesn't treat this
                // as an error.
                return false;
            }
            else
            {
                throw;
            }
        }

        return true;
    }

    inline bool remove_one_file(
        sftp_channel channel, const boost::filesystem::path& file)
    {
        return do_remove(channel, file, false);
    }

    inline bool remove_empty_directory(
        sftp_channel channel, const boost::filesystem::path& file)
    {
        return do_remove(channel, file, true);
    }

    inline boost::uintmax_t remove_directory(
        sftp_channel channel, const boost::filesystem::path& root)
    {
        boost::uintmax_t count = 0U;

        for (directory_iterator directory(channel, root);
            directory != directory_iterator(); ++directory)
        {
            const sftp_file& file = *directory;

            if (file.name() == "." || file.name() == "..")
            {
                continue;
            }

            if (file.attributes().type() == file_attributes::directory)
            {
                count += detail::remove_directory(channel, file.path());
            }
            else
            {
                if (detail::remove_one_file(channel, file.path()))
                {
                    ++count;
                }
                else
                {
                    // Something else deleted the file before we could
                }
            }
        }

        if (remove_empty_directory(channel, root))
        {
            ++count;
        }
        else
        {
            // Something else deleted the directory before we could or it
            // never existed in the first place
        }

        return count;
    }

    BOOST_SCOPED_ENUM_START(path_status)
    {
        non_existent,
        non_directory,
        directory
    };
    BOOST_SCOPED_ENUM_END;

    inline BOOST_SCOPED_ENUM(path_status) check_status(
        sftp_channel channel, const boost::filesystem::path& path)
    {
        try
        {
            file_attributes attrs = attributes(channel, path, false);

            if (attrs.type() == file_attributes::directory)
            {
                return path_status::directory;
            }
            else
            {
                return path_status::non_directory;
            }
        }
        catch (const sftp_error& e)
        {
            // Process errors by catching the exception, rather than
            // intercepting the error code directly, so as not to
            // duplicate the sftp_error processing logic.

            if (e.sftp_error_code() == LIBSSH2_FX_NO_SUCH_FILE)
            {
                // Mirror the Boost.Filesystem API which doesn't treat
                // this as an error.
                return path_status::non_existent;
            }
            else
            {
                throw;
            }
        }
    }

}

/**
 * Remove a file.
 *
 * Removes `target` on the filesystem available via `channel`.  If `target` is
 * a symlink, only removes the link, not what the link resolves to.  If
 * `target` is a directory, removes it only if the directory is empty.
 *
 * @returns `true` if the file was removed and `false` if the file did not
 *          exist in the first place.
 * @throws `sftp_error` if `target` is a non-empty directory.
 *
 *
 * If the calling code already knows whether `target` is a directory, this
 * function adds the overhead of a single extra stat call to the server above
 * what would be possible using plain SFTP unlink/rmdir.  This trip is needed
 * to find out that information and allows us to mirror the
 * POSIX/Boost.Filesystem remove functions that do not differentiate
 * directories.
 */
inline bool remove(sftp_channel channel, const boost::filesystem::path& target)
{
    // Unlike the POSIX/Boost.Filesystem API we are following, the SFTP
    // protocol mirrors the C API where directories can only be removed
    // using the special RMDIR command.
    //
    // We tried to avoid an extra round trip to the server (to stat the
    // file) by blindly trying the common case of non-directories and
    // ignoring the first SFTP error.  The theory was that any real
    // error should also occur on the second (rmdir) attempt.
    // But that's not true because the second error might be complaining
    // that we're trying the wrong kind of delete while the first error
    // is the actual problem (permissions, for example).  Saving the first
    // error, and overwriting the second error with it, doesn't solve the
    // problem either as it could be the second error that gives the real
    // problem with the first error being wrong-kind-of-delete.  Basically
    // we can't know which error is 'real'.  If we did, we'd know the
    // filetype already!

    switch (detail::check_status(channel, target))
    {
    case detail::path_status::non_existent:
        return false;

    case detail::path_status::directory:
        return detail::remove_empty_directory(channel, target);

    case detail::path_status::non_directory:
        // This includes 'unknown' file type.  What's the alternative?
        return detail::remove_one_file(channel, target);

    default:
        assert(false);
        BOOST_THROW_EXCEPTION(std::logic_error("Unknown path status"));
        return 0U;
    }
}

/**
 * Remove a file and anything below it in the hierarchy.
 *
 * Removes `target` on the filesystem available via `channel`.  If `target` is
 * a symlink, only removes the link, not what the link resolves to.  If
 * `target` is a directory, removes it and all its contents.
 *
 * @returns the number of files removed.
 *
 * If the calling code already knows whether `target` is a directory, this
 * function adds the overhead of a single extra stat call to the server above
 * what would be possible using plain SFTP unlink/rmdir.  This trip is needed
 * to find out that information and allows us to mirror the
 * POSIX/Boost.Filesystem remove functions that do not differentiate
 * directories.
 *
 * All files below the target must be statted (indirectly via directory listing)
 * by any implementation so this function adds no overhead for those.
 */
inline boost::uintmax_t remove_all(
    sftp_channel channel, const boost::filesystem::path& target)
{
    switch (detail::check_status(channel, target))
    {
    case detail::path_status::non_existent:
        return 0U;

    case detail::path_status::directory:
        return detail::remove_directory(channel, target);

    case detail::path_status::non_directory:
        // This includes 'unknown' file type.  What's the alternative?
        return detail::remove_one_file(channel, target);

    default:
        assert(false);
        BOOST_THROW_EXCEPTION(std::logic_error("Unknown path status"));
        return 0U;
    }
}

#undef SSH_THROW_LAST_SFTP_ERROR_WITH_PATH
#undef SSH_THROW_LAST_SFTP_ERROR

}} // namespace ssh::sftp

#endif
