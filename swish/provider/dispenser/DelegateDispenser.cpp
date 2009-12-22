/**
    @file

    Singleton wrapper around backend dispenser.

    @if licence

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

#include "swish/catch_com.hpp" // catchCom

#include <comet/ptr.h> // com_ptr
#include <comet/error_fwd.h> // com_error
#include <comet/handle_except.h> // COMET_CATCH_CLASS
#include <comet/interface.h> // uuidof, comtype
#include <comet/threading.h> // critical_section, auto_cs

using namespace comet;

template<> struct comtype<IOleItemContainer>
{
	static const IID& uuid() throw() { return IID_IOleItemContainer; }
	typedef IUnknown base;
};

namespace swish {
namespace provider {
namespace dispenser {

static critical_section dispenser_lock; ///< Lock around global dispenser
static IOleItemContainer* global_dispenser = NULL; ///< Global dispenser

namespace {

	IOleItemContainer* dispenser()
	{
		if (!global_dispenser) // First run - no dispenser yet
		{
			auto_cs cs(dispenser_lock);

			// Check twice: keep common case fast by not locking first
			if (!global_dispenser)
			{
				com_ptr<IOleItemContainer> real(L"Provider.RealDispenser");
				global_dispenser = real.detach();
			}
		}

		return global_dispenser;
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
	catchCom()
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
	catchCom()
	return S_OK;
}

STDMETHODIMP CDelegateDispenser::LockContainer(BOOL fLock)
{
	try
	{
		return dispenser()->LockContainer(fLock) | raise_exception;
	}
	catchCom()
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
	catchCom()
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
	catchCom()
	return S_OK;
}

STDMETHODIMP CDelegateDispenser::IsRunning(LPOLESTR pszItem)
{
	try
	{
		return dispenser()->IsRunning(pszItem) | raise_exception;
	}
	catchCom()
	return S_OK;
}

}}} // namespace swish::provider::dispenser
