/**
    @file

    Reporting exceptions to the user.

    @if license

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

#include "announce_error.hpp"

#include "swish/shell_folder/bind_best_taskdialog.hpp" // bind_best_taskdialog

#include <winapi/gui/task_dialog.hpp> // task_dialog

#include <boost/locale.hpp> // translate

#include <cassert> // assert
#include <exception>
#include <sstream> // wstringstream

using namespace winapi::gui::task_dialog;

using boost::locale::translate;

using std::exception;
using std::wstring;
using std::wstringstream;

namespace swish {
namespace shell_folder {

void announce_error(
	HWND hwnd, const wstring& problem, const wstring& suggested_resolution,
	const wstring& details)
{
	task_dialog<> td(
		hwnd, problem, suggested_resolution, L"Swish", icon_type::error);
	td.extended_text(
		details, expansion_position::below,
		initial_expansion_state::default,
		translate("Show &details (which may not be in your language)"),
		translate("Hide &details"));
	td.show();
}

void rethrow_and_announce(HWND hwnd, const wstring& title)
{
	// Only try and announce if we have an owner window
	if (hwnd == NULL)
		throw; 
	
	try
	{
		throw;
	}
	catch (const std::exception& error)
	{
		try
		{
			wstringstream message;
			message << error.what();
			announce_error(hwnd, title, L"", message.str());
		}
		catch (...)
		{
			// I've tested this catch handler and it works the way I
			// expected: it swallows the newly thrown exception allowing the
			// throw below to rethrow the original exception.
			// XXX: I can't find whether this is guaranteed by the C++
			// standard.  Implementing this behaviour must mean maintaining
			// some form of thrown exception stack.
			assert(!"Exception announcer threw new exception");
		}

		throw;
	}
}

}} // namespace swish::shell_folder
