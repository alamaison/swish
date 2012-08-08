/**
    @file

    Expose the remote filesystem as an IDropTarget.

    @if license

    Copyright (C) 2009, 2010, 2011, 2012
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

#ifndef SWISH_DROP_TARGET_DROPTARGET_HPP
#define SWISH_DROP_TARGET_DROPTARGET_HPP
#pragma once

#include "swish/provider/sftp_provider.hpp" // sftp_provider, ISftpConsumer
#include "swish/drop_target/DropActionCallback.hpp" // DropActionCallback
#include "swish/drop_target/Progress.hpp" // Progress

#include <winapi/object_with_site.hpp> // object_with_site
#include <winapi/shell/pidl.hpp> // apidl_t

#include <boost/shared_ptr.hpp>

#include <comet/ptr.h> // com_ptr
#include <comet/server.h> // simple_object

#include <OleIdl.h> // IDropTarget
#include <OCIdl.h> // IObjectWithSite


template<> struct comet::comtype<IDropTarget>
{
    static const IID& uuid() throw() { return IID_IDropTarget; }
    typedef IUnknown base;
};

namespace swish {
namespace drop_target {

class CDropTarget :
    public comet::simple_object<IDropTarget, winapi::object_with_site>
{
public:

    typedef IDropTarget interface_is;

    CDropTarget(
        boost::shared_ptr<swish::provider::sftp_provider> provider,
        comet::com_ptr<ISftpConsumer> consumer,
        const winapi::shell::pidl::apidl_t& remote_directory,
        boost::shared_ptr<DropActionCallback> callback);

    /** @name IDropTarget methods */
    // @{

    IFACEMETHODIMP DragEnter( 
        __in_opt IDataObject* pDataObj,
        __in DWORD grfKeyState,
        __in POINTL pt,
        __inout DWORD* pdwEffect);

    IFACEMETHODIMP DragOver( 
        __in DWORD grfKeyState,
        __in POINTL pt,
        __inout DWORD* pdwEffect);

    IFACEMETHODIMP DragLeave();

    IFACEMETHODIMP Drop( 
        __in_opt IDataObject* pDataObj,
        __in DWORD grfKeyState,
        __in POINTL pt,
        __inout DWORD* pdwEffect);

    // @}

private:

    virtual void on_set_site(comet::com_ptr<IUnknown> ole_site);

    boost::shared_ptr<swish::provider::sftp_provider> m_provider;
    comet::com_ptr<ISftpConsumer> m_consumer;

    winapi::shell::pidl::apidl_t m_remote_directory;
    comet::com_ptr<IDataObject> m_data_object;
    boost::shared_ptr<DropActionCallback> m_callback;
};

void copy_data_to_provider(
    comet::com_ptr<IDataObject> data_object,
    boost::shared_ptr<swish::provider::sftp_provider> provider,
    comet::com_ptr<ISftpConsumer> consumer,
    const winapi::shell::pidl::apidl_t& remote_directory,
    DropActionCallback& callback);

}} // namespace swish::drop_target

#endif
