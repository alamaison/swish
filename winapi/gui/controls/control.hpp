/**
    @file

    GUI control base.

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

#ifndef WINAPI_GUI_CONTROL_HPP
#define WINAPI_GUI_CONTROL_HPP
#pragma once

#include <boost/shared_ptr.hpp> // shared_ptr

#include <string>

namespace winapi {
namespace gui {

class form;

/**
 * Base-class for form control facades.
 *
 * All controls that can be added to forms are added as an instance of a
 * subclass of this this template. 
 * This allows form to access the impl pointer but nothing else.
 *
 * @param T  Type of implementation class (pimpl)
 */
template<typename T>
class control
{
public:

	control(boost::shared_ptr<T> impl) : m_impl(impl) {}
	virtual ~control() {}

	std::wstring text() const { return impl()->text(); }
	void text(const std::wstring& new_text) const
	{
		return impl()->text(new_text);
	}

protected:
	
	friend class form; // form need a p-impl from controls
	boost::shared_ptr<T> impl() const { return m_impl; }

private:

	boost::shared_ptr<T> m_impl; // pimpl
};

}} // namespace winapi::gui

#endif
