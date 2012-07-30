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

#ifndef SWISH_REMOTE_FOLDER_COMMANDS_DELETE_HPP
#define SWISH_REMOTE_FOLDER_COMMANDS_DELETE_HPP
#pragma once

#include "swish/interfaces/SftpProvider.h" // ISftpProvider, ISftpConsumer

#include <comet/ptr.h> // com_ptr

#include <boost/function.hpp> // function

#include <ObjIdl.h> // IDataObject

namespace swish {
namespace remote_folder {
namespace commands {

class Delete
{
public:
    Delete(
        boost::function<comet::com_ptr<ISftpProvider>(HWND)> provider_factory,
        boost::function<comet::com_ptr<ISftpConsumer>(HWND)> consumer_factory);

    void operator()(
        HWND hwnd_view, comet::com_ptr<IDataObject> selection) const;

private:
    boost::function<comet::com_ptr<ISftpProvider>(HWND)> m_provider_factory;
    boost::function<comet::com_ptr<ISftpConsumer>(HWND)> m_consumer_factory;
};

}}} // namespace swish::remote_folder::commands

#endif
