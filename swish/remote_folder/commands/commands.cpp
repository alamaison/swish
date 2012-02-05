/**
    @file

    Swish remote folder commands.

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

#include "commands.hpp"

#include "swish/frontend/announce_error.hpp" // rethrow_and_announce
#include "swish/frontend/UserInteraction.hpp" // CUserInteraction
#include "swish/nse/explorer_command.hpp" // CExplorerCommand*
#include "swish/nse/task_pane.hpp" // CUIElementErrorAdapter, CUICommandWithSite
#include "swish/shell_folder/SftpDirectory.h" // CSftpDirectory

#include <winapi/shell/services.hpp> // shell_browser, shell_view
#include <winapi/trace.hpp> // trace

#include <comet/datetime.h> // datetime_t
#include <comet/error.h> // com_error
#include <comet/server.h> // simple_object
#include <comet/smart_enum.h> // make_smart_enumeration
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/lexical_cast.hpp> // lexical_cast
#include <boost/locale.hpp> // translate
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/format.hpp> // wformat
#include <boost/make_shared.hpp> // make_shared
#include <boost/regex.hpp> // wregex, smatch, regex_match
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <exception>
#include <string>
#include <utility> // make_pair
#include <vector>

using swish::frontend::rethrow_and_announce;
using swish::frontend::CUserInteraction;
using swish::nse::Command;
using swish::nse::CExplorerCommandProvider;
using swish::nse::CExplorerCommandWithSite;
using swish::nse::CUIElementErrorAdapter;
using swish::nse::CUICommandWithSite;
using swish::nse::IEnumUICommand;
using swish::nse::IUICommand;
using swish::nse::IUIElement;
using swish::nse::WebtaskCommandTitleAdapter;
using swish::SmartListing;

using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;
using winapi::shell::shell_browser;
using winapi::shell::shell_view;
using winapi::trace;

using comet::com_error;
using comet::com_error_from_interface;
using comet::com_ptr;
using comet::datetime_t;
using comet::enum_iterator;
using comet::make_smart_enumeration;
using comet::simple_object;
using comet::uuid_t;

using boost::function;
using boost::lexical_cast;
using boost::locale::translate;
using boost::make_shared;
using boost::regex_match;
using boost::shared_ptr;
using boost::wformat;
using boost::wregex;
using boost::wsmatch;

using std::exception;
using std::make_pair;
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
	const wstring& initial_name, const CSftpDirectory& directory)
{
	wregex new_folder_pattern(
		str(wformat(L"%1%|%1% \\((\\d+)\\)") % initial_name));
	wsmatch digit_suffix_match;

	bool collision = false;
	vector<unsigned long> suffixes;
	enum_iterator<IEnumListing, SmartListing> it = directory.begin();
	for (; it != directory.end(); ++it)
	{
		wstring filename = wstring(it->get().bstrFilename);
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
	
	/**
     * Cause Explorer to refresh any windows displaying the owning folder.
	 *
	 * Inform shell that something in our folder changed (we don't know
	 * exactly what the new PIDL is until we reload from the registry, hence
	 * UPDATEDIR).
	 *
	 * We wait for the event to flush because setting the edit text afterwards
	 * depends on this.
	 */
	void notify_shell(const apidl_t folder_pidl)
	{
		assert(folder_pidl);
		::SHChangeNotify(
			SHCNE_MKDIR, SHCNF_IDLIST | SHCNF_FLUSH, folder_pidl.get(), NULL);
	}
}

NewFolder::NewFolder(
	const apidl_t& folder_pidl,
	const function<com_ptr<ISftpProvider>()>& provider,
	const function<com_ptr<ISftpConsumer>()>& consumer) :
	Command(
		translate("New &folder"), NEW_FOLDER_COMMAND_ID,
		translate("Create a new, empty folder in the folder you have open."),
		L"shell32.dll,-258", L"",
		translate("Make a new folder")),
	m_folder_pidl(folder_pidl), m_provider(provider), m_consumer(consumer) {}

bool NewFolder::disabled(
	const com_ptr<IDataObject>& /*data_object*/, bool /*ok_to_be_slow*/)
const
{ return false; }

bool NewFolder::hidden(
	const com_ptr<IDataObject>& /*data_object*/, bool /*ok_to_be_slow*/)
const
{ return false; }

void NewFolder::operator()(const com_ptr<IDataObject>&, const com_ptr<IBindCtx>&)
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

		CSftpDirectory directory(m_folder_pidl, m_provider(), m_consumer());

		// The default New Folder name may already exist in the folder. If it
		// does, we append a number to it to make it unique
		wstring initial_name = translate("Initial name", "New folder");
		initial_name = prefix_if_necessary(initial_name, directory);

		cpidl_t pidl = directory.CreateDirectory(initial_name);

		try
		{
			// A failure after this point is not worth reporting.  The folder
			// was created even if we didn't update the shell or allow the
			// user a chance to pick a name.

			notify_shell(m_folder_pidl + pidl);

			// Put item into 'rename' mode
			HRESULT hr = view->SelectItem(
				pidl.get(),
				SVSI_EDIT | SVSI_SELECT | SVSI_DESELECTOTHERS |
				SVSI_ENSUREVISIBLE | SVSI_FOCUSED);
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(com_error_from_interface(view, hr));
		}
		catch (const exception&)
		{
			trace("WARNING: Couldn't update shell with new folder");
		}
	}
	catch (...)
	{
		rethrow_and_announce(
			hwnd, translate("Could not create a new folder"),
			translate("You might not have permission."));
	}
}

void NewFolder::set_site(com_ptr<IUnknown> ole_site)
{
	m_site = ole_site;
}

com_ptr<IExplorerCommandProvider> remote_folder_command_provider(
	HWND /*hwnd*/, const apidl_t& folder_pidl,
	const function<com_ptr<ISftpProvider>()>& provider,
	const function<com_ptr<ISftpConsumer>()>& consumer)
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
		return translate("File and Folder Tasks");
	}

	virtual std::wstring icon(
		const comet::com_ptr<IShellItemArray>& /*items*/) const
	{
		return L"shell32.dll,-319";
	}

	virtual std::wstring tool_tip(
		const comet::com_ptr<IShellItemArray>& /*items*/) const
	{
		return translate("These tasks help you manage your remote files.");
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
	const function<com_ptr<ISftpProvider>()>& provider,
	const function<com_ptr<ISftpConsumer>()>& consumer)
{
	typedef shared_ptr< vector< com_ptr<IUICommand> > > shared_command_vector;
	shared_command_vector commands =
		make_shared< vector< com_ptr<IUICommand> > >();

	com_ptr<IUICommand> new_folder =
		new CUICommandWithSite< WebtaskCommandTitleAdapter<NewFolder> >(
				folder_pidl, provider, consumer);

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
