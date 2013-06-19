/**
    @file

    Remove host command.

    @if license

    Copyright (C) 2010, 2011, 2012, 2013
    Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_HOST_FOLDER_COMMANDS_REMOVE_HPP
#define SWISH_HOST_FOLDER_COMMANDS_REMOVE_HPP
#pragma once

#include "swish/nse/Command.hpp" // Command

#include <winapi/shell/pidl.hpp> // apidl_t

#include <comet/ptr.h> // com_ptr

#include <ObjIdl.h> // IDataObject, IBindCtx

namespace swish {
namespace host_folder {
namespace commands {

class Remove : public swish::nse::Command
{
public:
    Remove(HWND hwnd, const winapi::shell::pidl::apidl_t& folder_pidl);

    virtual BOOST_SCOPED_ENUM(state) state(
        const comet::com_ptr<IDataObject>& data_object,
        bool ok_to_be_slow) const;

    void operator()(
        const comet::com_ptr<IDataObject>& data_object,
        const comet::com_ptr<IBindCtx>& bind_ctx) const;

private:
    HWND m_hwnd;
    winapi::shell::pidl::apidl_t m_folder_pidl;
};

}}} // namespace swish::host_folder::commands

#endif
