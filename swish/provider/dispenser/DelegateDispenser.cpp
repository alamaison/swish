/**
    @file

    Free-threaded wrapper around backend singleton dispenser.

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

#include "DelegateDispenser.hpp"

#include "swish/utils.hpp" // class_object

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <comet/error_fwd.h> // com_error
#include <comet/interface.h> // uuidof, comtype
#include <comet/threading.h> // critical_section, auto_cs
#include <comet/server.h> // module()

using swish::utils::com::class_object;

using comet::critical_section;
using comet::auto_cs;
using comet::impl::operator |;
using comet::raise_exception;
using comet::module;

template<> struct comet::comtype<IOleItemContainer>
{
	static const IID& uuid() throw() { return IID_IOleItemContainer; }
	typedef IUnknown IOleContainer;
};

namespace swish {
namespace provider {
namespace dispenser {

namespace {

	critical_section real_dispenser_lock;
	IOleItemContainer* real_dispenser = NULL;

	/**
	 * Return a pointer to the real dispenser.
	 */
	IOleItemContainer* dispenser()
	{
		if (!real_dispenser) // First run - no dispenser yet
		{
			auto_cs cs(real_dispenser_lock);

			// Check twice: keep common case fast by not locking first
			if (!real_dispenser)
			{
				real_dispenser = class_object<IOleItemContainer>(
					L"Provider.RealDispenser").detach();
			}
		}

		return real_dispenser;
	}
}

// IParseDisplayName

STDMETHODIMP CDelegateDispenser::ParseDisplayName( 
	IBindCtx* pbc, LPOLESTR pszDisplayName, ULONG* pchEaten, 
	IMoniker** ppmkOut)
{
	try
	{
		return dispenser()->ParseDisplayName(
			pbc, pszDisplayName, pchEaten, ppmkOut) | raise_exception;
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();
	return S_OK;
}

// IOleContainer

STDMETHODIMP CDelegateDispenser::EnumObjects(
	DWORD grfFlags, IEnumUnknown** ppenum)
{
	try
	{
		return dispenser()->EnumObjects(grfFlags, ppenum) | raise_exception;
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();
	return S_OK;
}

STDMETHODIMP CDelegateDispenser::LockContainer(BOOL fLock)
{
	try
	{
		if (fLock)
			module().lock();
		else
			module().unlock();
		return dispenser()->LockContainer(fLock) | raise_exception;
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();
	return S_OK;
}

// IOleItemContainer

STDMETHODIMP CDelegateDispenser::GetObject( 
	LPOLESTR pszItem, DWORD dwSpeedNeeded, IBindCtx* pbc, REFIID riid,
	void** ppvObject)
{
	try
	{
		return dispenser()->GetObject(
			pszItem, dwSpeedNeeded, pbc, riid, ppvObject) | raise_exception;
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();
	return S_OK;
}

STDMETHODIMP CDelegateDispenser::GetObjectStorage(
	LPOLESTR pszItem, IBindCtx* pbc, REFIID riid, void** ppvStorage)
{
	try
	{
		return dispenser()->GetObjectStorage(
			pszItem, pbc, riid, ppvStorage) | raise_exception;
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();
	return S_OK;
}

STDMETHODIMP CDelegateDispenser::IsRunning(LPOLESTR pszItem)
{
	try
	{
		return dispenser()->IsRunning(pszItem) | raise_exception;
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();
	return S_OK;
}

}}} // namespace swish::provider::dispenser
