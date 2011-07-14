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

#ifndef SWISH_SHELL_FOLDER_SYMLINK_HPP
#define SWISH_SHELL_FOLDER_SYMLINK_HPP
#pragma once

#include "swish/interfaces/SftpProvider.h" // ISftpProvider, ISftpConsumer

#include <winapi/shell/pidl.hpp> // apidl_t

#include <comet/ptr.h> // com_ptr

#include <string>

#include <ShObjIdl.h> // IShellLinkW

namespace comet {

	template<> struct comtype<IShellLinkW>
	{
		static const IID& uuid() { return IID_IShellLinkW; }
		typedef IUnknown base;
	};

}

namespace swish {
namespace remote_folder {

comet::com_ptr<IShellLinkW> pidl_to_shell_link(
	const winapi::shell::pidl::apidl_t& directory,
	const winapi::shell::pidl::cpidl_t& item,
	comet::com_ptr<ISftpProvider> provider,
	comet::com_ptr<ISftpConsumer> consumer);


}} // namespace swish::remote_folder

#endif
