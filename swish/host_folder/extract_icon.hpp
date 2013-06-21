/**
    @file

    Host folder icons.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_HOST_FOLDER_EXTRACT_ICON_HPP
#define SWISH_HOST_FOLDER_EXTRACT_ICON_HPP

#include "swish/host_folder/host_pidl.hpp" // host_itemid_view,

#include <winapi/shell/pidl.hpp> // cpidl_t
#include <winapi/window/window.hpp>

#include <boost/optional.hpp>

#include <strsafe.h> // StringCchCopy

namespace swish {
namespace host_folder {

class extract_icon_co : public comet::simple_object<IExtractIconW>
{
public:
    extract_icon_co(
        const boost::optional<winapi::window::window<wchar_t>>& owning_view,
        const winapi::shell::pidl::cpidl_t& item)
        :
    m_owning_view(owning_view), m_item(item) {}

    /**
     * Extract an icon bitmap given the information passed.
     *
     * We return S_FALSE to tell the shell to extract the icons itself.
     */
    STDMETHODIMP extract_icon_co::Extract(
        LPCTSTR /*location*/, UINT /*index*/, HICON* /*large_icon_out*/,
        HICON* /*small_icon_out*/, UINT /*desired_sizes*/)
    {
        return S_FALSE;
    }

    /**
     * Retrieve the location of the appropriate icon.
     *
     * We set all SFTP hosts to have the icon from shell32.dll.
     */
    STDMETHODIMP extract_icon_co::GetIconLocation(
        UINT /*flags*/, wchar_t* location_buffer_out, 
        UINT buffer_size, int* index_out, UINT* flags_out)
    {
        // type of use (flags) is ignored for host folder

        // Set host to have the ICS host icon
        StringCchCopy(location_buffer_out, buffer_size, L"shell32.dll");
        *index_out = 17;

        // Force call to Extract
        *flags_out = GIL_DONTCACHE;

        return S_OK;
    }

private:
    boost::optional<winapi::window::window<wchar_t>> m_owning_view;
    winapi::shell::pidl::cpidl_t m_item;
};

}}

#endif
