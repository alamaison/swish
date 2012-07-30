/**
    @file

    Host folder property columns.

    @if license

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "properties.hpp"
#include "Mode.h"          // Unix-style permissions

#include "swish/remote_folder/remote_pidl.hpp" // remote_itemid_view

#include <boost/assign.hpp> // map_list_of
#include <boost/function.hpp> // function
#include <boost/locale.hpp> // translate
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <map>

#include <initguid.h> // Make DEFINE_PROPERTYKEY() actually define a key
#include <Propkey.h> // PKEY_ *
#include <ShellAPI.h> // SHGetFileInfo

using winapi::shell::pidl::cpidl_t;
using winapi::shell::property_key;

using comet::variant_t;

using boost::assign::map_list_of;
using boost::locale::translate;

using std::map;
using std::wstring;

namespace swish {
namespace remote_folder {

DEFINE_PROPERTYKEY(
    PKEY_Group, \
    0xb816a851, 0x5022, 0x11dc, 0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, \
    0x4f, 0x85, PID_FIRST_USABLE);
DEFINE_PROPERTYKEY(
    PKEY_Permissions, \
    0xb816a851, 0x5022, 0x11dc, 0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, \
    0x4f, 0x85, PID_FIRST_USABLE + 1);
DEFINE_PROPERTYKEY(
    PKEY_OwnerId, \
    0xb816a851, 0x5022, 0x11dc, 0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, \
    0x4f, 0x85, PID_FIRST_USABLE + 2);
DEFINE_PROPERTYKEY(
    PKEY_GroupId, \
    0xb816a851, 0x5022, 0x11dc, 0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, \
    0x4f, 0x85, PID_FIRST_USABLE + 3);

namespace {

    /**
     * Find the Windows friendly type name for the file given as a PIDL.
     *
     * This type name is the one used in Explorer details.  For example,
     * something.txt is given the type name "Text Document" and a directory
     * is called a "File Folder" regardless of its name.
     */
    std::wstring lookup_friendly_typename(const cpidl_t& pidl)
    {
        DWORD dwAttributes = 
            (remote_itemid_view(pidl).is_folder()) ?
                FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;

        UINT uInfoFlags = SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME;

        SHFILEINFO shfi;
        ATLENSURE(::SHGetFileInfo(
            remote_itemid_view(pidl).filename().c_str(), dwAttributes, 
            &shfi, sizeof(shfi), uInfoFlags));
        
        return shfi.szTypeName;
    }

    class unknown_property_error : public std::runtime_error
    {
    public:
        unknown_property_error() : std::runtime_error("Unknown property") {}
    };

    typedef map<
        property_key,
        boost::function<variant_t (const cpidl_t& pidl)> >
        remote_property_map;

    variant_t label_getter(const cpidl_t& pidl)
    { return remote_itemid_view(pidl).filename(); }

    variant_t owner_getter(const cpidl_t& pidl)
    { return remote_itemid_view(pidl).owner(); }

    variant_t group_getter(const cpidl_t& pidl)
    { return remote_itemid_view(pidl).group(); }

    variant_t owner_id_getter(const cpidl_t& pidl)
    { return remote_itemid_view(pidl).owner_id(); }

    variant_t group_id_getter(const cpidl_t& pidl)
    { return remote_itemid_view(pidl).group_id(); }

    variant_t size_getter(const cpidl_t& pidl)
    { return remote_itemid_view(pidl).size(); }

    variant_t modified_date_getter(const cpidl_t& pidl)
    { return remote_itemid_view(pidl).date_modified(); }

    variant_t accessed_date_getter(const cpidl_t& pidl)
    { return remote_itemid_view(pidl).date_accessed(); }

    variant_t type_getter(const cpidl_t& pidl)
    { return lookup_friendly_typename(pidl); }

    variant_t permissions_getter(const cpidl_t& pidl)
    {
        DWORD dwPerms = remote_itemid_view(pidl).permissions();
        return mode::Mode(dwPerms).toString();
    }

    const remote_property_map remote_property_getters = map_list_of
        (PKEY_ItemNameDisplay, label_getter) // Display name (Label)
        (PKEY_FileOwner, owner_getter) // Owner
        (PKEY_Group, group_getter) // Group
        (PKEY_OwnerId, owner_id_getter) // Owner ID (UID)
        (PKEY_GroupId, group_id_getter) // Group ID (GID)
        (PKEY_Permissions, permissions_getter) // File permissions: drwxr-xr-x
        (PKEY_Size, size_getter) // File size in bytes
        (PKEY_DateModified, modified_date_getter) // Last modified date
        (PKEY_DateAccessed, accessed_date_getter) // Last accessed date
        (PKEY_ItemTypeText, type_getter); // Friendly type name
}

/**
 * Get the requested property for a file based on its PIDL.
 *
 * Many of these will be standard system properties but some are custom
 * to Swish if an appropriate one did not already exist.
 */
variant_t property_from_pidl(const cpidl_t& pidl, const property_key& key)
{
    remote_property_map::const_iterator pos = remote_property_getters.find(key);
    if (pos == remote_property_getters.end())
        BOOST_THROW_EXCEPTION(unknown_property_error());

    return (pos->second)(pidl.get());
}

/**
 * Compare two PIDLs by one of their properties.
 *
 * @param left   First PIDL in the comparison.
 * @param right  Second PIDL in the comparison.
 * @param key    Property on which to compare the two PIDLs.
 *
 * @retval -1 if left < right for chosen property.
 * @retval  0 if left == right for chosen property.
 * @retval  1 if left > right for chosen property.
 */
int compare_pidls_by_property(
    const cpidl_t& left, const cpidl_t& right, const property_key& key)
{
    if (property_from_pidl(left, key) == property_from_pidl(right, key))
        return 0;
    else if (property_from_pidl(left, key) < property_from_pidl(right, key))
        return -1;

    assert(property_from_pidl(left, key) > property_from_pidl(right, key));
    return 1;
}

}} // namespace swish::remote_folder
