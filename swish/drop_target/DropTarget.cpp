/**
    @file

    Expose the remote filesystem as an IDropTarget.

    @if license

    Copyright (C) 2009, 2010, 2012, 2013
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

#include "DropTarget.hpp"

#include "swish/drop_target/PidlCopyPlan.hpp"
#include "swish/provider/sftp_provider.hpp" // sftp_provider, ISftpConsumer
#include "swish/shell_folder/data_object/ShellDataObject.hpp"
                                                  // PidlFormat, ShellDataObject

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <boost/shared_ptr.hpp>  // shared_ptr
#include <boost/thread.hpp>
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <comet/error.h> // com_error
#include <comet/git.h>
#include <comet/ptr.h>  // com_ptr
#include <comet/util.h> // auto_coinit

using swish::shell_folder::data_object::ShellDataObject;
using swish::shell_folder::data_object::PidlFormat;
using swish::provider::sftp_provider;

using winapi::shell::pidl::pidl_t;
using winapi::shell::pidl::apidl_t;

using boost::shared_ptr;
using boost::thread;

using comet::auto_coinit;
using comet::com_error;
using comet::com_ptr;
using comet::GIT;
using comet::GIT_cookie;

template<> struct comet::comtype<IAsyncOperation>
{
    static const IID& uuid() throw() { return IID_IAsyncOperation; }
    typedef IUnknown base;
};

template<> struct comet::comtype<IDataObject>
{
    static const IID& uuid() throw() { return IID_IDataObject; }
    typedef IUnknown base;
};

namespace swish {
namespace drop_target {

namespace { // private

    /**
     * Given a DataObject and bitfield of allowed DROPEFFECTs, determine
     * which drop effect, if any, should be chosen.  If none are
     * appropriate, return DROPEFFECT_NONE.
     */
    DWORD determine_drop_effect(
        const com_ptr<IDataObject>& pdo, DWORD allowed_effects)
    {
        if (pdo)
        {
            PidlFormat format(pdo);
            if (format.pidl_count() > 0)
            {
                if (allowed_effects & DROPEFFECT_COPY)
                    return DROPEFFECT_COPY;
            }
        }

        return DROPEFFECT_NONE;
    }

}

/**
 * Copy the items in the DataObject to the remote target.
 *
 * @param source_format     Clipboard PIDL format holding the items to be copied.
 * @param provider          SFTP connection to copy data over.
 * @param destination_root  PIDL to target directory in the remote filesystem
 *                          to copy items into.
 * @param progress          Progress dialogue.
 */
void copy_format_to_provider(
    PidlFormat source_format, shared_ptr<sftp_provider> provider,
    const apidl_t& destination_root, shared_ptr<DropActionCallback> callback)
{
    PidlCopyPlan copy_list(source_format, destination_root);

    copy_list.execute_plan(*callback, provider);
}

namespace {

void async_copy_format_to_provider(
    GIT_cookie<IDataObject> marshalling_cookie,
    shared_ptr<sftp_provider> provider,
    apidl_t destination_root, shared_ptr<DropActionCallback> callback)
{
    auto_coinit com;
    GIT git;

    // These interface from the GIT will be properly marshalled across thread
    // apartments
    com_ptr<IDataObject> data_object = git.get_interface(marshalling_cookie);
    com_ptr<IAsyncOperation> async = try_cast(data_object);

    try
    {
        try
        {
            copy_format_to_provider(
                PidlFormat(data_object), provider, destination_root,
                callback);
        }
        catch (...)
        {
            callback->handle_last_exception();
            throw;
        }
    }
    catch (const com_error& e)
    {
        async->EndOperation(e.hr(), NULL, DROPEFFECT_COPY);
    }
    catch (...)
    {
        async->EndOperation(E_FAIL, NULL, DROPEFFECT_COPY);
    }

    git.revoke_interface(marshalling_cookie);
}

}

/**
 * Copy the items in the DataObject to the remote target.
 *
 * @param data_object       IDataObject holding the items to be copied.
 * @param provider          SFTP connection to copy data over.
 * @param remote_directory  PIDL to target directory in the remote filesystem
 *                          to copy items into.
 */
void copy_data_to_provider(
    com_ptr<IDataObject> data_object, shared_ptr<sftp_provider> provider, 
    const apidl_t& remote_directory, shared_ptr<DropActionCallback> callback)
{
    ShellDataObject data(data_object);
    if (data.has_pidl_format())
    {
        if (data.supports_async())
        {
            com_ptr<IAsyncOperation> async = data.async();
            HRESULT hr = async->StartOperation(NULL);
            if (FAILED(hr))
                BOOST_THROW_EXCEPTION(com_error_from_interface(async, hr));

            // We place the interfaces in the Global Interface Table because
            // the other thread needs marshalled versions of the interfaces.
            // The GIT promises to provide that.
            GIT git;
            GIT_cookie<IDataObject> marshalling_cookie =
                git.register_interface(data_object);

            thread(
                &async_copy_format_to_provider, marshalling_cookie,
                provider, remote_directory, callback).detach();
        }
        else
        {
            copy_format_to_provider(
                PidlFormat(data_object), provider, remote_directory,
                callback);
        }
    }
    else
    {
        BOOST_THROW_EXCEPTION(
            com_error("DataObject doesn't contain a supported format"));
    }
}

/**
 * Create an instance of the DropTarget initialised with a data provider.
 */
CDropTarget::CDropTarget(
    shared_ptr<sftp_provider> provider, const apidl_t& remote_directory,
    shared_ptr<DropActionCallback> callback)
    :
    m_provider(provider),
    m_remote_directory(remote_directory), m_callback(callback) {}

/**
 * Indicate whether the contents of the DataObject can be dropped on
 * this DropTarget.
 *
 * @todo  Take account of the key state.
 */
STDMETHODIMP CDropTarget::DragEnter( 
    IDataObject* pdo, DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect)
{
    try
    {
        if (!pdwEffect)
            BOOST_THROW_EXCEPTION(com_error(E_POINTER));

        m_data_object = pdo;

        *pdwEffect = determine_drop_effect(pdo, *pdwEffect);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

/**
 * Refresh the choice drop effect for the last DataObject passed to DragEnter.
 * Although the DataObject will not have changed, the key state and allowed
 * effects bitfield may have.
 *
 * @todo  Take account of the key state.
 */
STDMETHODIMP CDropTarget::DragOver( 
    DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect)
{
    try
    {
        if (!pdwEffect)
            BOOST_THROW_EXCEPTION(com_error(E_POINTER));

        *pdwEffect = determine_drop_effect(m_data_object, *pdwEffect);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

/**
 * End the drag-and-drop loop for the current DataObject.
 */
STDMETHODIMP CDropTarget::DragLeave()
{
    try
    {
        m_data_object = NULL;
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

/**
 * Perform the drop operation by either copying or moving the data
 * in the DataObject to the remote target.
 *
 * @todo  Take account of the key state.
 */
STDMETHODIMP CDropTarget::Drop( 
    IDataObject* pdo, DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect)
{
    try
    {
        try
        {
            if (!pdwEffect)
                BOOST_THROW_EXCEPTION(com_error(E_POINTER));

            // Drop doesn't need to maintain any state and is handed a fresh
            // copy of the IDataObject so we can can immediately cancel the
            // one we were using for the other parts of the drag-drop loop
            m_data_object = NULL;

            *pdwEffect = determine_drop_effect(pdo, *pdwEffect);

            if (pdo && *pdwEffect == DROPEFFECT_COPY)
            {
                copy_data_to_provider(
                    pdo, m_provider, m_remote_directory, m_callback);
            }
        }
        catch (...)
        {
            m_callback->handle_last_exception();
            throw;
        }
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

}} // namespace swish::drop_target
