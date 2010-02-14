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

#pragma once

#include "swish/shell_folder/pidl.hpp" // apidl_t

#include <comet/ptr.h> // com_ptr
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/function.hpp> // function

#include <ObjIdl.h> // IDataObject, IBindCtx
#include <shobjidl.h> // IExplorerCommandProvider, IShellItemArray

#include <string>

namespace swish {
namespace shell_folder {
namespace commands {

class Command
{
public:
	Command(
		const std::wstring& title, const comet::uuid_t& guid,
		const std::wstring& tool_tip=std::wstring(),
		const std::wstring& icon_descriptor=std::wstring());

	virtual ~Command() {}

	virtual void operator()(
		const comet::com_ptr<IShellItemArray>& items,
		const comet::com_ptr<IBindCtx>& bind_ctx);

	virtual void operator()(
		const comet::com_ptr<IDataObject>& data_object,
		const comet::com_ptr<IBindCtx>& bind_ctx) = 0;

	std::wstring title() const;
	comet::uuid_t guid() const;
	std::wstring tool_tip() const;
	std::wstring icon_descriptor() const;

private:
	std::wstring m_title;
	comet::uuid_t m_guid;
	std::wstring m_tool_tip;
	std::wstring m_icon_descriptor;
};

}}} // namespace swish::shell_folder::commands
