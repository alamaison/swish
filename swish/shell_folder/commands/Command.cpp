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

namespace comet {

template<> struct comtype<IDataObject>
{
	static const IID& uuid() throw() { return IID_IDataObject; }
	typedef IUnknown base;
};

}

namespace swish {
namespace shell_folder {
namespace commands {

void Command::operator()(
	const com_ptr<IShellItemArray>& items, const com_ptr<IBindCtx>& bind_ctx)
{
	com_ptr<IDataObject> data_object;
	if (items)
	{
		items->BindToHandler(
			bind_ctx.get(), BHID_DataObject, uuidof(data_object.in()),
			reinterpret_cast<void**>(data_object.out()));
	}

	// We don't care if binding succeeded - if it did, great; we pass the
	// DataObject.  If not, the data_object pointer will be NULL and we can
	// assume that no items were selected

	this->operator()(data_object, bind_ctx);
}

}}} // namespace swish::shell_folder::commands
