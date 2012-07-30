/**
    @file

    Manage WinSparkle initialisation and cleanup.

    @if license

    Copyright (C) 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "winsparkle_shower.hpp"

#include <winsparkle.h>

using std::string;
using std::wstring;

namespace swish {
namespace frontend {

winsparkle_shower::winsparkle_shower(
    const string& appcast_url, const wstring& app_name, 
    const wstring& app_version, const wstring& company_name,
    const string& relative_registry_path) : m_needs_cleanup(false)
{
    win_sparkle_set_appcast_url(appcast_url.c_str());
    win_sparkle_set_registry_path(relative_registry_path.c_str());
    win_sparkle_set_app_details(
        company_name.c_str(), app_name.c_str(), app_version.c_str());
}

void winsparkle_shower::show()
{
    // the dialog may be requested more than once so we need to clean up
    // before showing it again
    if (m_needs_cleanup)
        win_sparkle_cleanup();

    m_needs_cleanup = true;
    win_sparkle_init();
}

winsparkle_shower::~winsparkle_shower()
{
    win_sparkle_cleanup();
}

}} // namespace swish::frontend
