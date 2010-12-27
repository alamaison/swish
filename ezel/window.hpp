/**
    @file

    Ezel window.

    @if license

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

#ifndef EZEL_WINDOW_HPP
#define EZEL_WINDOW_HPP
#pragma once

#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/signal.hpp> // signal

#include <string>

namespace ezel {

/**
 * Base-class for window facades.
 *
 * All Ezel windows are an instance of a subclass of this this template. 
 * This provides access to the methods and properties common to all windows.
 *
 * @param T  Type of implementation class (pimpl)
 */
template<typename T>
class window
{
public:

	window(boost::shared_ptr<T> impl) : m_impl(impl) {}
	virtual ~window() {}

	std::wstring text() const { return impl()->text(); }
	void text(const std::wstring& new_text) const
	{
		return impl()->text(new_text);
	}

	void visible(bool visibility) { impl()->visible(visibility); }
	void enable(bool enablement) { impl()->enable(enablement); }

	/// @name Events
	// @{

	boost::signal<void (bool)>& on_showing() { return impl()->on_showing(); }
	boost::signal<void (bool)>& on_show() { return impl()->on_show(); }

	boost::signal<void (const wchar_t*)>& on_text_change()
	{ return impl()->on_text_change(); }

	boost::signal<void ()>& on_text_changed()
	{ return impl()->on_text_changed(); }

	// @}

protected:
	
	boost::shared_ptr<T> impl() const { return m_impl; }

private:

	boost::shared_ptr<T> m_impl; // pimpl
};

}

#endif
