/**
    @file

    Pageant launch command.

    @if license

    Copyright (C) 2012, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "LaunchAgent.hpp"

#include <winapi/error.hpp> // last_error
#include <winapi/dynamic_link.hpp> // module_path, module_handle

#include <comet/error.h> // com_error
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/exception/info.hpp> // errinfo
#include <boost/exception/errinfo_file_name.hpp> // errinfo_file_name
#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/locale.hpp> // translate
#include <boost/filesystem/path.hpp> // wpath
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <string>

using swish::nse::Command;

using winapi::module_handle;
using winapi::module_path;
using winapi::shell::pidl::apidl_t;

using comet::com_error;
using comet::com_ptr;
using comet::uuid_t;

using boost::locale::translate;
using boost::filesystem::wpath;

using std::wstring;

// http://stackoverflow.com/a/557859/67013
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace swish {
namespace host_folder {
namespace commands {

namespace {
   const uuid_t ADD_COMMAND_ID(L"b816a884-5022-11dc-9153-0090f5284f85");

   const wpath PAGEANT_FILE_NAME = L"pageant.exe";

   wpath pageant_path()
   {
       return module_path<wchar_t>(((HINSTANCE)&__ImageBase)).parent_path()
           / PAGEANT_FILE_NAME;
   }
   
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

LaunchAgent::LaunchAgent(HWND hwnd, const apidl_t& folder_pidl) :
   Command(
      translate(
        L"Title of command used to launch the SSH agent program",
        L"&Launch key agent"), ADD_COMMAND_ID,
      translate(L"Launch Putty SSH key agent, Pageant."),
      L"",
      translate(
        L"Title of command used to launch the SSH agent program",
        L"&Launch key agent"),
      translate(
        L"Title of command used to launch the SSH agent program",
        L"Launch key agent")),
   m_hwnd(hwnd), m_folder_pidl(folder_pidl) {}


BOOST_SCOPED_ENUM(Command::state) LaunchAgent::state(
    const comet::com_ptr<IDataObject>& /*data_object*/, bool /*ok_to_be_slow*/)
const
{
    HWND hwnd = ::FindWindowW(L"Pageant", L"Pageant");

    return (hwnd) ? state::hidden : state::enabled;
}

void LaunchAgent::operator()(const com_ptr<IDataObject>&, const com_ptr<IBindCtx>&)
const
{
    static wstring pageant = pageant_path().file_string();

    STARTUPINFOW si = STARTUPINFOW();
    PROCESS_INFORMATION pi = PROCESS_INFORMATION();
    if (!::CreateProcessW(
        pageant.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(winapi::last_error()) <<
                boost::errinfo_file_name("pageant") <<
                boost::errinfo_api_function("CreateProcess"));

    // Notify the shell because it needs to prod the commands to recalculate
    // their visibility so that we can tell it not to show our button now
    // that Pageant is running.
    notify_shell(m_folder_pidl);
}

}}} // namespace swish::host_folder::commands
