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
#include <boost/cstdint.hpp> // int64_t
#include <boost/filesystem/path.hpp> // wpath
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <winapi/shell/shell.hpp> // stream_from_pidl, strret_to_string,
                                  // bind_to_handler_object

#include <comet/error.h> // com_error

#include <cassert> // assert
#include <functional> // unary_function
#include <utility> // pair, make_pair
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
using boost::int64_t;
using boost::filesystem::wpath;
using boost::numeric_cast;
using boost::shared_ptr;

using std::make_pair;
using std::pair;
using std::size_t;
using std::unary_function;
using std::wstring;

namespace swish {
namespace drop_target {

namespace {

	pair<wpath, int64_t> stat_stream(const com_ptr<IStream>& stream)
	{
		STATSTG statstg;
		HRESULT hr = stream->Stat(&statstg, STATFLAG_DEFAULT);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error_from_interface(stream, hr));

		shared_ptr<OLECHAR> name(statstg.pwcsName, ::CoTaskMemFree);
		return make_pair(name.get(), statstg.cbSize.QuadPart);
	}

	/**
	 * Return the stream name from an IStream.
	 */
	wpath filename_from_stream(const com_ptr<IStream>& stream)
	{
		return stat_stream(stream).first;
	}

	/**
	 * Query an item's parent folder for the item's display name relative
	 * to that folder.
	 */
	wstring display_name_of_item(
		const com_ptr<IShellFolder>& parent_folder, const cpidl_t& pidl)
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

	/**
	 * Return the parsing path name for a PIDL relative the the given parent.
	 */
	wpath parsing_path_from_pidl(const apidl_t& parent, const pidl_t& pidl)
	{
		if (pidl.empty())
			return wpath();

		cpidl_t item;
		item.attach(::ILCloneFirst(pidl.get()));

		return display_name_from_pidl(parent, item) / 
			parsing_path_from_pidl(parent + item, ::ILNext(pidl.get()));
	}

	void build_copy_list_recursively(
		const apidl_t& parent, const pidl_t& folder_pidl,
		SequentialPlan& plan_out)
	{
		wpath folder_path = parsing_path_from_pidl(parent, folder_pidl);

		plan_out.add_stage(
			CreateDirectoryOperation(parent, folder_pidl, folder_path));

		com_ptr<IShellFolder> folder = 
			bind_to_handler_object<IShellFolder>(parent + folder_pidl);

		// Add non-folder contents

		com_ptr<IEnumIDList> e;
		HRESULT hr = folder->EnumObjects(
			NULL, SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, e.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error_from_interface(folder, hr));

		cpidl_t item;
		while (hr == S_OK && e->Next(1, item.out(), NULL) == S_OK)
		{
			pidl_t pidl = folder_pidl + item;
			plan_out.add_stage(
				CopyFileOperation(
					parent, pidl, parsing_path_from_pidl(parent, pidl)));
		}

		// Recursively add folders

		hr = folder->EnumObjects(
			NULL, SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN, e.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error_from_interface(folder, hr));

		while (hr == S_OK && e->Next(1, item.out(), NULL) == S_OK)
		{
			pidl_t pidl = folder_pidl + item;
			build_copy_list_recursively(parent, pidl, plan_out);
		}
	}
}

/**
 * Create plan to copy items represented by clipboard PIDL format.
 *
 * Expands the top-level PIDLs into a list of all items in the hierarchy.
 */
PidlCopyPlan::PidlCopyPlan(const PidlFormat& source_format)
{
	for (unsigned int i = 0; i < source_format.pidl_count(); ++i)
	{
		pidl_t pidl = source_format.relative_file(i);
		try
		{
			// Test if streamable
			com_ptr<IStream> stream = stream_from_pidl(source_format.file(i));

			CopyFileOperation entry(
				source_format.parent_folder(), pidl,
				filename_from_stream(stream));
			m_plan.add_stage(entry);
		}
		catch (const com_error&)
		{
			// Treating the item as something with an IStream has failed
			// Now we try to treat it as an IShellFolder and hope we
			// have more success

			build_copy_list_recursively(
				source_format.parent_folder(), pidl, m_plan);
		}
	}
}

const Operation& PidlCopyPlan::operator[](unsigned int i) const
{
	return m_plan[i];
}

size_t PidlCopyPlan::size() const
{
	return m_plan.size();
}

void PidlCopyPlan::execute_plan(
	const apidl_t& remote_destination_root, Progress& progress,
	com_ptr<ISftpProvider> provider, com_ptr<ISftpConsumer> consumer,
	CopyCallback& callback) const
{
	m_plan.execute_plan(
		remote_destination_root, progress, provider, consumer, callback);
}

void PidlCopyPlan::add_stage(const Operation& entry)
{
	m_plan.add_stage(entry);
}

}}
