/**
    @file

    Standard IObjectWithSite implementation.

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

#ifndef WINAPI_OBJECT_WITH_SITE_HPP
#define WINAPI_OBJECT_WITH_SITE_HPP
#pragma once

#include <winapi/com/catch.hpp> // COM_CATCH_AUTO_INTERFACE

#include <comet/error.h> // com_error
#include <comet/interface.h> // comtype

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <OCIdl.h> // IObjectWithSite

template<> struct ::comet::comtype<::IObjectWithSite>
{
	static const ::IID& uuid() throw() { return ::IID_IObjectWithSite; }
	typedef ::IUnknown base;
};

namespace winapi {

/**
 * Mixin providing a standard implementation of IObjectWithSite.
 */
class object_with_site : public IObjectWithSite
{
public:

	typedef IObjectWithSite interface_is;

	virtual IFACEMETHODIMP SetSite(IUnknown* pUnkSite)
	{
		try
		{
			m_ole_site = pUnkSite;
			on_set_site(m_ole_site);
		}
		COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP GetSite(REFIID riid, void** ppvSite)
	{
		try
		{
			if (!ppvSite)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*ppvSite = NULL;

			HRESULT hr = m_ole_site.get()->QueryInterface(riid, ppvSite);
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(comet::com_error(m_ole_site, hr));
		}
		COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

protected:

	comet::com_ptr<IUnknown> ole_site() { return m_ole_site; }

private:

	/**
	 * Custom site action.
	 *
	 * Override this method to perform a custom action when the site is set.
	 */
	virtual void on_set_site(comet::com_ptr<IUnknown> ole_site) {}

	comet::com_ptr<IUnknown> m_ole_site;
};

} // namespace winapi

#endif
