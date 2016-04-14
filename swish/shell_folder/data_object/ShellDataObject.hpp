/**
    @file

    Classes to handle the typical Explorer 'Shell DataObject'.

    @if license

    Copyright (C) 2008, 2009, 2010, 2013
    Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <washer/shell/pidl.hpp> // pidl_t, PIDL wrapper types

#include <comet/ptr.h> // com_ptr

#include <shldisp.h> // IDataObjectAsyncCapability

namespace swish
{
namespace shell_folder
{
namespace data_object
{

/**
 * Wrapper around an IDataObject pointer providing access to the usual
 * shell formats.
 */
class ShellDataObject
{
public:
    ShellDataObject(comet::com_ptr<IDataObject> data_object);
    ~ShellDataObject();

    bool supports_async() const;
    comet::com_ptr<IDataObjectAsyncCapability> async() const;
    bool has_pidl_format() const;
    bool has_hdrop_format() const;
    bool has_file_group_descriptor_format() const;
    bool has_unicode_file_group_descriptor_format() const;
    bool has_ansi_file_group_descriptor_format() const;

private:
    comet::com_ptr<IDataObject> m_data_object;
};

/**
 * Access wrapper for the items in a DataObject's SHELL_IDLIST format.
 */
class PidlFormat
{
public:
    PidlFormat(const comet::com_ptr<IDataObject>& data_object);
    ~PidlFormat();

    washer::shell::pidl::apidl_t parent_folder() const;
    washer::shell::pidl::apidl_t file(UINT i) const;
    washer::shell::pidl::pidl_t relative_file(UINT i) const;
    UINT pidl_count() const;

private:
    comet::com_ptr<IDataObject> m_data_object;
};
}
}
} // namespace swish::shell_folder::data_object
