/**
    @file

    User interaction during a drop.

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

#ifndef SWISH_DROP_TARGET_DROPACTIONCALLBACK_HPP
#define SWISH_DROP_TARGET_DROPACTIONCALLBACK_HPP
#pragma once

#include <boost/filesystem.hpp> // wpath

#include <memory> // auto_ptr

namespace swish {
namespace drop_target {

class Progress;

/**
 * Interface for drop target to communicate with the user during a drop.
 */
class DropActionCallback
{
public:
    virtual ~DropActionCallback() {}
    virtual bool can_overwrite(const boost::filesystem::wpath& target) = 0;
    virtual std::auto_ptr<Progress> progress() = 0;
    virtual void handle_and_rethrow_last_exception() = 0;
};

}}

#endif