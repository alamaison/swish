/**
    @file

    Swish host folder commands.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "host.hpp"

#include "swish/forms/add_host.hpp" // add_host
#include "swish/shell_folder/explorer_command.hpp" // CExplorerCommandProvider
#include "swish/shell_folder/data_object/ShellDataObject.hpp" // PidlFormat
#include "swish/shell_folder/host_management.hpp" // AddConnectionToRegistry
#include "swish/shell_folder/HostPidl.h" // CHostItemAbsolute
#include "swish/exception.hpp" // com_exception

#include <comet/uuid_fwd.h> // uuid_t

#include <boost/locale.hpp> // translate
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert>
#include <string>

using swish::forms::add_host;
using swish::forms::host_info;
using swish::shell_folder::explorer_command::CExplorerCommandProvider;
using swish::shell_folder::explorer_command::make_explorer_command;
using swish::shell_folder::data_object::PidlFormat;
using swish::shell_folder::commands::Command;
using swish::host_management::AddConnectionToRegistry;
using swish::host_management::RemoveConnectionFromRegistry;
using swish::host_management::ConnectionExists;
using swish::exception::com_exception;

using winapi::shell::pidl::apidl_t;

using comet::uuid_t;
using comet::com_ptr;
using comet::uuidof;

using boost::locale::translate;

using std::wstring;

namespace swish {
namespace shell_folder {
namespace commands {
namespace host {

namespace {
	const uuid_t ADD_COMMAND_ID(L"b816a880-5022-11dc-9153-0090f5284f85");
	const uuid_t REMOVE_COMMAND_ID("b816a881-5022-11dc-9153-0090f5284f85");

	/**
     * Cause Explorer to refresh any windows displaying the owning folder.
	 *
	 * Inform shell that something in our folder changed (we don't know
	 * exactly what the new PIDL is until we reload from the registry, hence
	 * UPDATEDIR).
	 */
	void notify_shell(const apidl_t folder_pidl)
	{
		assert(folder_pidl);
		::SHChangeNotify(
			SHCNE_UPDATEDIR, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT,
			folder_pidl.get(), NULL);
	}
}

Add::Add(HWND hwnd, const apidl_t& folder_pidl) :
	Command(
		translate("&Add SFTP Connection"), ADD_COMMAND_ID,
		translate("Create a new SFTP connection with Swish."),
		L"shell32.dll,-258"),
	m_hwnd(hwnd), m_folder_pidl(folder_pidl) {}

bool Add::disabled(
	const comet::com_ptr<IDataObject>& /*data_object*/, bool /*ok_to_be_slow*/)
const
{ return false; }

bool Add::hidden(
	const comet::com_ptr<IDataObject>& /*data_object*/, bool /*ok_to_be_slow*/)
const
{ return false; }

/** Display dialog to get connection info from user. */
void Add::operator()(const com_ptr<IDataObject>&, const com_ptr<IBindCtx>&)
const
{
	host_info info = add_host(m_hwnd);

	if (ConnectionExists(info.name))
		BOOST_THROW_EXCEPTION(com_exception(E_FAIL));

	AddConnectionToRegistry(
		info.name, info.host, info.port, info.user, info.path);

	notify_shell(m_folder_pidl);
}

Remove::Remove(HWND hwnd, const apidl_t& folder_pidl) :
	Command(
		translate("&Remove SFTP Connection"), REMOVE_COMMAND_ID,
		translate("Remove a SFTP connection created with Swish."),
		L"shell32.dll,-240"),
	m_hwnd(hwnd), m_folder_pidl(folder_pidl) {}

bool Remove::disabled(
	const comet::com_ptr<IDataObject>& data_object, bool /*ok_to_be_slow*/)
const
{
	PidlFormat format(data_object);
	return format.pidl_count() != 1;
}

bool Remove::hidden(
	const comet::com_ptr<IDataObject>& data_object, bool ok_to_be_slow)
const
{
	return disabled(data_object, ok_to_be_slow);
}

void Remove::operator()(
	const com_ptr<IDataObject>& data_object, const com_ptr<IBindCtx>&)
const
{
	PidlFormat format(data_object);
	// XXX: for the moment we only allow removing one item.
	//      is this what we want?
	if (format.pidl_count() != 1)
		BOOST_THROW_EXCEPTION(com_exception(E_FAIL));

	CHostItemAbsolute pidl_selected = format.file(0).get();
	wstring label = pidl_selected.FindHostPidl().GetLabel();
	assert(!label.empty());
	if (label.empty())
		BOOST_THROW_EXCEPTION(com_exception(E_UNEXPECTED));

	RemoveConnectionFromRegistry(label);
	notify_shell(m_folder_pidl);
}

com_ptr<IExplorerCommandProvider> host_folder_command_provider(
	HWND hwnd, const apidl_t& folder_pidl)
{
	CExplorerCommandProvider::ordered_commands commands;
	commands.push_back(make_explorer_command(Add(hwnd, folder_pidl)));
	commands.push_back(make_explorer_command(Remove(hwnd, folder_pidl)));
	return new CExplorerCommandProvider(commands);
}

}}}} // namespace swish::shell_folder::commands::host
