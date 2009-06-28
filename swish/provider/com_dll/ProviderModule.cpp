/**
    @file

    DLL exports for in-proc COM server and ATL module implementation.

    @if licence

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "pch.h"

#include "resource.h"            // main symbols
#include "com_dll.h"             // provider UUIDs (generated header)

#include "swish/atl.hpp"         // Common ATL setup

namespace swish {
namespace provider {
namespace com_dll {

using ATL::CAtlDllModuleT;

class CProviderModule : public CAtlDllModuleT< CProviderModule >
{
public :
	DECLARE_LIBID(LIBID_ProviderLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(
		IDR_PROVIDERDLL, "{b816a860-5022-11dc-9153-0090f5284f85}")
};

}}} // namespace swish::provider::com_dll

swish::provider::com_dll::CProviderModule _AtlModule;


/** DLL Entry Point. */
extern "C" BOOL WINAPI DllMain(
	HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	hInstance;
    return _AtlModule.DllMain(dwReason, lpReserved); 
}

/** Used to determine whether the DLL can be unloaded by OLE. */
STDAPI DllCanUnloadNow()
{
    return _AtlModule.DllCanUnloadNow();
}

/** Return a class factory to create an object of the requested type. */
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

/**
 * Add entries to the system registry.
 *
 * Registers object, typelib and all interfaces in typelib.
 */
STDAPI DllRegisterServer()
{
    HRESULT hr = _AtlModule.DllRegisterServer();
	return hr;
}

/** Remove entries from the system registry. */
STDAPI DllUnregisterServer()
{
	HRESULT hr = _AtlModule.DllUnregisterServer();
	return hr;
}
