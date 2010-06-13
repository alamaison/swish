/**
    @file

    SSH SFTP subsystem.

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

#ifndef SSH_SFTP_HPP
#define SSH_SFTP_HPP
#pragma once

#include "exception.hpp" // last_error
#include "host_key.hpp" // host_key

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo_api_function
#include <boost/filesystem/path.hpp> // path
#include <boost/iterator/iterator_facade.hpp> // iterator_facade
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <algorithm> // min
#include <exception> // bad_alloc
#include <string>
#include <vector>

#include <libssh2_sftp.h>

namespace ssh {
namespace sftp {

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
					ssh::exception::last_error(session) <<
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
				BOOST_THROW_EXCEPTION(
					ssh::exception::last_error(session) <<
					boost::errinfo_api_function("libssh2_sftp_open_ex"));

			return boost::shared_ptr<LIBSSH2_SFTP_HANDLE>(
				handle, libssh2_sftp_close_handle);
		}

	}}
}

class sftp_channel
{
public:

	/**
	 * Open a new SFTP channel in an SSH session.
	 */
	explicit sftp_channel(::ssh::session::session session)
		:
		m_session(session),
		m_sftp(detail::libssh2::sftp::init(session.get())) {}

	/**
	 * Attach to an existing, open SFTP channel.
	 */
	sftp_channel(
		::ssh::session::session session,
		boost::shared_ptr<LIBSSH2_SFTP> sftp)
		:
		m_session(session),
		m_sftp(sftp) {}

	boost::shared_ptr<LIBSSH2_SFTP> get() { return m_sftp; }
	ssh::session::session session() { return m_session; }

private:
	ssh::session::session m_session;
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

class sftp_file
{
public:
	sftp_file(
		const boost::filesystem::path& file, const std::string& long_entry,
		const LIBSSH2_SFTP_ATTRIBUTES& attributes)
		:
		m_file(file), m_long_entry(long_entry), m_attributes(attributes) {}

	const boost::filesystem::path& name() const { return m_file; }
	const std::string& long_entry() const { return m_long_entry; }

private:
	boost::filesystem::path m_file;
	std::string m_long_entry;
	LIBSSH2_SFTP_ATTRIBUTES m_attributes;
};

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
			BOOST_THROW_EXCEPTION(std::logic_error("No more files"));
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
			BOOST_THROW_EXCEPTION(
				ssh::exception::last_error(m_session) <<
				boost::errinfo_api_function("libssh2_sftp_readdir_ex"));
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
				std::range_error("Can't dereference the end of a collection"));

		return sftp_file(m_file_name, m_long_entry, m_attributes);
	}

	boost::shared_ptr<LIBSSH2_SESSION> m_session;
	boost::shared_ptr<LIBSSH2_SFTP> m_channel;
	boost::shared_ptr<LIBSSH2_SFTP_HANDLE> m_handle;

	/// @name Properties of last successfully listed file.
	// @{
	std::string m_file_name;
	std::string m_long_entry;
	LIBSSH2_SFTP_ATTRIBUTES m_attributes;
	// @}
};

}} // namespace ssh::sftp

#endif
