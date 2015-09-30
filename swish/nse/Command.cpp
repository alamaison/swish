/**
    @file

    Swish host folder commands.

    @if license

    Copyright (C) 2010, 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "Command.hpp"

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

using washer::shell::pidl::apidl_t;

using comet::com_ptr;
using comet::uuid_t;

using std::wstring;

namespace swish {
namespace nse {

Command::Command(
    const wstring& title, const uuid_t& guid,
    const wstring& tool_tip, const wstring& icon_descriptor,
    const wstring& menu_title, const wstring& webtask_title)
: m_title(title), m_guid(guid), m_tool_tip(tool_tip),
  m_icon_descriptor(icon_descriptor), m_menu_title(menu_title),
  m_webtask_title(webtask_title) {}

wstring Command::title(comet::com_ptr<IShellItemArray>) const
{ return m_title; }

const uuid_t& Command::guid() const
{ return m_guid; }

wstring Command::tool_tip(comet::com_ptr<IShellItemArray>) const
{ return m_tool_tip; }

wstring Command::icon_descriptor(comet::com_ptr<IShellItemArray>) const
{ return m_icon_descriptor; }

wstring Command::menu_title(
    comet::com_ptr<IShellItemArray> selection) const
{ return (m_menu_title.empty()) ? title(selection) : m_menu_title; }

wstring Command::webtask_title(
    comet::com_ptr<IShellItemArray> selection) const
{ return (m_webtask_title.empty()) ? title(selection) : m_webtask_title; }


}} // namespace swish::nse
