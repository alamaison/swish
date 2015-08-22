/* Copyright (C) 2015  Alexander Lamaison <swish@lammy.co.uk>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation, either version 3 of the License, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "command_site.hpp"

#include "swish/shell/shell.hpp" // window_for_ole_site

#include <cassert>
#include <exception>

using swish::shell::window_for_ole_site;

using washer::window::window;

using comet::com_ptr;

using boost::optional;

namespace swish {
namespace nse {

command_site::command_site() {}

command_site::command_site(com_ptr<IUnknown> ole_site) : m_ole_site(ole_site) {}

command_site::command_site(
    com_ptr<IUnknown> ole_site,
    const optional<window<wchar_t>>& ui_owner_fallback)
    : m_ole_site(ole_site), m_ui_owner_fallback(ui_owner_fallback)
{
    assert("NULL HWND in initialised optional<window>" &&
        (!m_ui_owner_fallback || m_ui_owner_fallback->hwnd()));
}

optional<window<wchar_t>> command_site::ui_owner() const
{
    if (m_ole_site)
    {
        try
        {
            optional<window<wchar_t>> view_window =
                window_for_ole_site(m_ole_site);
            if (view_window)
            {
                return view_window;
            }
            else
            {
                return m_ui_owner_fallback;
            }
        }
        catch (const std::exception&)
        {
            return m_ui_owner_fallback;
        }
    }
    else
    {
        return m_ui_owner_fallback;
    }
}

com_ptr<IUnknown> command_site::ole_site() const
{
    return m_ole_site;
}

}} // namespace swish::nse
