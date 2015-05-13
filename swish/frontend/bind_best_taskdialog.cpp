/**
    @file

    TaskDialogIndirect implementation selector.

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

#include "bind_best_taskdialog.hpp"

#include "swish/atl.hpp" // required by TaskDialog.h

#include <washer/dynamic_link.hpp> // load_function

#include <TaskDialog.h> // Task98DialogIndirect

#include <exception>

using washer::gui::task_dialog::tdi_function;
using washer::load_function;

using std::exception;

namespace swish {
namespace frontend {

tdi_function bind_best_taskdialog()
{
    try
    {
        return load_function<
            HRESULT WINAPI (const TASKDIALOGCONFIG*, int*, int*, BOOL*)>(
                "comctl32.dll", "TaskDialogIndirect");
    }
    catch (const exception&)
    {
        return ::Task98DialogIndirect;
    }
}

}} // namespace swish::frontend
