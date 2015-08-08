/* Copyright (C) 2011, 2012, 2013, 2015
   Alexander Lamaison <swish@lammy.co.uk>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation, either version 3 of the License, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SWISH_REMOTE_FOLDER_COMMANDS_HPP
#define SWISH_REMOTE_FOLDER_COMMANDS_HPP
#pragma once

#include "swish/provider/sftp_provider.hpp" // sftp_provider, ISftpConsumer
#include "swish/nse/UICommand.hpp" // IUIElement

#include <washer/shell/pidl.hpp> // apidl_t
#include <washer/window/window.hpp>

#include <comet/ptr.h> // com_ptr

#include <boost/function.hpp>
#include <boost/optional/optional.hpp>
#include <boost/shared_ptr.hpp>

#include <shobjidl.h> // IExplorerCommandProvider

namespace swish {
namespace remote_folder {
namespace commands {

typedef boost::function<
    boost::shared_ptr<swish::provider::sftp_provider>(
        comet::com_ptr<ISftpConsumer>, const std::string& task_name)>
    provider_factory;

typedef boost::function<comet::com_ptr<ISftpConsumer>()> consumer_factory;

comet::com_ptr<IExplorerCommandProvider> remote_folder_command_provider(
    const boost::optional<washer::window::window<wchar_t>>& parent_window,
    const washer::shell::pidl::apidl_t& folder_pidl,
    const provider_factory& provider,
    const consumer_factory& consumer);

std::pair<comet::com_ptr<nse::IUIElement>, comet::com_ptr<nse::IUIElement> >
remote_folder_task_pane_titles(
    const boost::optional<washer::window::window<wchar_t>>& parent_window,
    const washer::shell::pidl::apidl_t& folder_pidl);

std::pair<
    comet::com_ptr<nse::IEnumUICommand>,
    comet::com_ptr<nse::IEnumUICommand> >
remote_folder_task_pane_tasks(
    const boost::optional<washer::window::window<wchar_t>>& parent_window,
    const washer::shell::pidl::apidl_t& folder_pidl,
    comet::com_ptr<IUnknown> ole_site,
    const provider_factory& provider,
    const consumer_factory& consumer);

}}} // namespace swish::remote_folder::commands

#endif
