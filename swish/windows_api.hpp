/**
    @file

    Reimplementation of some Windows API functions.

    @if license

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.

    @endif
*/

#pragma once

#include <ShlObj.h> // ::SHGetDesktopFolder, IL-* functions

namespace swish {
namespace windows_api {

/**
 * Bind to the parent object of an absolute PIDL.
 *
 * This exists for compatability with Windows 9x which doesn't have this API
 * function.
 *
 * @returns  A pointer to the requested interface of the parent object along
 *           with a pointer to the item in the original PIDL relative to the
 *           object.
 *
 * Based on the implementation from the Wine project subject to the LGPL:
 * http://source.winehq.org/source/dlls/shell32/pidl.c#L1282
 * Copyright 1998 Juergen Schmied.
 */
inline HRESULT WINAPI SHBindToParent(
	PCIDLIST_ABSOLUTE pidl, REFIID riid, LPVOID* ppv, 
	PCUITEMID_CHILD* ppidlLast)
{
	IShellFolder* psfDesktop;
	HRESULT hr = E_FAIL;

	if (!ppv)
		return E_POINTER;
	*ppv = NULL;
	
	if (ppidlLast)
		*ppidlLast = NULL;

	if (!pidl)
		return E_INVALIDARG;

	hr = ::SHGetDesktopFolder(&psfDesktop);
	if (FAILED(hr))
		return hr;

	if (::ILIsChild(pidl))
	{
		/* we are on desktop level */
		hr = psfDesktop->QueryInterface(riid, ppv);
	}
	else
	{
		PIDLIST_ABSOLUTE pidlParent = ::ILCloneFull(pidl);
		::ILRemoveLastID(pidlParent);
		hr = psfDesktop->BindToObject(pidlParent, NULL, riid, ppv);
		::ILFree(pidlParent);
	}

	psfDesktop->Release();

	if (SUCCEEDED(hr) && ppidlLast)
		*ppidlLast = ::ILFindLastID(pidl);

	return hr;
}

}} // namespace swish::windows_api
