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

#ifndef SWISH_NSE_DATA_OJECT_UTIL_HPP
#define SWISH_NSE_DATA_OJECT_UTIL_HPP

#include <comet/interface.h> // uuidof, comtype
#include <comet/ptr.h> // com_ptr

#include <ObjIdl.h> // IDataObject, IBindCtx
#include <shobjidl.h> // IShellItemArray

template<> struct comet::comtype<IDataObject>
{
    static const IID& uuid() throw() { return IID_IDataObject; }
    typedef IUnknown base;
};

namespace swish {
namespace nse {

/**
 * Convert a ShellItemArray to a DataObject.
 *
 * This DataObject hold the items in the array in the usual form
 * expected of a shell DataObject.
 *
 * @return  NULL if there is a failure.  This indicates that the array
 *          was empty.
 */
comet::com_ptr<IDataObject> data_object_from_item_array(
    comet::com_ptr<IShellItemArray> items,
    comet::com_ptr<IBindCtx> bind_ctx=NULL);

}} // namespace swish::nse

#endif
