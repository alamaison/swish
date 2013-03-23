/**
    @file

    User-interaction for DropTarget.

    @if license

    Copyright (C) 2010, 2012, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_DROP_TARGET_DROPUI_HPP
#define SWISH_DROP_TARGET_DROPUI_HPP
#pragma once

#include "swish/drop_target/DropActionCallback.hpp"
#include "swish/drop_target/Progress.hpp"

#include <winapi/window/window.hpp>

#include <boost/optional.hpp>

#include <memory> // auto_ptr

namespace swish {
namespace drop_target {

    /**
     * DropTarget callback turning requests into GUI windows so user can
     * handle them.
     */
    class DropUI : public DropActionCallback
    {
    public:
        DropUI(
            const boost::optional< winapi::window::window<wchar_t> >& owner);

        virtual bool can_overwrite(const boost::filesystem::wpath& target);
        virtual std::auto_ptr<Progress> progress();
        virtual void handle_last_exception();
        
    private:
        boost::optional< winapi::window::window<wchar_t> > m_owner;
        Progress* m_progress;
    };

}} // namespace swish::drop_target

#endif
