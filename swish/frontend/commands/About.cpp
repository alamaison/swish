/**
    @file

    Swish About box.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    If you modify this Program, or any covered work, by linking or
    combining it with the OpenSSL project's OpenSSL library (or a
    modified version of that library), containing parts covered by the
    terms of the OpenSSL or SSLeay licenses, the licensors of this
    Program grant you additional permission to convey the resulting work.

    @endif
*/

#include "About.hpp"

#include "swish/versions/version.hpp" // release_version

#include <washer/gui/message_box.hpp>
#include <washer/dynamic_link.hpp> // module_path

#include <comet/uuid_fwd.h> // uuid_t

#include <boost/locale.hpp> // translate
#include <boost/filesystem/path.hpp>

#include <cassert> // assert
#include <sstream> // ostringstream
#include <string>

using swish::nse::Command;

using namespace washer::gui::message_box;
using washer::module_path;
using washer::shell::pidl::apidl_t;

using comet::com_ptr;
using comet::uuid_t;

using boost::locale::translate;
using boost::filesystem::path;

using std::ostringstream;
using std::string;

// http://stackoverflow.com/a/557859/67013
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace swish {
namespace frontend {
namespace commands {

namespace {

   const uuid_t ABOUT_COMMAND_ID(L"b816a885-5022-11dc-9153-0090f5284f85");
   
   path installation_path()
   {
       return module_path<char>(((HINSTANCE)&__ImageBase));
   }
}

About::About(HWND hwnd) :
   Command(
      translate(
        L"Title of command used to show the Swish 'About' box in the "
        L"Explorer Help menu",
        L"About &Swish"), ABOUT_COMMAND_ID,
      translate(
          L"Displays version, licence and copyright information for Swish.")),
   m_hwnd(hwnd) {}

BOOST_SCOPED_ENUM(Command::state) About::state(
   const comet::com_ptr<IDataObject>& /*data_object*/, bool /*ok_to_be_slow*/)
const
{
    return state::enabled;
}

void About::operator()(const com_ptr<IDataObject>&, const com_ptr<IBindCtx>&)
const
{
    string snapshot = snapshot_version();
    if (snapshot.empty())
    {
        snapshot = translate(
            "Placeholder version if actual version is not known",
            "unknown");
    }

    ostringstream message;
    message
        << "Swish " << release_version().as_string() << "\n"
        << translate(
            "A short description of Swish", "Easy SFTP for Windows Explorer")
        << "\n\n"
        << "Copyright (C) 2006-2013  Alexander Lamaison and contributors.\n\n"
        << "This program comes with ABSOLUTELY NO WARRANTY. This is free "
           "software, and you are welcome to redistribute it under the terms "
           "of the GNU General Public License as published by the Free "
           "Software Foundation, either version 3 of the License, or "
           "(at your option) any later version.\n\n"
        << translate("Title of a version description", "Snapshot:")
        << " " <<  snapshot << "\n"
        << translate("Title for a date and time", "Build time:")
        << " " << build_date() << " " << build_time() << "\n"
        << translate("Title of a filesystem path", "Installation path:")
        << " " << installation_path().file_string();

   message_box(
        m_hwnd, message.str(),
        translate("Title of About dialog box", "About Swish"), box_type::ok,
        icon_type::information);
}

}}} // namespace swish::frontend::commands
