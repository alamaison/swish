/**
    @file

    Directory creation operation.

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

#ifndef SWISH_DROP_TARGET_CREATEDIRECTORYOPERATION_HPP
#define SWISH_DROP_TARGET_CREATEDIRECTORYOPERATION_HPP
#pragma once

#include "swish/drop_target/Operation.hpp" // Operation

namespace swish {
namespace drop_target {

class CreateDirectoryOperation : public Operation
{
public:

	CreateDirectoryOperation(
		const winapi::shell::pidl::apidl_t& root_pidl,
		const winapi::shell::pidl::pidl_t& pidl,
		const boost::filesystem::wpath& relative_path);

	virtual winapi::shell::pidl::apidl_t pidl() const;

	virtual boost::filesystem::wpath relative_path() const;

	virtual void operator()(
		const resolved_destination& target, 
		boost::function<void(ULONGLONG, ULONGLONG)> progress,
		comet::com_ptr<ISftpProvider> provider,
		comet::com_ptr<ISftpConsumer> consumer,
		CopyCallback& callback) const;

private:

	virtual Operation* do_clone() const;

	winapi::shell::pidl::apidl_t m_root_pidl;
	winapi::shell::pidl::pidl_t m_pidl;
	boost::filesystem::wpath m_relative_path;
};

}}

#endif
