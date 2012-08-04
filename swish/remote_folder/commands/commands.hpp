/**
    @file

    Swish remote folder commands.

    @if license

    Copyright (C) 2011, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_REMOTE_FOLDER_COMMANDS_HPP
#define SWISH_REMOTE_FOLDER_COMMANDS_HPP
#pragma once

#include "swish/provider/SftpProvider.h" // ISftpProvider, ISftpConsumer
#include "swish/nse/UICommand.hpp" // IUIElement

#include <winapi/shell/pidl.hpp> // apidl_t

#include <comet/ptr.h> // com_ptr

#include <boost/function.hpp> // function

#include <shobjidl.h> // IExplorerCommandProvider

namespace swish {
namespace remote_folder {
namespace commands {

comet::com_ptr<IExplorerCommandProvider> remote_folder_command_provider(
    HWND hwnd, const winapi::shell::pidl::apidl_t& folder_pidl,
    const boost::function<comet::com_ptr<ISftpProvider>()>& provider,
    const boost::function<comet::com_ptr<ISftpConsumer>()>& consumer);

std::pair<comet::com_ptr<nse::IUIElement>, comet::com_ptr<nse::IUIElement> >
remote_folder_task_pane_titles(
    HWND hwnd, const winapi::shell::pidl::apidl_t& folder_pidl);

std::pair<
    comet::com_ptr<nse::IEnumUICommand>,
    comet::com_ptr<nse::IEnumUICommand> >
remote_folder_task_pane_tasks(
    HWND hwnd, const winapi::shell::pidl::apidl_t& folder_pidl,
    comet::com_ptr<IUnknown> ole_site,
    const boost::function<comet::com_ptr<ISftpProvider>()>& provider,
    const boost::function<comet::com_ptr<ISftpConsumer>()>& consumer);

}}} // namespace swish::remote_folder::commands

#endif
