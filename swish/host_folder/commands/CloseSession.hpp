/* Copyright (C) 2013, 2015
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

#ifndef SWISH_HOST_FOLDER_COMMANDS_CLOSE_SESSION_HPP
#define SWISH_HOST_FOLDER_COMMANDS_CLOSE_SESSION_HPP
#pragma once

#include "swish/nse/Command.hpp" // Command

namespace swish {
namespace host_folder {
namespace commands {

class CloseSession : public swish::nse::Command
{
public:
    CloseSession();

    virtual presentation_state state(comet::com_ptr<IShellItemArray> selection,
                                     bool ok_to_be_slow) const;

    void operator()(
        comet::com_ptr<IShellItemArray> selection,
        const swish::nse::command_site& site,
        comet::com_ptr<IBindCtx> bind_ctx) const;
};

}}} // namespace swish::host_folder::commands

#endif
