/**
    @file

    Undocumented Windows XP task pane interfaces.

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

#ifndef SWISH_NSE_UICOMMAND_HPP
#define SWISH_NSE_UICOMMAND_HPP
#pragma once

#include <comet/enum.h> // enumerated_type_of
#include <comet/interface.h> // comtype

#include <Guiddef.h> // DEFINE_GUID
#include <ShObjIdl.h> // IShellItemArray

namespace swish {
namespace nse {

DEFINE_GUID(IID_IUIElement,
    0xEC6FE84F,0xDC14,0x4FBB,0x88,0x9F,0xEA,0x50,0xFE,0x27,0xFE,0x0F);

DEFINE_GUID(IID_IUICommand,
    0x4026DFB9,0x7691,0x4142,0xB7,0x1C,0xDC,0xF0,0x8E,0xA4,0xDD,0x9C);

DEFINE_GUID(IID_IEnumUICommand,
    0x869447DA,0x9F84,0x4E2A,0xB9,0x2D,0x00,0x64,0x2D,0xC8,0xA9,0x11);

/**
 * XP folder web view item.
 *
 * Undocumented by Microsoft.  Based on public domain code at
 * http://www.whirlingdervishes.com/nselib/mfc/samples/source.php.
 *
 * Copyright (C) 1998-2003 Whirling Dervishes Software.
 */
struct IUIElement : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE get_Name(
		IShellItemArray* pItemArray, wchar_t** ppszName) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_Icon(
		IShellItemArray* pItemArray, wchar_t** ppszIcon) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_Tooltip(
		IShellItemArray* pItemArray, wchar_t** ppszInfotip) = 0;
};

/**
 * XP folder web view command.
 *
 * Undocumented by Microsoft.  Based on public domain code at
 * http://www.whirlingdervishes.com/nselib/mfc/samples/source.php.
 *
 * Copyright (C) 1998-2003 Whirling Dervishes Software.
 */
struct IUICommand : public IUIElement
{
	virtual HRESULT STDMETHODCALLTYPE get_CanonicalName(GUID* pGuid) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_State(
		IShellItemArray* pItemArray, int nRequested, EXPCMDSTATE* pState) = 0;
	virtual HRESULT STDMETHODCALLTYPE Invoke(
		IShellItemArray* pItemArray, IBindCtx* pCtx) = 0;
};

/**
 * XP folder web view command enumerator.
 *
 * Undocumented by Microsoft.  Based on public domain code at
 * http://www.whirlingdervishes.com/nselib/mfc/samples/source.php.
 *
 * Copyright (C) 1998-2003 Whirling Dervishes Software.
 */
struct IEnumUICommand : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Next(
		ULONG celt, IUICommand** rgelt, ULONG* pceltFetched) = 0;
	virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt) = 0;
	virtual HRESULT STDMETHODCALLTYPE Reset() = 0;
	virtual HRESULT STDMETHODCALLTYPE Clone(IEnumUICommand** ppenum) = 0;
};

}} // namespace swish::nse

template<> struct comet::comtype<swish::nse::IUIElement>
{
	static const IID& uuid() throw()
	{ return swish::nse::IID_IUIElement; }
	typedef IUnknown base;
};

template<> struct comet::comtype<swish::nse::IUICommand>
{
	static const IID& uuid() throw()
	{ return swish::nse::IID_IUICommand; }
	typedef swish::nse::IUIElement base;
};

template<> struct comet::comtype<swish::nse::IEnumUICommand>
{
	static const IID& uuid() throw()
	{ return swish::nse::IID_IEnumUICommand; }
	typedef IUnknown base;
};

template<>
struct comet::enumerated_type_of<swish::nse::IEnumUICommand>
{ typedef swish::nse::IUICommand* is; };

template<>
struct comet::impl::type_policy<swish::nse::IUICommand*>
{
	template<typename S>
	static void init(swish::nse::IUICommand*& p, const S& s) 
	{  p = s.get(); p->AddRef(); }

	static void clear(swish::nse::IUICommand*& p) { p->Release(); }	
};

#endif
