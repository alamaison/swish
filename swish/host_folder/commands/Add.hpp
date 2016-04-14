/* Copyright (C) 2010, 2011, 2012, 2013, 2015
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

#ifndef SWISH_HOST_FOLDER_COMMANDS_ADD_HPP
#define SWISH_HOST_FOLDER_COMMANDS_ADD_HPP

#include "swish/nse/Command.hpp" // Command

#include <washer/shell/pidl.hpp> // apidl_t

namespace swish {
namespace host_folder {
namespace commands {

class Add : public swish::nse::Command
{
public:
    Add(const washer::shell::pidl::apidl_t& folder_pidl);

    virtual presentation_state state(comet::com_ptr<IShellItemArray> selection,
                                     bool ok_to_be_slow) const;

    void operator()(
        comet::com_ptr<IShellItemArray> selection,
        const swish::nse::command_site& site,
        comet::com_ptr<IBindCtx> bind_ctx) const;

private:
    washer::shell::pidl::apidl_t m_folder_pidl;
};

}}} // namespace swish::host_folder::commands

#endif
