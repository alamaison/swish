/**
    @file

    Wrap CDropTarget to show errors to user.

    @if license

    Copyright (C) 2010, 2011, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_DROP_TARGET_DROPTARGETERRORUI_HPP
#define SWISH_DROP_TARGET_DROPTARGETERRORUI_HPP
#pragma once

#include "DropTarget.hpp" // CDropTarget wrapped inner

#include <winapi/shell/pidl.hpp> // apidl_t

#include <boost/filesystem.hpp>  // wpath
#include <boost/shared_ptr.hpp> // shared_ptr to UI callback

#include <comet/ptr.h> // com_ptr
#include <comet/server.h> // simple_object

namespace swish {
namespace drop_target {

/**
 * Layer around CDropTarget that reports errors to the user.
 *
 * This keeps UI out of CDropTarget.  This class only reports errors
 * during Drop as throwing up UI during the other parts of the drag-drop
 * cycle would be distracting.
 */
class CSnitchingDropTarget :
    public comet::simple_object<IDropTarget, IObjectWithSite>
{
public:

    CSnitchingDropTarget(
        HWND hwnd_owner, comet::com_ptr<ISftpProvider> provider,
        comet::com_ptr<ISftpConsumer> consumer,
        const winapi::shell::pidl::apidl_t& remote_directory,
        boost::shared_ptr<DropActionCallback> callback);

    /** @name IDropTarget methods */
    // @{

    IFACEMETHODIMP DragEnter( 
        IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

    IFACEMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

    IFACEMETHODIMP DragLeave();

    IFACEMETHODIMP Drop(
        IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

    // @}

    /** @name IObjectWithSite methods */
    // @{

    IFACEMETHODIMP SetSite(IUnknown* pUnkSite);
    IFACEMETHODIMP GetSite(REFIID riid, void** ppvSite);

    // @}

private:

    comet::com_ptr<IDropTarget> m_inner;
    HWND m_hwnd_owner;
};

}} // namespace swish::drop_target

#endif
