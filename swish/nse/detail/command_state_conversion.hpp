/**
    @file

    Conversion between command state representations.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_NSE_DETAIL_COMMAND_STATE_CONVERSION_HPP
#define SWISH_NSE_DETAIL_COMMAND_STATE_CONVERSION_HPP

#include "swish/nse/Command.hpp"

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <stdexcept> // logic_error

#include <shobjidl.h> // EXPCMDSTATE

namespace swish
{
namespace nse
{
namespace detail
{

inline EXPCMDSTATE
command_state_to_expcmdstate(Command::presentation_state state_in)
{
    switch (state_in)
    {
    case Command::presentation_state::enabled:
        return ECS_ENABLED;

    case Command::presentation_state::disabled:
        return ECS_DISABLED;

    case Command::presentation_state::hidden:
        // Add disabled flag as well just to be on the safe side.
        // As the command button is hidden it shouldn't matter whether it
        // is disabled
        return ECS_HIDDEN | ECS_DISABLED;

    default:
        BOOST_THROW_EXCEPTION(std::logic_error("Unrecognised command state"));
    }
}
}
}
}

#endif
