/**
    @file

    New remote folder command.

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

#ifndef SWISH_REMOTE_FOLDER_COMMANDS_NEW_FOLDER_HPP
#define SWISH_REMOTE_FOLDER_COMMANDS_NEW_FOLDER_HPP
#pragma once

#include "swish/interfaces/SftpProvider.h" // ISftpProvider, ISftpConsumer
#include "swish/nse/Command.hpp" // Command

#include <winapi/shell/pidl.hpp> // apidl_t

#include <comet/ptr.h> // com_ptr

#include <boost/function.hpp> // function

#include <ObjIdl.h> // IDataObject, IBindCtx

namespace swish {
namespace remote_folder {
namespace commands {

class NewFolder : public swish::nse::Command
{
public:
    NewFolder(
        const winapi::shell::pidl::apidl_t& folder_pidl,
        const boost::function<comet::com_ptr<ISftpProvider>()>& provider,
        const boost::function<comet::com_ptr<ISftpConsumer>()>& consumer);
    
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
    boost::function<comet::com_ptr<ISftpProvider>()> m_provider;
    boost::function<comet::com_ptr<ISftpConsumer>()> m_consumer;
    comet::com_ptr<IUnknown> m_site;
};

}}} // namespace swish::remote_folder::commands

#endif
