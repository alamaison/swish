/**
    @file

    DLL exports for in-proc COM server.

    @if license

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "resource.h"                  // main symbols
#include <swish/shell_folder/locale_setup.hpp> // LocaleSetup lifetime manager
#include "swish/shell_folder/Swish.h"  // Swish type-library

#include "swish/atl.hpp"

namespace swish {
namespace shell_folder {
namespace com_dll {

using namespace ATL;

/**
 * ATL module needed to use ATL-based objects.
 * Also provides implementation of in-proc server functions.
 */
class CSwishModule : public CAtlDllModuleT< CSwishModule >
{
public :
	DECLARE_LIBID(LIBID_SwishLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(
		IDR_SWISH, "{b816a838-5022-11dc-9153-0090f5284f85}")
};

}}} // namespace swish::shell_folder::com_dll

swish::shell_folder::com_dll::CSwishModule _Module;


/** DLL Entry Point. */
extern "C" BOOL WINAPI DllMain(
	HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	static swish::shell_folder::LocaleSetup m_locale; ///< Boost.Locale manager
	(void)hInstance;
    return _Module.DllMain(dwReason, lpReserved); 
}

/** Used to determine whether the DLL can be unloaded by OLE. */
STDAPI DllCanUnloadNow()
{
    return _Module.DllCanUnloadNow();
}

/** Return a class factory to create an object of the requested type. */
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.DllGetClassObject(rclsid, riid, ppv);
}

/**
 * Add entries to the system registry.
 *
 * Registers object, typelib and all interfaces in typelib.
 */
STDAPI DllRegisterServer()
{
    HRESULT hr = _Module.DllRegisterServer();
	return hr;
}

/** Remove entries from the system registry. */
STDAPI DllUnregisterServer()
{
	HRESULT hr = _Module.DllUnregisterServer();
	return hr;
}
