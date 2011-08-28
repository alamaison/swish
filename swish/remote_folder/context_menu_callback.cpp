/**
    @file

    RemoteFolder context menu implementation (basically that what it does).

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

#include "swish/frontend/announce_error.hpp" // rethrow_and_announce
#include "swish/shell_folder/data_object/ShellDataObject.hpp" // PidlFormat
#include "swish/shell_folder/SftpDirectory.h" // CSftpDirectory
#include "swish/remote_folder/commands/delete.hpp" // Delete
#include "swish/remote_folder/connection.hpp" // connection_from_pidl
#include "swish/remote_folder/context_menu_callback.hpp"
                                                       // context_menu_callback

#include <winapi/error.hpp> // last_error
#include <winapi/shell/pidl.hpp> // apidl_t, cpidl_t

#include <boost/locale.hpp> // translate
#include <boost/exception/errinfo_file_name.hpp> // errinfo_file_name
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <ShellApi.h> // ShellExecuteEx
#include <Windows.h> // InsertMenu, SetMenuDefaultItem

using swish::frontend::rethrow_and_announce;
using swish::shell_folder::data_object::PidlFormat;

using winapi::last_error;
using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;
using winapi::shell::pidl::pidl_cast;

using comet::com_error;
using comet::com_ptr;

using boost::enable_error_info;
using boost::errinfo_api_function;
using boost::function;
using boost::locale::translate;

using std::string;
using std::wstring;

namespace swish {
namespace remote_folder {

namespace {

	bool is_single_link(com_ptr<IDataObject> data_object)
	{
		PidlFormat format(data_object);

		if (format.pidl_count() != 1)
		{
			return false;
		}
		else
		{
			for (UINT i = 0; i < format.pidl_count(); ++i)
			{
				if (!remote_itemid_view(format.relative_file(i)).is_link())
					return false;
			}

			return true;
		}
	}

	const UINT MENU_OFFSET_OPEN = 0;

}

context_menu_callback::context_menu_callback(
	function<com_ptr<ISftpProvider>(HWND)> provider_factory,
	function<com_ptr<ISftpConsumer>(HWND)> consumer_factory)
	: m_provider_factory(provider_factory),
	m_consumer_factory(consumer_factory) {}

/**
 * @todo  Take account of allowed changes flags.
 */
bool context_menu_callback::merge_context_menu(
	HWND hwnd_view, com_ptr<IDataObject> selection, HMENU hmenu,
	UINT first_item_index, UINT& minimum_id, UINT maximum_id,
	UINT allowed_changes_flags)
{
	if (is_single_link(selection))
	{
		BOOL success = ::InsertMenuW(
			hmenu, first_item_index, MF_BYPOSITION, 
			minimum_id + MENU_OFFSET_OPEN,
			wstring(translate("Open &link")).c_str());
		if (!success)
			BOOST_THROW_EXCEPTION(
				enable_error_info(last_error()) << 
				errinfo_api_function("InsertMenuW"));

		// It's not worth aborting menu creation just because we can't
		// set default so ignore return val
		::SetMenuDefaultItem(hmenu, minimum_id + MENU_OFFSET_OPEN, FALSE);

		++minimum_id;

		// Return false so that Explorer won't add its own 'open'
		// and 'explore' menu items.
		// TODO: Find out what else we lose.
		return false;
	}
	else
	{
		return true; // Let Explorer provide standard verbs
	}
}

void context_menu_callback::verb(
	HWND hwnd_view, com_ptr<IDataObject> selection, UINT command_id_offset,
	wstring& verb_out)
{
	if (command_id_offset != MENU_OFFSET_OPEN)
		BOOST_THROW_EXCEPTION(
			com_error("Unrecognised menu command ID", E_FAIL));

	verb_out = L"open";
}

void context_menu_callback::verb(
	HWND hwnd_view, com_ptr<IDataObject> selection, UINT command_id_offset,
	string& verb_out)
{
	if (command_id_offset != MENU_OFFSET_OPEN)
		BOOST_THROW_EXCEPTION(
			com_error("Unrecognised menu command ID", E_FAIL));

	verb_out = "open";
}

namespace {

	bool do_invoke_command(
		function<com_ptr<ISftpProvider>(HWND)> provider_factory,
		function<com_ptr<ISftpConsumer>(HWND)> consumer_factory,
		HWND hwnd_view, com_ptr<IDataObject> selection, UINT item_offset,
		const wstring& /*arguments*/, int window_mode)
	{
		if (item_offset == DFM_CMD_DELETE)
		{
			commands::Delete deletion_command(
				provider_factory, consumer_factory);
			deletion_command(hwnd_view, selection);
			return true;
		}
		else if (item_offset == MENU_OFFSET_OPEN && is_single_link(selection))
		{
			try
			{
				PidlFormat format(selection);

				// Create SFTP Consumer for this HWNDs lifetime
				com_ptr<ISftpConsumer> consumer = consumer_factory(hwnd_view);

				com_ptr<ISftpProvider> provider = connection_from_pidl(
					format.parent_folder(), hwnd_view);
				CSftpDirectory directory(
					format.parent_folder(), provider, consumer);

				apidl_t target = directory.ResolveLink(
					pidl_cast<cpidl_t>(format.relative_file(0)));

				SHELLEXECUTEINFO sei = SHELLEXECUTEINFO();
				sei.cbSize = sizeof(SHELLEXECUTEINFO);
				sei.fMask = SEE_MASK_IDLIST;
				sei.hwnd = hwnd_view;
				sei.nShow = window_mode;
				sei.lpIDList =
					const_cast<void*>(
						reinterpret_cast<const void*>(target.get()));
				sei.lpVerb = L"open";
				if (!::ShellExecuteEx(&sei))
					BOOST_THROW_EXCEPTION(
						enable_error_info(last_error()) << 
						errinfo_api_function("ShellExecuteEx"));
			}
			catch (...)
			{
				rethrow_and_announce(
					hwnd_view, translate("Unable to open the link"),
					translate("You might not have permission."));
			}

			return true; // Even if the above fails, we don't want to invoke
			// any default action provided by Explorer
		}
		else
		{
			return false;
		}
	}

}

bool context_menu_callback::invoke_command(
	HWND hwnd_view, com_ptr<IDataObject> selection, UINT item_offset,
	const wstring& arguments)
{
	return do_invoke_command(
		m_provider_factory, m_consumer_factory, hwnd_view, selection,
		item_offset, arguments, SW_NORMAL);
}

/**
 * @todo  Take account of the behaviour flags.
 */
bool context_menu_callback::invoke_command(
	HWND hwnd_view, com_ptr<IDataObject> selection, UINT item_offset,
	const wstring& arguments, DWORD /*behaviour_flags*/, UINT /*minimum_id*/,
	UINT /*maximum_id*/, const CMINVOKECOMMANDINFO& invocation_details,
	com_ptr<IUnknown> /*context_menu_site*/)
{
	return do_invoke_command(
		m_provider_factory, m_consumer_factory, hwnd_view, selection,
		item_offset, arguments, invocation_details.nShow);
}

bool context_menu_callback::default_menu_item(
	HWND /*hwnd_view*/, com_ptr<IDataObject> selection,
	UINT& default_command_id)
{
	if (is_single_link(selection))
	{
		default_command_id = MENU_OFFSET_OPEN;
		return true;
	}
	else
	{
		return false;
	}
}

}} // namespace swish::remote_folder
