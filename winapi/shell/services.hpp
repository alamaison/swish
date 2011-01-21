/**
    @file

    Helpers to access services provided by Windows shell objects.

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

#ifndef WINAPI_SHELL_SERVICES_HPP
#define WINAPI_SHELL_SERVICES_HPP
#pragma once

#include <comet/interface.h> // comtype
#include <comet/ptr.h> // com_ptr

#include <ServProv.h> // IServiceProvider
#include <shobjidl.h> // IShellBrowser, IShellView

template<> struct comet::comtype<IServiceProvider>
{
	static const IID& uuid() throw() { return IID_IServiceProvider; }
	typedef IUnknown base;
};

template<> struct comet::comtype<IShellBrowser>
{
	static const IID& uuid() throw() { return IID_IShellBrowser; }
	typedef IOleWindow base;
};

template<> struct comet::comtype<IShellView>
{
	static const IID& uuid() throw() { return IID_IShellView; }
	typedef IOleWindow base;
};

namespace winapi {
namespace shell {

/**
 * Return the parent IShellBrowser from an OLE site.
 */
inline comet::com_ptr<IShellBrowser> shell_browser(
	comet::com_ptr<IUnknown> ole_site)
{
	if (!ole_site)
		BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

	comet::com_ptr<IServiceProvider> sp = try_cast(ole_site);

	comet::com_ptr<IShellBrowser> browser;
	HRESULT hr = sp->QueryService(SID_SShellBrowser, browser.out());
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(comet::com_error_from_interface(sp, hr));

	return browser;
}

/**
 * Return the parent IShellView of a shell browser.
 */
inline comet::com_ptr<IShellView> shell_view(
	comet::com_ptr<IShellBrowser> browser)
{
	comet::com_ptr<IShellView> view;
	HRESULT hr = browser->QueryActiveShellView(view.out());
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(comet::com_error_from_interface(browser, hr));

	return view;
}

}} // namespace winapi::shell

#endif
