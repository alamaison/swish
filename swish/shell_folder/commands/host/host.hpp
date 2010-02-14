/**
    @file

    Swish host folder commands.

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

    @endif
*/

#pragma once

#include "swish/shell_folder/pidl.hpp" // apidl_t
#include "swish/shell_folder/commands/Command.hpp" // Command

#include <comet/ptr.h> // com_ptr

#include <ObjIdl.h> // IDataObject, IBindCtx
#include <shobjidl.h> // IExplorerCommandProvider, IShellItemArray

namespace swish {
namespace shell_folder {
namespace commands {
namespace host {

class Add : public swish::shell_folder::commands::Command
{
public:
	Add(HWND hwnd, const swish::shell_folder::pidl::apidl_t& folder_pidl);
	
	bool disabled(const comet::com_ptr<IDataObject>& data_object,
		bool ok_to_be_slow) const;
	bool hidden(const comet::com_ptr<IDataObject>& data_object,
		bool ok_to_be_slow) const;

	void operator()(
		const comet::com_ptr<IDataObject>& data_object,
		const comet::com_ptr<IBindCtx>& bind_ctx) const;

private:
	HWND m_hwnd;
	swish::shell_folder::pidl::apidl_t m_folder_pidl;
};

class Remove : public swish::shell_folder::commands::Command
{
public:
	Remove(HWND hwnd, const swish::shell_folder::pidl::apidl_t& folder_pidl);
	
	bool disabled(
		const comet::com_ptr<IDataObject>& data_object,
		bool ok_to_be_slow) const;
	bool hidden(const comet::com_ptr<IDataObject>& data_object,
		bool ok_to_be_slow) const;

	void operator()(
		const comet::com_ptr<IDataObject>& data_object,
		const comet::com_ptr<IBindCtx>& bind_ctx) const;

private:
	HWND m_hwnd;
	swish::shell_folder::pidl::apidl_t m_folder_pidl;
};
	
comet::com_ptr<IExplorerCommandProvider> host_folder_command_provider(
	HWND hwnd, const swish::shell_folder::pidl::apidl_t& folder_pidl);

}}}} // namespace swish::shell_folder::commands::host
