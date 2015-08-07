/* Copyright (C) 2015  Alexander Lamaison <swish@lammy.co.uk>

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

#ifndef SWISH_HOST_FOLDER_COMMANDS_RENAME_HPP
#define SWISH_HOST_FOLDER_COMMANDS_RENAME_HPP

#include "swish/nse/Command.hpp" // Command

#include <washer/shell/pidl.hpp> // apidl_t

#include <comet/ptr.h> // com_ptr

#include <ObjIdl.h> // IDataObject, IBindCtx

namespace swish {
namespace host_folder {
namespace commands {

class Rename : public swish::nse::Command
{
public:
    Rename(HWND hwnd, const washer::shell::pidl::apidl_t& folder_pidl);

    virtual BOOST_SCOPED_ENUM(state) state(
        const comet::com_ptr<IDataObject>& data_object,
        bool ok_to_be_slow) const;

    void operator()(
        const comet::com_ptr<IDataObject>& data_object,
        const comet::com_ptr<IBindCtx>& bind_ctx) const;

    void set_site(comet::com_ptr<IUnknown> ole_site);

private:
    HWND m_hwnd;
    washer::shell::pidl::apidl_t m_folder_pidl;
    comet::com_ptr<IUnknown> m_site;
};

}}} // namespace swish::host_folder::commands


#endif
