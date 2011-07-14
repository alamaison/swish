/**
    @file

	SFTP symlinks in Explorer

    @if license

    Copyright (C) 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/remote_folder/symlink.hpp"

#include "swish/shell_folder/SftpDirectory.h" // CSftpDirectory

#include <comet/error.h> // com_error_from_interface

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

using comet::com_error_from_interface;
using comet::com_ptr;

using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;

namespace swish {
namespace remote_folder {

com_ptr<IShellLinkW> pidl_to_shell_link(
	const apidl_t& parent_directory, const cpidl_t& item,
	com_ptr<ISftpProvider> provider, com_ptr<ISftpConsumer> consumer)
{
	CSftpDirectory directory(parent_directory, provider, consumer);
	apidl_t target = directory.ResolveLink(item);
	
	// This is not the best way to do it.  It would be better to reimplement
	// IShellLink so that it resolved the symlink on demand.  The current method
	// means that listing a directory resolves every link in it
	com_ptr<IShellLinkW> link(CLSID_ShellLink);
	HRESULT hr = link->SetIDList(target.get());
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_error_from_interface(link, hr));

	return link;
}

}} // namespace swish::remote_folder
