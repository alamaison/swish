/**
    @file

    Abstraction of SFTP drop destination.

    @if license

    Copyright (C) 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    @endif
*/

#ifndef SWISH_DROP_TARGET_SFTPDESTINATION_HPP
#define SWISH_DROP_TARGET_SFTPDESTINATION_HPP
#pragma once

#include "swish/remote_folder/swish_pidl.hpp" // absolute_path_from_swish_pidl
#include "swish/remote_folder/remote_pidl.hpp" // create_remote_itemid

#include <winapi/shell/pidl.hpp> // apidl_t

#include <boost/filesystem.hpp> // wpath
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <stdexcept> // logic_error
#include <string>

namespace swish {
namespace drop_target {

/**
 * A destination (directory or file) on the remote server given as a
 * directory PIDL and a filename.
 */
class resolved_destination
{
public:
	resolved_destination(
		const winapi::shell::pidl::apidl_t& remote_directory,
		const std::wstring& filename)
		: m_remote_directory(remote_directory), m_filename(filename)
	{
		if (boost::filesystem::wpath(m_filename).has_parent_path())
			BOOST_THROW_EXCEPTION(
				std::logic_error(
					"Path not properly resolved; filename expected"));
	}

	const winapi::shell::pidl::apidl_t& directory() const
	{
		return m_remote_directory;
	}

	const std::wstring filename() const
	{
		return m_filename;
	}

	boost::filesystem::wpath as_absolute_path() const
	{
		return swish::remote_folder::absolute_path_from_swish_pidl(
			m_remote_directory) / m_filename;
	}

private:
	winapi::shell::pidl::apidl_t m_remote_directory;
	std::wstring m_filename;
};

/**
 * A destination (directory or file) on the remote server given as a
 * path relative to a PIDL.
 *
 * As in an FGD, the path may be multi-level.  The directories named by the
 * intermediate sections may not exist so care must be taken that the,
 * destinations are used in the order listed in the FGD which is designed
 * to make sure they exist.
 */
class SftpDestination
{
public:
	SftpDestination(
		const winapi::shell::pidl::apidl_t& remote_root,
		const boost::filesystem::wpath& relative_path)
		: m_remote_root(remote_root), m_relative_path(relative_path)
	{
		if (relative_path.has_root_directory())
			BOOST_THROW_EXCEPTION(
				std::logic_error("Path must be relative to root"));
	}

	resolved_destination resolve_destination() const
	{
		winapi::shell::pidl::apidl_t directory = m_remote_root;

		BOOST_FOREACH(
			std::wstring intermediate_directory_name,
			m_relative_path.parent_path())
		{
			directory += swish::remote_folder::create_remote_itemid(
				intermediate_directory_name, true, false, L"", L"", 0, 0, 0, 0,
				comet::datetime_t::now(), comet::datetime_t::now());
		}

		return resolved_destination(directory, m_relative_path.filename());
	}

	SftpDestination operator/(const boost::filesystem::wpath& path) const
	{
		return SftpDestination(m_remote_root, m_relative_path / path);
	}

private:
	winapi::shell::pidl::apidl_t m_remote_root;
	boost::filesystem::wpath m_relative_path;
};

}}

#endif
