/**
    @file

    Swish remote folder commands.

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

#ifndef SWISH_SHELL_FOLDER_COMMANDS_REMOTE_REMOTE_HPP
#define SWISH_SHELL_FOLDER_COMMANDS_REMOTE_REMOTE_HPP
#pragma once

#include "swish/interfaces/SftpProvider.h" // ISftpProvider, ISftpConsumer
#include "swish/nse/UICommand.hpp" // IUIElement
#include "swish/shell_folder/commands/Command.hpp" // Command

#include <winapi/shell/pidl.hpp> // apidl_t

#include <comet/ptr.h> // com_ptr

#include <ObjIdl.h> // IDataObject, IBindCtx
#include <shobjidl.h> // IExplorerCommandProvider, IShellItemArray

namespace swish {
namespace shell_folder {
namespace commands {
namespace remote {

class NewFolder : public swish::shell_folder::commands::Command
{
public:
	NewFolder(
		const winapi::shell::pidl::apidl_t& folder_pidl,
		comet::com_ptr<ISftpProvider> provider,
		comet::com_ptr<ISftpConsumer> consumer);
	
	bool disabled(const comet::com_ptr<IDataObject>& data_object,
		bool ok_to_be_slow) const;
	bool hidden(const comet::com_ptr<IDataObject>& data_object,
		bool ok_to_be_slow) const;

	void operator()(
		const comet::com_ptr<IDataObject>& data_object,
		const comet::com_ptr<IBindCtx>& bind_ctx) const;

	void set_site(comet::com_ptr<IUnknown> ole_site);

private:
	HWND m_hwnd;
	winapi::shell::pidl::apidl_t m_folder_pidl;
	comet::com_ptr<ISftpProvider> m_provider;
	comet::com_ptr<ISftpConsumer> m_consumer;
	comet::com_ptr<IUnknown> m_site;
};

comet::com_ptr<IExplorerCommandProvider> remote_folder_command_provider(
	const winapi::shell::pidl::apidl_t& folder_pidl,
	comet::com_ptr<ISftpProvider> provider,
	comet::com_ptr<ISftpConsumer> consumer);

std::pair<comet::com_ptr<nse::IUIElement>, comet::com_ptr<nse::IUIElement> >
remote_folder_task_pane_titles();

std::pair<
	comet::com_ptr<nse::IEnumUICommand>,
	comet::com_ptr<nse::IEnumUICommand> >
remote_folder_task_pane_tasks(
	const winapi::shell::pidl::apidl_t& folder_pidl,
	comet::com_ptr<ISftpProvider> provider,
	comet::com_ptr<ISftpConsumer> consumer);

}}}} // namespace swish::shell_folder::commands::remote

#endif
