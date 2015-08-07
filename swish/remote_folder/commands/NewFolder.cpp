/**
    @file

    New remote folder command.

    @if license

    Copyright (C) 2011, 2012, 2013, 2015  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "NewFolder.hpp"

#include "swish/frontend/announce_error.hpp" // announce_last_exception
#include "swish/provider/sftp_filesystem_item.hpp"
#include "swish/remote_folder/swish_pidl.hpp" // absolute_path_from_swish_pidl
#include "swish/shell_folder/SftpDirectory.h" // CSftpDirectory
#include "swish/shell_folder/shell.hpp" // put_view_item_into_rename_mode

#include <washer/shell/services.hpp> // shell_browser, shell_view
#include <washer/trace.hpp> // trace

#include <comet/error.h> // com_error
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/lexical_cast.hpp> // lexical_cast
#include <boost/locale.hpp> // translate
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/format.hpp> // wformat
#include <boost/regex.hpp> // wregex, smatch, regex_match
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <exception>
#include <string>
#include <vector>

using swish::frontend::announce_last_exception;
using swish::nse::Command;
using swish::provider::sftp_filesystem_item;
using swish::provider::sftp_provider;
using swish::remote_folder::absolute_path_from_swish_pidl;
using swish::shell_folder::put_view_item_into_rename_mode;

using washer::shell::pidl::apidl_t;
using washer::shell::pidl::cpidl_t;
using washer::shell::shell_browser;
using washer::shell::shell_view;
using washer::trace;

using comet::com_error;
using comet::com_error_from_interface;
using comet::com_ptr;
using comet::enum_iterator;
using comet::uuid_t;

using boost::filesystem::path;
using boost::function;
using boost::shared_ptr;
using boost::lexical_cast;
using boost::locale::translate;
using boost::regex_match;
using boost::wformat;
using boost::wregex;
using boost::wsmatch;

using std::exception;
using std::vector;
using std::wstring;

namespace {

/**
 * Find the first non-existent directory name that begins with @a initial_name.
 *
 * This may simply be initial_name, however, if an item of this name already
 * exists in the directory, return a name that begins with @a initial_name
 * followed by a space and a digit in brackets.  The digit should be the lowest
 * digit that will create a name that doesn't already exits.
 *
 * @todo  Investigate whether other locales require something other than an
 *        Arabic digit being appended or if it should be appended in a different
 *        place.
 */
wstring prefix_if_necessary(
    const wstring& initial_name, shared_ptr<sftp_provider> provider,
    const path& directory)
{
    wregex new_folder_pattern(
        str(wformat(L"%1%|%1% \\((\\d+)\\)") % initial_name));
    wsmatch digit_suffix_match;

    bool collision = false;
    vector<unsigned long> suffixes;
    BOOST_FOREACH(
        const sftp_filesystem_item& lt, provider->listing(directory))
    {
        wstring filename = lt.filename().wstring();
        if (regex_match(filename, digit_suffix_match, new_folder_pattern))
        {
            assert(digit_suffix_match.size() == 2); // complete + capture group
            assert(digit_suffix_match[0].matched); // why are we here otherwise?

            // We record whether an exact match was found with "initial_name"
            // but keep going regardless: if it was, we will need to find
            // the next available digit suffix; if not, it might be found on
            // a future iteration so we still need the next available digit
            if (digit_suffix_match[1].matched)
            {
                unsigned long suffix = lexical_cast<unsigned long>(
                    wstring(digit_suffix_match[1]));
                suffixes.push_back(suffix);
            }
            else if (digit_suffix_match[0].matched)
            {
                collision = true;
            }
            else
            {
                assert(false && "Invariant violation: should never reach here");
                collision = false;
                break;
            }
        }
    }

    if (!collision)
        return initial_name;
    else
    {
        unsigned long lowest = 2;
        std::sort(suffixes.begin(), suffixes.end());
        BOOST_FOREACH(unsigned long suffix, suffixes)
        {
            if (suffix == 1) // skip "New Folder (1)" as Windows doesn't use it
                continue;    // so we won't either
            if (suffix == lowest)
                ++lowest;
            if (suffix > lowest)
                break;
        }

        return str(wformat(L"%1% (%2%)") % initial_name % lowest);
    }
}

}

namespace swish {
namespace remote_folder {
namespace commands {

namespace {
    const uuid_t NEW_FOLDER_COMMAND_ID(L"b816a882-5022-11dc-9153-0090f5284f85");
}

NewFolder::NewFolder(
    const apidl_t& folder_pidl,
    const provider_factory& provider,
    const consumer_factory& consumer) :
    Command(
        translate(L"New &folder"), NEW_FOLDER_COMMAND_ID,
        translate(L"Create a new, empty folder in the folder you have open."),
        L"shell32.dll,-258", L"",
        translate(L"Make a new folder")),
    m_folder_pidl(folder_pidl), m_provider_factory(provider),
    m_consumer_factory(consumer) {}

BOOST_SCOPED_ENUM(Command::state) NewFolder::state(
    const com_ptr<IDataObject>& /*data_object*/, bool /*ok_to_be_slow*/)
const
{
    return state::enabled;
}

void NewFolder::operator()(
    const com_ptr<IDataObject>&, const com_ptr<IBindCtx>&)
const
{
    HWND hwnd = NULL;
    try
    {
        // Get the view which we need to report errors and to put new folder
        // into edit mode.  Failure to get the view is not enough reason to
        // abort the operation so we swallow any errors
        com_ptr<IShellView> view;
        try
        {
            view = shell_view(shell_browser(m_site));
            HRESULT hr = view->GetWindow(&hwnd);
            if (FAILED(hr))
                BOOST_THROW_EXCEPTION(com_error_from_interface(view, hr));
        }
        catch (const exception&)
        {
            trace("WARNING: couldn't get current IShellView or HWND");
        }


        shared_ptr<sftp_provider> provider = m_provider_factory(
            m_consumer_factory(),
            translate("Name of a running task", "Creating new folder"));

        CSftpDirectory directory(m_folder_pidl, provider);

        // The default New Folder name may already exist in the folder. If it
        // does, we append a number to it to make it unique
        wstring initial_name = translate(L"Initial name", L"New folder");
        initial_name = prefix_if_necessary(
            initial_name, provider,
            absolute_path_from_swish_pidl(m_folder_pidl));

        cpidl_t pidl = directory.CreateDirectory(initial_name);

        try
        {
            // A failure after this point is not worth reporting.  The folder
            // was created even if we didn't allow the user a chance to pick a
            // name.
            put_view_item_into_rename_mode(view, pidl);
        }
        catch (const exception& e)
        {
            trace("WARNING: Couldn't put folder into rename mode: %s") %
                e.what();
        }
    }
    catch (...)
    {
        announce_last_exception(
            hwnd, translate(L"Could not create a new folder"),
            translate(L"You might not have permission."));
        throw;
    }
}

void NewFolder::set_site(com_ptr<IUnknown> ole_site)
{
    m_site = ole_site;
}

}}} // namespace swish::remote_folder::commands
