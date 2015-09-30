/* Copyright (C) 2010, 2011, 2012, 2015
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

#ifndef SWISH_HOST_FOLDER_COMMANDS_HPP
#define SWISH_HOST_FOLDER_COMMANDS_HPP
#pragma once

#include "swish/nse/UICommand.hpp" // IUIElement

#include <washer/shell/pidl.hpp> // apidl_t

#include <comet/ptr.h> // com_ptr

#include <shobjidl.h> // IExplorerCommandProvider

namespace swish {
namespace host_folder {
namespace commands {

comet::com_ptr<IExplorerCommandProvider> host_folder_command_provider(
    const washer::shell::pidl::apidl_t& folder_pidl);

std::pair<comet::com_ptr<nse::IUIElement>, comet::com_ptr<nse::IUIElement> >
host_folder_task_pane_titles(const washer::shell::pidl::apidl_t& folder_pidl);

std::pair<
    comet::com_ptr<nse::IEnumUICommand>,
    comet::com_ptr<nse::IEnumUICommand> >
host_folder_task_pane_tasks(const washer::shell::pidl::apidl_t& folder_pidl);

}}} // namespace swish::host_folder::commands

#endif
