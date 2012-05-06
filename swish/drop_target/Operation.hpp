/**
    @file

    Interface to drop target operations.

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

#ifndef SWISH_DROP_TARGET_OPERATION_HPP
#define SWISH_DROP_TARGET_OPERATION_HPP
#pragma once

#include "swish/remote_folder/swish_pidl.hpp" // absolute_path_from_swish_pidl

#include <winapi/shell/pidl.hpp> // apidl_t

#include <boost/filesystem.hpp> // wpath
#include <boost/function.hpp> // function
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <comet/ptr.h> // com_ptr

#include <cassert> // assert
#include <stdexcept> // logic_error
#include <string>

struct ISftpProvider;
struct ISftpConsumer;

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

class CopyCallback;

/**
 * Interface of operation functors making up a drop.
 */
class Operation
{
public:
	virtual winapi::shell::pidl::apidl_t pidl() const = 0;

	virtual boost::filesystem::wpath relative_path() const = 0;

	virtual void operator()(
		const resolved_destination& target,
		boost::function<void(ULONGLONG, ULONGLONG)> progress,
		comet::com_ptr<ISftpProvider> provider,
		comet::com_ptr<ISftpConsumer> consumer,
		CopyCallback& callback)
		const = 0;

	Operation* clone() const
	{
		Operation* item = do_clone();
		assert(typeid(*this) == typeid(*item) &&
			"do_clone() sliced object!");
		return item;
	}

private:
	virtual Operation* do_clone() const = 0;
};

inline Operation* new_clone(const Operation& item)
{
	return item.clone();
}

}} // namespace swish::drop_target

#endif
