/**
    @file

    Utility functions to work with the Windows Shell Namespace.

    @if license

    Copyright (C) 2009, 2011, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "shell.hpp"

#include <washer/shell/shell_item.hpp> // pidl_shell_item

#include <comet/error.h> // com_error

#include <shlobj.h>  // SHILCreateFromPath, ILFree
#include <Winerror.h>  // FAILED

using washer::shell::pidl_shell_item;

using comet::com_error;
using comet::com_ptr;

using boost::filesystem::path;
using boost::filesystem::directory_iterator;
using boost::shared_ptr;

using std::invalid_argument;

namespace swish {
namespace shell_folder {

path path_from_pidl(PIDLIST_ABSOLUTE pidl)
{
    return pidl_shell_item(pidl).parsing_name();
}

shared_ptr<ITEMIDLIST_ABSOLUTE> pidl_from_path(
    const path& filesystem_path)
{
    PIDLIST_ABSOLUTE pidl;
    HRESULT hr = ::SHILCreateFromPath(
        filesystem_path.wstring().c_str(), &pidl, NULL);
    if (FAILED(hr))
        BOOST_THROW_EXCEPTION(com_error(hr));

    return shared_ptr<ITEMIDLIST_ABSOLUTE>(pidl, ::ILFree);
}

com_ptr<IDataObject> data_object_for_file(const path& file)
{
    return data_object_for_files(&file, &file + 1);
}

com_ptr<IDataObject> data_object_for_directory(const path& directory)
{
    if (!is_directory(directory))
        BOOST_THROW_EXCEPTION(
            invalid_argument("The path must be to a directory."));

    return data_object_for_files(
        directory_iterator(directory), directory_iterator());
}


}} // namespace swish::shell_folder
