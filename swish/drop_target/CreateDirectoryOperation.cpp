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

#include "CreateDirectoryOperation.hpp"

#include "swish/shell_folder/SftpDirectory.h" // CSftpDirectory

using winapi::shell::pidl::pidl_t;
using winapi::shell::pidl::apidl_t;

using boost::filesystem::wpath;
using boost::function;

using comet::com_ptr;

namespace swish {
namespace drop_target {

CreateDirectoryOperation::CreateDirectoryOperation(
	const apidl_t& root_pidl, const pidl_t& pidl, const wpath& relative_path) :
m_root_pidl(root_pidl), m_pidl(pidl), m_relative_path(relative_path)
{}

apidl_t CreateDirectoryOperation::pidl() const
{
	return m_root_pidl + m_pidl;
}

wpath CreateDirectoryOperation::relative_path() const
{
	return m_relative_path;
}

void CreateDirectoryOperation::operator()(
	const resolved_destination& target, 
	function<void(ULONGLONG, ULONGLONG)> progress,
	com_ptr<ISftpProvider> provider, com_ptr<ISftpConsumer> consumer,
	CopyCallback& /*callback*/)
	const
{
	progress(0, 1);

	CSftpDirectory sftp_directory(target.directory(), provider, consumer);
	sftp_directory.CreateDirectory(target.filename());

	progress(1, 1);
}

Operation* CreateDirectoryOperation::do_clone() const
{
	return new CreateDirectoryOperation(*this);
}

}}
