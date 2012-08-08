/**
    @file

    Expose the remote filesystem as an IDropTarget.

    @if license

    Copyright (C) 2009, 2010, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include "swish/provider/SftpProvider.h" // sftp_provider, ISftpConsumer
#include "swish/shell_folder/data_object/ShellDataObject.hpp"
                                                  // PidlFormat, ShellDataObject

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <boost/shared_ptr.hpp>  // shared_ptr
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <comet/error.h> // com_error
#include <comet/ptr.h>  // com_ptr

#include <memory> // auto_ptr

using swish::shell_folder::data_object::ShellDataObject;
using swish::shell_folder::data_object::PidlFormat;
using swish::provider::sftp_provider;

using winapi::shell::pidl::pidl_t;
using winapi::shell::pidl::apidl_t;

using boost::shared_ptr;

using comet::com_error;
using comet::com_ptr;

using std::auto_ptr;

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
    com_ptr<ISftpConsumer> consumer, const apidl_t& destination_root,
    DropActionCallback& callback)
{
    PidlCopyPlan copy_list(source_format, destination_root);

    copy_list.execute_plan(callback, provider, consumer);
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
    com_ptr<ISftpConsumer> consumer, const apidl_t& remote_directory,
    DropActionCallback& callback)
{
    ShellDataObject data(data_object.get());
    if (data.has_pidl_format())
    {
        copy_format_to_provider(
            PidlFormat(data_object), provider, consumer, remote_directory,
            callback);
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
    shared_ptr<sftp_provider> provider,
    com_ptr<ISftpConsumer> consumer, const apidl_t& remote_directory,
    shared_ptr<DropActionCallback> callback)
    :
    m_provider(provider), m_consumer(consumer),
    m_remote_directory(remote_directory), m_callback(callback) {}

void CDropTarget::on_set_site(com_ptr<IUnknown> ole_site)
{
    m_callback->site(ole_site);
}

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
        if (!pdwEffect)
            BOOST_THROW_EXCEPTION(com_error(E_POINTER));

        // Drop doesn't need to maintain any state and is handed a fresh copy
        // of the IDataObject so we can can immediately cancel the one we were
        // using for the other parts of the drag-drop loop
        m_data_object = NULL;

        *pdwEffect = determine_drop_effect(pdo, *pdwEffect);

        if (pdo && *pdwEffect == DROPEFFECT_COPY)
            copy_data_to_provider(
                pdo, m_provider, m_consumer, m_remote_directory, *m_callback);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

}} // namespace swish::drop_target
