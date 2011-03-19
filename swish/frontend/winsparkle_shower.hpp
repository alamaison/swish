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

#ifndef SWISH_FRONTEND_WINSPARKLE_SHOWER_HPP
#define SWISH_FRONTEND_WINSPARKLE_SHOWER_HPP
#pragma once

#include <string>

namespace swish {
namespace frontend {

class winsparkle_shower
{
public:
	winsparkle_shower(
		const std::string& appcast_url, const std::wstring& app_name,
		const std::wstring& app_version, const std::wstring& company_name,
		const std::string& relative_registry_path);
	~winsparkle_shower();

	void show();

private:
	bool m_needs_cleanup;
};

}} // namespace swish::frontend

#endif
