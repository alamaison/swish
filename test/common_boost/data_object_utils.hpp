/**
    @file

    Helper functions for tests that involve DataObjects.

    @if license

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <boost/filesystem.hpp>  // wpath

#include <comet/ptr.h>  // com_ptr

#include <ObjIdl.h>  // IDataObject

namespace test {
namespace data_object_utils {

boost::filesystem::wpath create_test_zip_file(
    const boost::filesystem::wpath& in_directory);

comet::com_ptr<IDataObject> data_object_for_zipfile(
    const boost::filesystem::wpath& zip_file);

}} // namespace test::data_object_utils
