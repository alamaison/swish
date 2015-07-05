/**
    @file

    Utility functions to work with DataObjects.

    @if license

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

#include "data_object_util.hpp"

#include <shlguid.h> // BHID_DataObject

using comet::com_ptr;

namespace swish {
namespace nse {

com_ptr<IDataObject> data_object_from_item_array(
    com_ptr<IShellItemArray> items, com_ptr<IBindCtx> bind_ctx)
{
    com_ptr<IDataObject> data_object;
    if (items)
    {
        items->BindToHandler(
            bind_ctx.get(), BHID_DataObject, data_object.iid(),
            reinterpret_cast<void**>(data_object.out()));
    }

    // We don't care if binding succeeded - if it did, great; we pass
    // the DataObject.  If not, the data_object pointer will be NULL
    // and we can assume that no items were selected

    return data_object;
}

}} // namespace swish::nse
