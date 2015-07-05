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

#ifndef SWISH_FRONTEND_BIND_BEST_TASKDIALOG
#define SWISH_FRONTEND_BIND_BEST_TASKDIALOG
#pragma once

#include <washer/gui/task_dialog.hpp> // tdi_function, tdi_implementation

namespace swish {
namespace frontend {

washer::gui::task_dialog::tdi_function bind_best_taskdialog();

class best_taskdialog : public washer::gui::task_dialog::tdi_implementation
{
public:
    best_taskdialog()
        : washer::gui::task_dialog::tdi_implementation(bind_best_taskdialog())
    {}
};

}} // namespace swish::frontend

#endif
