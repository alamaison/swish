/**
    @file

    Swish remote folder commands.

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

#include "commands.hpp"

#include "swish/nse/explorer_command.hpp" // CExplorerCommand*
#include "swish/nse/task_pane.hpp" // CUIElementErrorAdapter, CUICommandWithSite
#include "swish/remote_folder/commands/NewFolder.hpp"

#include <comet/server.h> // simple_object
#include <comet/smart_enum.h> // make_smart_enumeration

#include <boost/bind.hpp> // bind, _1
#include <boost/locale.hpp> // translate
#include <boost/make_shared.hpp> // make_shared
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <exception>
#include <string>
#include <utility> // make_pair
#include <vector>

using swish::nse::CExplorerCommandProvider;
using swish::nse::CExplorerCommandWithSite;
using swish::nse::CUIElementErrorAdapter;
using swish::nse::CUICommandWithSite;
using swish::nse::IEnumUICommand;
using swish::nse::IUICommand;
using swish::nse::IUIElement;
using swish::nse::WebtaskCommandTitleAdapter;
using swish::provider::sftp_provider;

using winapi::shell::pidl::apidl_t;

using comet::com_ptr;
using comet::make_smart_enumeration;
using comet::simple_object;

using boost::bind;
using boost::function;
using boost::locale::translate;
using boost::make_shared;
using boost::shared_ptr;

using std::make_pair;
using std::vector;
using std::wstring;

namespace swish {
namespace remote_folder {
namespace commands {

com_ptr<IExplorerCommandProvider> remote_folder_command_provider(
    HWND /*hwnd*/, const apidl_t& folder_pidl,
    const provider_factory& provider,
    const consumer_factory& consumer)
{
    CExplorerCommandProvider::ordered_commands commands;
    commands.push_back(
        new CExplorerCommandWithSite<NewFolder>(
            folder_pidl, provider, consumer));
    return new CExplorerCommandProvider(commands);
}

class CSftpTasksTitle : public simple_object<CUIElementErrorAdapter>
{
public:

    virtual std::wstring title(
        const comet::com_ptr<IShellItemArray>& /*items*/) const
    {
        return translate(L"File and Folder Tasks");
    }

    virtual std::wstring icon(
        const comet::com_ptr<IShellItemArray>& /*items*/) const
    {
        return L"shell32.dll,-319";
    }

    virtual std::wstring tool_tip(
        const comet::com_ptr<IShellItemArray>& /*items*/) const
    {
        return translate(L"These tasks help you manage your remote files.");
    }
};

std::pair<com_ptr<IUIElement>, com_ptr<IUIElement> >
remote_folder_task_pane_titles(HWND /*hwnd*/, const apidl_t& /*folder_pidl*/)
{
    return make_pair(new CSftpTasksTitle(), com_ptr<IUIElement>());
}

std::pair<com_ptr<IEnumUICommand>, com_ptr<IEnumUICommand> >
remote_folder_task_pane_tasks(
    HWND /*hwnd*/, const apidl_t& folder_pidl,
    com_ptr<IUnknown> ole_site,
    const provider_factory& provider,
    const consumer_factory& consumer)
{
    typedef shared_ptr< vector< com_ptr<IUICommand> > > shared_command_vector;
    shared_command_vector commands =
        make_shared< vector< com_ptr<IUICommand> > >();

    com_ptr<IUICommand> new_folder =
        new CUICommandWithSite< WebtaskCommandTitleAdapter<NewFolder> >(
        folder_pidl,
        bind(
            provider, _1, 
            translate("Name of a running task", "Creating new folder")),
        consumer);

    com_ptr<IObjectWithSite> object_with_site = com_cast(new_folder);

    // Explorer doesn't seem to call SetSite on the command object which is odd
    // because any command that needs to change the view would need it.  We do
    // it instead.
    // XXX: We never unset the site.  Explorer normally does if it sets it.
    //      I don't know if this is a problem.
    if (object_with_site)
        object_with_site->SetSite(ole_site.get());

    commands->push_back(new_folder);

    com_ptr<IEnumUICommand> e = 
        make_smart_enumeration<IEnumUICommand>(commands);

    return make_pair(e, com_ptr<IEnumUICommand>());
}

}}} // namespace swish::remote_folder::commands
