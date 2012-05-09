/**
    @file

    Plan copying items in PIDL clipboard format to remote server.

    @if license

    Copyright (C) 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer

#include <boost/bind.hpp> // bind
#include <boost/filesystem/path.hpp> // wpath
#include <boost/function_output_iterator.hpp>
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <winapi/shell/shell.hpp> // stream_from_pidl, strret_to_string,
                                  // bind_to_handler_object

#include <comet/error.h> // com_error

#include <string>

using swish::shell_folder::data_object::PidlFormat;

using winapi::shell::bind_to_handler_object;
using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;
using winapi::shell::pidl::pidl_t;
using winapi::shell::stream_from_pidl;
using winapi::shell::strret_to_string;

using comet::com_error;
using comet::com_ptr;

using boost::bind;
using boost::filesystem::wpath;
using boost::make_function_output_iterator;
using boost::ref;

using std::wstring;

namespace swish {
namespace drop_target {

namespace {

	/**
	 * Query an item's parent folder for the item's display name relative
	 * to that folder.
	 */
	wstring display_name_of_item(
		com_ptr<IShellFolder> parent_folder, const cpidl_t& pidl)
	{
		STRRET strret;
		HRESULT hr = parent_folder->GetDisplayNameOf(
			pidl.get(), SHGDN_INFOLDER | SHGDN_FORPARSING, &strret);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error_from_interface(parent_folder, hr));

		return strret_to_string<wchar_t>(strret, pidl);
	}

	/**
	 * Return the parsing name of an item.
	 */
	wpath display_name_from_pidl(const apidl_t& parent, const cpidl_t& item)
	{
		com_ptr<IShellFolder> parent_folder = 
			bind_to_handler_object<IShellFolder>(parent);

		return display_name_of_item(parent_folder, item);
	}

	template<typename OutIt>
	void output_operations_for_stream_pidl(
		const apidl_t& source_pidl, const SftpDestination& destination,
		OutIt output_iterator)
	{
		wpath display_name = display_name_from_pidl(
			source_pidl.parent(), source_pidl.last_item());

		CopyFileOperation operation(source_pidl, destination / display_name);

		*output_iterator++ = operation;
	}

	template<typename OutIt>
	void output_operations_for_folder_pidl(
		com_ptr<IShellFolder> folder, const apidl_t& source_pidl,
		const SftpDestination& destination, OutIt output_iterator)
	{
		wpath display_name = display_name_from_pidl(
			source_pidl.parent(), source_pidl.last_item());

		SftpDestination new_destination = destination / display_name;

		*output_iterator++ = CreateDirectoryOperation(new_destination);

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
				source_pidl + item, new_destination, output_iterator);
		}
	}

	template<typename OutIt>
	void output_operations_for_pidl(
		const apidl_t& source_pidl, const SftpDestination& destination,
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
			stream_from_pidl(source_pidl);

			output_operations_for_stream_pidl(
				source_pidl, destination, output_iterator);
		}
		catch (const com_error&)
		{
			// Treating the item as something with an IStream has failed
			// Now we try to treat it as an IShellFolder and hope we
			// have more success

			com_ptr<IShellFolder> folder =
				bind_to_handler_object<IShellFolder>(source_pidl);

			output_operations_for_folder_pidl(
				folder, source_pidl, destination, output_iterator);
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
			pidl, SftpDestination(destination_root, wpath()),
			make_function_output_iterator(
				bind(&SequentialPlan::add_stage, ref(m_plan), _1)));
	}
}

void PidlCopyPlan::execute_plan(
	Progress& progress, com_ptr<ISftpProvider> provider,
	com_ptr<ISftpConsumer> consumer, CopyCallback& callback) const
{
	m_plan.execute_plan(progress, provider, consumer, callback);
}

void PidlCopyPlan::add_stage(const Operation& entry)
{
	m_plan.add_stage(entry);
}

}}
