/**
    @file

    RemoteFolder context menu implementation (basically that what it does).

    @if license

    Copyright (C) 2011, 2012, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/frontend/announce_error.hpp" // announce_last_exception
#include "swish/shell_folder/data_object/ShellDataObject.hpp" // PidlFormat
#include "swish/shell_folder/SftpDirectory.h" // CSftpDirectory
#include "swish/shell_folder/shell.hpp" // ui_object_of_item
#include "swish/remote_folder/commands/delete.hpp" // Delete
#include "swish/remote_folder/pidl_connection.hpp" // provider_from_pidl
#include "swish/remote_folder/context_menu_callback.hpp"
                                                       // context_menu_callback

#include <washer/error.hpp> // last_error
#include <washer/shell/pidl.hpp> // apidl_t, cpidl_t
#include <washer/shell/shell.hpp> // pidl_from_parsing_name

#include <comet/uuid.h> // uuid_t

#include <boost/locale.hpp> // translate
#include <boost/exception/errinfo_file_name.hpp> // errinfo_file_name
#include <boost/filesystem/path.hpp> // path
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <stdexcept> // runtime_error
#include <vector>

#include <ShellApi.h> // ShellExecuteEx
#include <Windows.h> // InsertMenu, SetMenuDefaultItem

using swish::frontend::announce_last_exception;
using swish::provider::sftp_provider;
using swish::shell_folder::data_object::PidlFormat;
using swish::shell_folder::ui_object_of_item;

using washer::last_error;
using washer::shell::pidl_from_parsing_name;
using washer::shell::pidl::apidl_t;
using washer::shell::pidl::cpidl_t;
using washer::shell::pidl::pidl_cast;

using comet::com_error;
using comet::com_ptr;
using comet::uuid_t;

using boost::enable_error_info;
using boost::errinfo_api_function;
using boost::filesystem::path;
using boost::function;
using boost::locale::translate;
using boost::numeric_cast;
using boost::shared_ptr;

using std::runtime_error;
using std::string;
using std::vector;
using std::wstring;

namespace comet {

template<> struct comtype<IDropTarget>
{
    static const IID& uuid() throw() { return IID_IDropTarget; }
    typedef IUnknown base;
};

} // namespace comet

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

    bool are_normal_files(com_ptr<IDataObject> selection)
    {
        PidlFormat format(selection);

        if (format.pidl_count() < 1)
            return false;

        for (UINT i = 0; i < format.pidl_count(); ++i)
        {
            if (remote_itemid_view(format.relative_file(i)).is_link() ||
                remote_itemid_view(format.relative_file(i)).is_folder())
                return false;
        }

        // FIXME: failure to be a folder or a link does not mean you're a
        // normal file.
        return true;
    }

    const UINT MENU_OFFSET_OPEN = 0;

}

context_menu_callback::context_menu_callback(
    my_provider_factory provider_factory,
    my_consumer_factory consumer_factory)
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
            wstring(translate(L"Open &link")).c_str());
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
    else if (are_normal_files(selection))
    {
        BOOL success = ::InsertMenuW(
            hmenu, first_item_index, MF_BYPOSITION, 
            minimum_id + MENU_OFFSET_OPEN,
            wstring(translate(L"&Open")).c_str());
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

    template<typename ProviderFactory, typename ConsumerFactory>
    bool do_invoke_command(
        ProviderFactory provider_factory,
        ConsumerFactory consumer_factory,
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

                shared_ptr<sftp_provider> provider = provider_from_pidl(
                    format.parent_folder(), consumer,
                    translate("Name of a running task", "Resolving link"));

                CSftpDirectory directory(format.parent_folder(), provider);

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
                announce_last_exception(
                    hwnd_view, translate(L"Unable to open the link"),
                    translate(L"You might not have permission."));
                throw;
            }

            return true; // Even if the above fails, we don't want to invoke
            // any default action provided by Explorer
        }
        // TODO: handle links so that links to files are resolved and the
        // targets are opened
        //
        // FIXME: what if the selection contains a mix of items?
        else if (item_offset == MENU_OFFSET_OPEN && are_normal_files(selection))
        {
            try
            {
                PidlFormat format(selection);

                // XXX: We're only opening the first file even though we copy all 
                // of them.  Is this what we want?

                vector<wchar_t> system_temp_dir(MAX_PATH + 1, L'\0');
                DWORD rc = ::GetTempPathW(
                    numeric_cast<DWORD>(system_temp_dir.size()),
                    &system_temp_dir[0]);
                if (rc < 1)
                    BOOST_THROW_EXCEPTION(last_error());
                
                // We're using drag-and-drop to do the copy, so we don't
                // want collisions to be possible as they will throw up
                // confirmation dialogues.  We create a unique directory to
                // copy the file into by generating a GUID.  If this already
                // exists, the universe may be close to collapse in which case
                // we should probably find our loved ones and stop worrying
                // about file transfers.
                path temp_dir = &system_temp_dir[0];
                temp_dir /= uuid_t::create().w_str();
                if (!create_directory(temp_dir))
                    BOOST_THROW_EXCEPTION(
                        runtime_error(
                            "Temporary download location already exists"));
                
                com_ptr<IDropTarget> drop_target =
                    ui_object_of_item<IDropTarget>(
                        pidl_from_parsing_name(temp_dir.wstring()).get());

                POINTL pt = { 0, 0 };
                DWORD dwEffect = DROPEFFECT_COPY;
                HRESULT hr = drop_target->DragEnter(
                    selection.get(), MK_LBUTTON, pt, &dwEffect);
                if (FAILED(hr))
                    BOOST_THROW_EXCEPTION(
                        com_error_from_interface(drop_target, hr));

                dwEffect &= DROPEFFECT_COPY;
                if (dwEffect)
                {
                    hr = drop_target->Drop(
                        selection.get(), MK_LBUTTON, pt, &dwEffect);
                    if (FAILED(hr))
                        BOOST_THROW_EXCEPTION(
                            com_error_from_interface(drop_target, hr));
                }
                else
                {
                    hr = drop_target->DragLeave();
                    if (FAILED(hr))
                        BOOST_THROW_EXCEPTION(
                            com_error_from_interface(drop_target, hr));
                    BOOST_THROW_EXCEPTION(
                        runtime_error(
                            "Permission refused to copy remote file to "
                            "temporary location"));
                }

                path target = temp_dir;
                target /= remote_itemid_view(format.relative_file(0)).filename();
                wstring target_windows_path = target.wstring();

                // Before opening the file we make it read-only to discourage
                // users from making changes and saving it to the temporary
                // location - they're likely to forget and then lose their data.
                // This should force most apps to invoke Save As.
                ::SetFileAttributes(
                    target_windows_path.c_str(),
                    ::GetFileAttributes(target_windows_path.c_str())
                        | FILE_ATTRIBUTE_READONLY);
                // It isn't worth aborting the operation if this fails so we
                // ignore any error

                SHELLEXECUTEINFO sei = SHELLEXECUTEINFO();
                sei.cbSize = sizeof(SHELLEXECUTEINFO);
                //sei.fMask = SEE_
                sei.hwnd = hwnd_view;
                sei.nShow = window_mode;
                sei.lpFile = target_windows_path.c_str();
                sei.lpVerb = L"open";
                if (!::ShellExecuteEx(&sei))
                    BOOST_THROW_EXCEPTION(
                        enable_error_info(last_error()) << 
                        errinfo_api_function("ShellExecuteEx"));
            }
            catch (...)
            {
                announce_last_exception(
                    hwnd_view, translate(L"Unable to open the file"),
                    translate(L"You might not have permission."));
                throw;
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
