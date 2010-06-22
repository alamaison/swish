/**
    @file

    User-interaction for DropTarget.

    @if licence

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

#ifndef SWISH_SHELL_FOLDER_DROP_UI_HPP
#define SWISH_SHELL_FOLDER_DROP_UI_HPP
#pragma once

#include "swish/shell_folder/DropTarget.hpp" // CopyCallback, Progress

#include <comet/ptr.h> // com_ptr

#include <boost/shared_ptr.hpp> // shared_ptr

namespace swish {
namespace shell_folder {

	/**
	 * DropTarget callback turning requests into GUI windows so user can
	 * handle them.
	 */
	class DropUI : public CopyCallback
	{
	public:
		DropUI(HWND hwnd_owner);

		virtual void site(comet::com_ptr<IUnknown> ole_site);

		virtual bool can_overwrite(const boost::filesystem::wpath& target);
		virtual std::auto_ptr<Progress> progress();
		
	private:
		HWND m_hwnd_owner;
		comet::com_ptr<IUnknown> m_ole_site;
	};

}} // namespace swish::shell_folder

#endif
