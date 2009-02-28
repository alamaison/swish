/** @file DLL exports for COM server. */

#include "stdafx.h"
#include "resource.h"

#include "Swish.h"

class CSwishModule : public CAtlDllModuleT< CSwishModule >
{
public :
	DECLARE_LIBID(LIBID_SwishLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(
		IDR_SWISH, "{b816a838-5022-11dc-9153-0090f5284f85}")
};

CSwishModule _Module;

/** DLL Entry Point. */
extern "C" BOOL WINAPI DllMain(
	HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	hInstance;
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