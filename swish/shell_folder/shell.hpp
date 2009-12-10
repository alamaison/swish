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

#include <comet/interface.h>  // uuidof, comtype
#include <comet/ptr.h>  // com_ptr

#include <boost/filesystem.hpp>  // wpath
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION
#include <boost/iterator/indirect_iterator.hpp>

#include <shobjidl.h>  // IShellFolder
#include <ObjIdl.h>  // IDataObject

#include <vector>
#include <string>
#include <algorithm>  // transform
#include <stdexcept>  // range_error, invalid_argument

namespace { // private

	/**
	 * Function adapter for ILFindLastID to work with transform algorithm
	 * in ui_object_of_items.
	 */
	PUITEMID_CHILD find_last_ID(const ITEMIDLIST_RELATIVE& idl)
	{
		return ::ILFindLastID(&idl);
	}
}

namespace comet {

template<> struct comtype<IShellFolder>
{
	static const IID& uuid() throw() { return IID_IShellFolder; }
	typedef IUnknown base;
};

template<> struct comtype<IDataObject>
{
	static const IID& uuid() throw() { return IID_IDataObject; }
	typedef IUnknown base;
};

}

namespace swish {
namespace shell_folder {

/**
 * Return the desktop folder IShellFolder handler.
 */
comet::com_ptr<IShellFolder> desktop_folder();

/**
 * Return the filesystem path represented by the given PIDL.
 *
 * @warning
 * The PIDL must be a PIDL to a filesystem item.  If it isn't this
 * function is likely but not guaranteed to throw an exception
 * when it converts the parsing name to a path.  If the parsing
 * name looks sufficiently path-like, however, it may silently
 * succeed and return a bogus path.
 */
boost::filesystem::wpath path_from_pidl(PIDLIST_ABSOLUTE pidl);

/**
 * Return an absolute PIDL to the item in the filesystem at the given 
 * path.
 */
boost::shared_ptr<ITEMIDLIST_ABSOLUTE> pidl_from_path(
	const boost::filesystem::wpath& filesystem_path);

/**
 * Return an IDataObject representing several files in the same folder.
 *
 * The files are passed as a half-open range of fully-qualified paths to
 * each file.
 *
 * @templateparam It  An iterator type whose items are convertible to wpath
 *                    by the wpath constructor (e.g. could be wpaths or 
 *                    strings).
 */
template<typename It>
comet::com_ptr<IDataObject> data_object_for_files(It begin, It end)
{
	std::vector<boost::shared_ptr<ITEMIDLIST_ABSOLUTE> > pidls;
	transform(begin, end, back_inserter(pidls), pidl_from_path);

	return ui_object_of_items<IDataObject>(pidls.begin(), pidls.end());
}

/**
 * Return an IDataObject representing a file on the local filesystem.
 */
comet::com_ptr<IDataObject> data_object_for_file(
	const boost::filesystem::wpath& file);

/**
 * Return an IDataObject representing all the files in a directory.
 */
comet::com_ptr<IDataObject> data_object_for_directory(
	const boost::filesystem::wpath& directory);

/**
 * Return the associated object of several items.
 *
 * This is a convenience function that binds to the items' parent and then
 * asks the parent for the associated object.  The items are passed as a 
 * half-open range of absolute PIDLs.
 *
 * Analogous to GetUIObjectOf().
 *
 * @warning
 * In order for this to work all items MUST HAVE THE SAME PARENT (i.e. they
 * must all be in the same folder).
 */
template<typename T, typename It>
comet::com_ptr<T> ui_object_of_items(It begin, It end)
{
	//
	// All the items we're passed have to have the same parent folder so
	// we just bind to the parent of the *first* item in the collection.
	//

	if (begin == end)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Empty range given"));

	comet::com_ptr<IShellFolder> parent;
	HRESULT hr = ::SHBindToParent( // &* strips smart pointer, if any
		&**begin, comet::uuidof(parent.in()),
		reinterpret_cast<void**>(parent.out()), NULL);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(swish::exception::com_exception(hr));

	std::vector<ITEMID_CHILD *> child_pidls;
	std::transform(
		boost::make_indirect_iterator(begin),
		boost::make_indirect_iterator(end),
		back_inserter(child_pidls), find_last_ID);

	comet::com_ptr<T> ui_object;
	hr = parent->GetUIObjectOf(
		NULL, child_pidls.size(),
		(child_pidls.empty()) ? NULL : &child_pidls[0],
		comet::uuidof<T>(), NULL, reinterpret_cast<void**>(ui_object.out()));
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(swish::exception::com_exception(hr));

	return ui_object;
}

/**
 * Return the associated object of an item.
 *
 * This is a convenience function that binds to the item's parent
 * and then asks the parent for the associated object.  The type of
 * associated object is determined by the template parameter.
 *
 * Analogous to GetUIObjectOf().
 *
 * @templateparam T  Type of associated object to return.
 */
template<typename T>
comet::com_ptr<T> ui_object_of_item(PCIDLIST_ABSOLUTE pidl)
{
	return ui_object_of_items<T>(&pidl, &pidl + 1);
}

/**
 * Bind to the handler object of an item.
 *
 * This handler object is usually an IShellFolder implementation but may be
 * an IStream as well as other handler types.  The type of handler is
 * determined by the template parameter.
 *
 * Analogous to BindToObject().
 *
 * @templateparam T  Type of handler to return.
 *
 * @param pidl  The item for which the handler is being requested.  Usually a
 *              PIDL for a folder when T is IShellFolder.
 *              If pidl is NULL or is the empty PIDL, the item is the Desktop
 *              folder.
 */
template<typename T>
comet::com_ptr<T> bind_to_handler_object(PCIDLIST_ABSOLUTE pidl)
{
	comet::com_ptr<IShellFolder> desktop = desktop_folder();
	comet::com_ptr<T> handler;

	if (::ILIsEmpty(pidl)) // get handler via QI
		return try_cast(desktop);

	HRESULT hr = desktop->BindToObject(
		(::ILIsEmpty(pidl)) ? NULL : pidl, NULL, comet::uuidof<T>(), 
		reinterpret_cast<void**>(handler.out()));

	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(swish::exception::com_exception(hr));
	if (!handler)
		BOOST_THROW_EXCEPTION(swish::exception::com_exception(E_FAIL));

	return handler;
}

/**
 * Return the FORPARSING name of the given PIDL.
 *
 * For filesystem items this will be the absolute path.
 */
std::wstring parsing_name_from_pidl(PCIDLIST_ABSOLUTE pidl);

}} // namespace swish::shell_folder
