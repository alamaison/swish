/**
    @file

    Plan copying items in PIDL clipboard format to remote server.

    @if license

    Copyright (C) 2012, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "PidlCopyPlan.hpp"

#include "swish/drop_target/CopyFileOperation.hpp"
#include "swish/drop_target/CreateDirectoryOperation.hpp"
#include "swish/drop_target/RootedSource.hpp"
#include "swish/provider/sftp_provider.hpp" // sftp_provider, ISftpConsumer

#include <boost/bind.hpp> // bind
#include <boost/filesystem/path.hpp> // wpath
#include <boost/function_output_iterator.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <winapi/shell/shell.hpp> // stream_from_pidl, bind_to_handler_object
#include <winapi/shell/shell_item.hpp> // pidl_shell_item

#include <comet/error.h> // com_error

#include <string>

using swish::provider::sftp_provider;
using swish::shell_folder::data_object::PidlFormat;

using winapi::shell::bind_to_handler_object;
using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;
using winapi::shell::pidl::pidl_t;
using winapi::shell::pidl_shell_item;
using winapi::shell::stream_from_pidl;

using comet::com_error;
using comet::com_ptr;

using boost::bind;
using boost::filesystem::wpath;
using boost::make_function_output_iterator;
using boost::shared_ptr;
using boost::ref;

using std::wstring;

namespace swish {
namespace drop_target {

namespace {

    /**
     * Return the name the copy should have at the target location.
     */
    wpath target_name_from_source(const RootedSource& source)
    {
        return pidl_shell_item(source.pidl()).friendly_name(
            pidl_shell_item::friendly_name_type::relative);
    }

    template<typename OutIt>
    void output_operations_for_stream_pidl(
        const RootedSource& source, const SftpDestination& destination,
        OutIt output_iterator)
    {
        wpath new_name = target_name_from_source(source);

        SftpDestination new_destination = destination / new_name;

        CopyFileOperation operation(source, new_destination);

        *output_iterator++ = operation;
    }

    template<typename OutIt>
    void output_operations_for_folder_pidl(
        com_ptr<IShellFolder> folder, const RootedSource& source,
        const SftpDestination& destination, OutIt output_iterator)
    {
        wpath new_name = target_name_from_source(source);

        SftpDestination new_destination = destination / new_name;

        *output_iterator++ = CreateDirectoryOperation(source, new_destination);

        com_ptr<IEnumIDList> e;
        HRESULT hr = folder->EnumObjects(
            NULL,
            SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN,
            e.out());
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error_from_interface(folder, hr));

        cpidl_t item;
        while (hr == S_OK && e->Next(1, item.out(), NULL) == S_OK)
        {
            output_operations_for_pidl(
                source / item, new_destination, output_iterator);
        }
    }

    template<typename OutIt>
    void output_operations_for_pidl(
        const RootedSource& source, const SftpDestination& destination,
        OutIt output_iterator)
    {
        try
        {
            /*
            Test if streamable.
            We don't use this stream to perform the operation as that would
            mean large transfers keeping open a large number of file handles
            while building the copy plan - a bad idea, especially if the files
            are on another remote server
            */
            stream_from_pidl(source.pidl());

            output_operations_for_stream_pidl(
                source, destination, output_iterator);
        }
        catch (const com_error&)
        {
            // Treating the item as something with an IStream has failed
            // Now we try to treat it as an IShellFolder and hope we
            // have more success

            com_ptr<IShellFolder> folder =
                bind_to_handler_object<IShellFolder>(source.pidl());

            output_operations_for_folder_pidl(
                folder, source, destination, output_iterator);
        }
    }

}

/**
 * Create plan to copy items represented by clipboard PIDL format.
 *
 * Expands the top-level PIDLs into a list of all items in the hierarchy.
 */
PidlCopyPlan::PidlCopyPlan(
    const PidlFormat& source_format, const apidl_t& destination_root)
{
    for (unsigned int i = 0; i < source_format.pidl_count(); ++i)
    {
        apidl_t pidl = source_format.file(i);

        output_operations_for_pidl(
            RootedSource(
                source_format.parent_folder(), source_format.relative_file(i)),
            SftpDestination(destination_root, wpath()),
            make_function_output_iterator(
                bind(&SequentialPlan::add_stage, ref(m_plan), _1)));
    }
}

void PidlCopyPlan::execute_plan(
    DropActionCallback& callback, shared_ptr<sftp_provider> provider) const
{
    m_plan.execute_plan(callback, provider);
}

void PidlCopyPlan::add_stage(const Operation& entry)
{
    m_plan.add_stage(entry);
}

}}
