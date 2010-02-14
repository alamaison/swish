/**
    @file

    Swish host folder commands.

    @if licence

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

#include "Command.hpp"

#include "swish/exception.hpp" // com_exception

#include <comet/interface.h> // uuidof

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

using swish::shell_folder::pidl::apidl_t;
using swish::exception::com_exception;

using comet::com_ptr;
using comet::uuidof;
using comet::uuid_t;

using std::wstring;

namespace swish {
namespace shell_folder {
namespace commands {

Command::Command(
	const wstring& title, const uuid_t& guid,
	const wstring& tool_tip, const wstring& icon_descriptor)
: m_title(title), m_guid(guid), m_tool_tip(tool_tip),
  m_icon_descriptor(icon_descriptor) {}

wstring Command::title(const comet::com_ptr<IDataObject>&) const
{ return m_title; }

const uuid_t& Command::guid() const
{ return m_guid; }

wstring Command::tool_tip(const comet::com_ptr<IDataObject>&) const
{ return m_tool_tip; }

wstring Command::icon_descriptor(const comet::com_ptr<IDataObject>&) const 
{ return m_icon_descriptor; }

}}} // namespace swish::shell_folder::commands
