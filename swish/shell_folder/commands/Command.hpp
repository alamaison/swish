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

	/**
	 * Invoke to perform the command.
	 *
	 * Concrete commands will provide their implementation by overriding
	 * this method.
	 *
	 * @param data_object  DataObject holding items on which to perform the
	 *                     command.  This may be NULL in which case the
	 *                     command should only execute if it makes sense to
	 *                     do so regardless of selected items.
	 */
	virtual void operator()(
		const comet::com_ptr<IDataObject>& data_object,
		const comet::com_ptr<IBindCtx>& bind_ctx) const = 0;

	/** @name Attributes. */
	// @{
	const comet::uuid_t& guid() const;
	std::wstring title(
		const comet::com_ptr<IDataObject>& data_object) const;
	std::wstring tool_tip(
		const comet::com_ptr<IDataObject>& data_object) const;
	std::wstring icon_descriptor(
		const comet::com_ptr<IDataObject>& data_object) const;
	// @}

	/** @name State. */
	// @{
	virtual bool disabled(
		const comet::com_ptr<IDataObject>& data_object,
		bool ok_to_be_slow) const = 0;
	virtual bool hidden(
		const comet::com_ptr<IDataObject>& data_object,
		bool ok_to_be_slow) const = 0;
	// @}

private:
	std::wstring m_title;
	comet::uuid_t m_guid;
	std::wstring m_tool_tip;
	std::wstring m_icon_descriptor;
};

}}} // namespace swish::shell_folder::commands
