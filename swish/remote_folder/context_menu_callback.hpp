/**
    @file

    RemoteFolder context menu implementation (basically that what it does).

    @if license

    Copyright (C) 2011, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_REMOTE_FOLDER_CONTEXT_MENU_CALLBACK_HPP
#define SWISH_REMOTE_FOLDER_CONTEXT_MENU_CALLBACK_HPP
#pragma once

#include "swish/provider/sftp_provider.hpp" // sftp_provider, ISftpConsumer
#include "swish/nse/default_context_menu_callback.hpp"
                                               // default_context_menu_callback

#include <comet/ptr.h> // com_ptr

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <string>

#include <ObjIdl.h> // IDataObject
#include <ShObjIdl.h> // CMINVOKECOMMANDINFO
#include <Windows.h> // HWND, HMENU

namespace swish {
namespace remote_folder {

class context_menu_callback : public swish::nse::default_context_menu_callback
{
    typedef boost::function<
        boost::shared_ptr<swish::provider::sftp_provider>(
        comet::com_ptr<ISftpConsumer>, const std::string&)
    > my_provider_factory;

    typedef boost::function<comet::com_ptr<ISftpConsumer>(HWND)>
        my_consumer_factory;

public:
    context_menu_callback(
        my_provider_factory provider_factory,
        my_consumer_factory consumer_factory);

private:
    bool merge_context_menu(
        HWND hwnd_view, comet::com_ptr<IDataObject> selection, HMENU hmenu,
        UINT first_item_index, UINT& minimum_id, UINT maximum_id,
        UINT allowed_changes_flags);

    void verb(
        HWND hwnd_view, comet::com_ptr<IDataObject> selection,
        UINT command_id_offset, std::wstring& verb_out);

    void verb(
        HWND hwnd_view, comet::com_ptr<IDataObject> selection,
        UINT command_id_offset, std::string& verb_out);

    bool invoke_command(
        HWND hwnd_view, comet::com_ptr<IDataObject> selection, UINT item_offset,
        const std::wstring& arguments);

    bool invoke_command(
        HWND hwnd_view, comet::com_ptr<IDataObject> selection, UINT item_offset,
        const std::wstring& arguments, DWORD behaviour_flags, UINT minimum_id,
        UINT maximum_id, const CMINVOKECOMMANDINFO& invocation_details,
        comet::com_ptr<IUnknown> context_menu_site);

    bool default_menu_item(
        HWND hwnd_view, comet::com_ptr<IDataObject> selection,
        UINT& default_command_id);

    my_provider_factory m_provider_factory;
    my_consumer_factory m_consumer_factory;
};

}} // namespace swish::remote_folder

#endif
