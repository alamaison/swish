/**
    @file

    Wrap CDropTarget to show errors to user.

    @if license

    Copyright (C) 2010, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "SnitchingDropTarget.hpp"

#include "swish/provider/sftp_provider.hpp" // sftp_provider, ISftpConsumer
#include "swish/frontend/announce_error.hpp" // rethrow_and_announce

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <boost/locale.hpp> // translate
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/shared_ptr.hpp>  // shared_ptr

#include <comet/error.h> // com_error
#include <comet/ptr.h>  // com_ptr

using swish::frontend::rethrow_and_announce;
using swish::provider::sftp_provider;

using winapi::shell::pidl::apidl_t;

using boost::filesystem::wpath;
using boost::locale::translate;
using boost::shared_ptr;

using comet::com_error;
using comet::com_ptr;

namespace swish {
namespace drop_target {

CSnitchingDropTarget::CSnitchingDropTarget(
    HWND hwnd_owner, shared_ptr<sftp_provider> provider,
    com_ptr<ISftpConsumer> consumer, const apidl_t& remote_directory,
    shared_ptr<DropActionCallback> callback)
    :
    m_inner(new CDropTarget(provider, consumer, remote_directory, callback)),
    m_hwnd_owner(hwnd_owner) {}

STDMETHODIMP CSnitchingDropTarget::SetSite(IUnknown* pUnkSite)
{
    try
    {
        com_ptr<IObjectWithSite> obj = try_cast(m_inner);
        return obj->SetSite(pUnkSite);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

STDMETHODIMP CSnitchingDropTarget::GetSite(REFIID riid, void** ppvSite)
{
    try
    {
        com_ptr<IObjectWithSite> obj = try_cast(m_inner);
        return obj->GetSite(riid, ppvSite);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

STDMETHODIMP CSnitchingDropTarget::DragEnter( 
    IDataObject* pdo, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    return m_inner->DragEnter(pdo, grfKeyState, pt, pdwEffect);
}

STDMETHODIMP CSnitchingDropTarget::DragOver( 
    DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    return m_inner->DragOver(grfKeyState, pt, pdwEffect);
}

STDMETHODIMP CSnitchingDropTarget::DragLeave()
{
    return m_inner->DragLeave();
}

/**
 * Report any error encountered during Drop to the user with a GUI message.
 */
STDMETHODIMP CSnitchingDropTarget::Drop( 
    IDataObject* pdo, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    HRESULT hr = S_OK;

    try
    {
        try
        {
            hr = m_inner->Drop(pdo, grfKeyState, pt, pdwEffect);
            if (FAILED(hr))
            {
                BOOST_THROW_EXCEPTION(com_error_from_interface(m_inner, hr));
            }
        }
        catch (...)
        {
            rethrow_and_announce(
                m_hwnd_owner, translate("Unable to transfer files"),
                translate(
                    "You might not have permission to write to this "
                    "directory."));
        }
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return hr;
}

}} // namespace swish::drop_target
