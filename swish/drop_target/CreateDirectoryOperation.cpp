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

using std::wstring;

namespace swish {
namespace drop_target {

CreateDirectoryOperation::CreateDirectoryOperation(
	const SftpDestination& target) : m_destination(target) {}

wstring CreateDirectoryOperation::title() const
{
	return m_destination.resolve_destination().filename();
}

wstring CreateDirectoryOperation::description() const
{
	return m_destination.resolve_destination().as_absolute_path().string();
}

void CreateDirectoryOperation::operator()(
	OperationCallback& callback,
	com_ptr<ISftpProvider> provider, com_ptr<ISftpConsumer> consumer) const
{
	callback.update_progress(0, 1);

	resolved_destination resolved_target(m_destination.resolve_destination());

	CSftpDirectory sftp_directory(
		resolved_target.directory(), provider, consumer);
	sftp_directory.CreateDirectory(resolved_target.filename());

	callback.update_progress(1, 1);
}

Operation* CreateDirectoryOperation::do_clone() const
{
	return new CreateDirectoryOperation(*this);
}

}}
