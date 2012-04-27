/**
    @file

    Remote item deletion.

    @if license

    Copyright (C) 2011, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/remote_folder/commands/delete.hpp"

#include "swish/frontend/announce_error.hpp" // rethrow_and_announce
#include "swish/shell_folder/data_object/ShellDataObject.hpp" // PidlFormat
#include "swish/shell_folder/SftpDirectory.h" // CSftpDirectory

#include <winapi/shell/pidl.hpp> // apidl_t, cpidl_t, pidl_cast

#include <boost/locale.hpp> // translate
#include <boost/format.hpp> // wformat
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <string>
#include <vector>

#include <Windows.h> // IsolationAwareMessageBox

using swish::frontend::rethrow_and_announce;
using swish::shell_folder::data_object::PidlFormat;

using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;
using winapi::shell::pidl::pidl_cast;

using comet::com_ptr;

using boost::function;
using boost::locale::translate;
using boost::wformat;

using std::vector;
using std::wstring;


namespace swish {
namespace remote_folder {
namespace commands {

namespace {

	/**
	 * Deletes files or folders.
	 *
	 * The list of items to delete is supplied as a list of PIDLs and may
	 * contain a mix of files and folder.
	 */
	void do_delete(
		HWND hwnd_view, const vector<cpidl_t>& death_row,
		function<com_ptr<ISftpProvider>(HWND)> provider_factory,
		function<com_ptr<ISftpConsumer>(HWND)> consumer_factory,
		const apidl_t& parent_folder)
	{
		com_ptr<ISftpProvider> provider = provider_factory(hwnd_view);
		com_ptr<ISftpConsumer> consumer = consumer_factory(hwnd_view);

		// Create instance of our directory handler class
		CSftpDirectory directory(parent_folder, provider, consumer);

		// Delete each item and notify shell
		vector<cpidl_t>::const_iterator it = death_row.begin();
		while (it != death_row.end())
		{
			directory.Delete(*it++);
		}
	}

	/**
	 * Displays dialog seeking confirmation from user to delete a single item.
	 *
	 * The dialog differs depending on whether the item is a file or a folder.
	 *
	 * @param hwnd_view  Handle to the window used for UI.
	 * @param filename   Name of the file or folder being deleted.
	 * @param is_folder  Is the item in question a file or a folder?
	 *
	 * @returns  Whether confirmation was given or denied.
	 */
	bool confirm_deletion(
		HWND hwnd_view, const wstring& filename, bool is_folder)
	{
		if (hwnd_view == NULL)
			return false;

		wstring message;
		if (!is_folder)
		{
			message = L"Are you sure you want to permanently delete '";
			message += filename;
			message += L"'?";
		}
		else
		{
			message = L"Are you sure you want to permanently delete the "
				L"folder '";
			message += filename;
			message += L"' and all of its contents?";
		}

		int ret = ::IsolationAwareMessageBox(
			hwnd_view, message.c_str(),
			(is_folder) ? L"Confirm Folder Delete" : L"Confirm File Delete", 
			MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1);

		return (ret == IDYES);
	}


	/**
	 * Displays dialog seeking confirmation from user to delete multiple items.
	 *
	 * @param hwnd_view   Handle to the window used for UI.
	 * @param item_count  Number of items selected for deletion.
	 *
	 * @returns  Whether confirmation was given or denied.
	 */
	bool confirm_multiple_deletion(HWND hwnd_view, size_t item_count)
	{
		if (hwnd_view == NULL)
			return false;

		wstring message =
			str(wformat(
				L"Are you sure you want to permanently delete these %d items?")
				% item_count);

		int ret = ::IsolationAwareMessageBox(
			hwnd_view, message.c_str(), L"Confirm Multiple Item Delete",
			MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1);

		return (ret == IDYES);
	}

	/**
	 * Deletes files or directories after seeking confirmation from user.
	 *
	 * The list of items to delete is supplied as a list of PIDLs and may
	 * contain a mix of files and folders.
	 *
	 * If just one item is chosen, a specific confirmation message for that
	 * item is shown.  If multiple items are to be deleted, a general
	 * confirmation message is displayed asking if the number of items are
	 * to be deleted.
	 */
	void execute_death_row(
		HWND hwnd_view, const vector<cpidl_t>& death_row,
		function<com_ptr<ISftpProvider>(HWND)> provider_factory,
		function<com_ptr<ISftpConsumer>(HWND)> consumer_factory,
		const apidl_t& parent_folder)
	{
		size_t item_count = death_row.size();

		BOOL go_ahead = false;
		if (item_count == 1)
		{
			remote_itemid_view itemid(death_row[0]);
			go_ahead = confirm_deletion(
				hwnd_view, itemid.filename(), itemid.is_folder());
		}
		else if (item_count > 1)
		{
			go_ahead = confirm_multiple_deletion(hwnd_view, item_count);
		}
		else
		{
			assert(false);
			return; // do nothing because no items were given
		}

		if (go_ahead)
			do_delete(
				hwnd_view, death_row, provider_factory, consumer_factory,
				parent_folder);
	}
}


Delete::Delete(
	function<com_ptr<ISftpProvider>(HWND)> provider_factory,
	function<com_ptr<ISftpConsumer>(HWND)> consumer_factory)
	: m_provider_factory(provider_factory), m_consumer_factory(consumer_factory)
{}

void Delete::operator()(HWND hwnd_view, com_ptr<IDataObject> selection)
const
{
	try
	{
		PidlFormat format(selection);

		// Build up a list of PIDLs for all the items to be deleted
		vector<cpidl_t> death_row;
		for (UINT i = 0; i < format.pidl_count(); i++)
		{
			death_row.push_back(pidl_cast<cpidl_t>(format.relative_file(i)));
		}

		execute_death_row(
			hwnd_view, death_row, m_provider_factory, m_consumer_factory,
			format.parent_folder());
	}
	catch (...)
	{
		rethrow_and_announce(
			hwnd_view, translate("Unable to delete the item"),
			translate("You might not have permission."));
	}
}

}}} // namespace swish::remote_folder::commands
