/**
    @file

    Utility functions to work with the Windows Shell Namespace.

    @if licence

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

#include "shell.hpp"

#include "swish/exception.hpp"  // com_exception

#include <winapi/shell/shell.hpp> // strret_to_string

#include <shlobj.h>  // SHGetDesktopFolder, SHILCreateFromPath, ILFree
#include <Winerror.h>  // FAILED

#include <string>

using swish::exception::com_exception;

using winapi::shell::strret_to_string;

using comet::com_ptr;
using comet::uuidof;

using boost::filesystem::wpath;
using boost::filesystem::wdirectory_iterator;
using boost::shared_ptr;

using std::invalid_argument;
using std::wstring;
using std::vector;

namespace swish {
namespace shell_folder {

com_ptr<IShellFolder> desktop_folder()
{
	com_ptr<IShellFolder> folder;
	HRESULT hr = ::SHGetDesktopFolder(folder.out());
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_exception(hr));
	return folder;
}

#ifdef NTDDI_VERSION
wpath path_from_pidl(PIDLIST_ABSOLUTE pidl)
#else
wpath path_from_pidl(LPITEMIDLIST pidl)
#endif
{
	return parsing_name_from_pidl(pidl);
}

#ifdef NTDDI_VERSION
shared_ptr<ITEMIDLIST_ABSOLUTE> pidl_from_path(
	const wpath& filesystem_path)
{
	PIDLIST_ABSOLUTE pidl;
#else
shared_ptr<ITEMIDLIST> pidl_from_path(
	const wpath& filesystem_path)
{
	LPITEMIDLIST pidl;
#endif
	HRESULT hr = ::SHILCreateFromPath(
		filesystem_path.file_string().c_str(), &pidl, NULL);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_exception(hr));

#ifdef NTDDI_VERSION
	return shared_ptr<ITEMIDLIST_ABSOLUTE>(pidl, ::ILFree);
#else
	return shared_ptr<ITEMIDLIST>(pidl, ::ILFree);
#endif
}

com_ptr<IDataObject> data_object_for_file(const wpath& file)
{
	return data_object_for_files(&file, &file + 1);
}

com_ptr<IDataObject> data_object_for_directory(const wpath& directory)
{
	if (!is_directory(directory))
		BOOST_THROW_EXCEPTION(
			invalid_argument("The path must be to a directory."));

	return data_object_for_files(
		wdirectory_iterator(directory), wdirectory_iterator());
}

#ifdef NTDDI_VERSION
wstring parsing_name_from_pidl(PCIDLIST_ABSOLUTE pidl)
#else
wstring parsing_name_from_pidl(LPITEMIDLIST pidl)
#endif
{
	com_ptr<IShellFolder> folder;
#ifdef NTDDI_VERSION
	PCUITEMID_CHILD child_pidl;
#else
	LPITEMIDLIST child_pidl;
#endif
	HRESULT hr = swish::windows_api::SHBindToParent(
		pidl, uuidof(folder.in()), reinterpret_cast<void**>(folder.out()), 
		&child_pidl);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_exception(hr));

	STRRET str;
	hr = folder->GetDisplayNameOf(child_pidl, SHGDN_FORPARSING, &str);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(com_exception(hr));

	return strret_to_string<wchar_t>(str, child_pidl);
}

}} // namespace swish::shell_folder
