/**
    @file

    Externally COM-creatable aspects of Swish.

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

    @endif
*/

#include "swish/shell_folder/HostFolder.h"
#include "swish/shell_folder/RemoteFolder.h"
#include "swish/shell_folder/Swish.h"  // CHost/RemoteFolder UUIDs

#include <comet/server.h>

using comet::com_ptr;
using comet::make_list;
using comet::module;
using comet::regkey;
using comet::uuid_t;

struct swish_typelib;

/**
 * @name  Component descriptor types.
 *
 * These descriptor structs work like traits classes and play the same role
 * for Comet that the interface maps and RGS files do in ATL.
 *
 * Rather than adding an interface with @c COM_INTERFACE_ENTRY, add it to the
 * templated list of types called @c interfaces.  Rather than using 
 * @c COM_INTERFACE_ENTRY_CHAIN to inherit from a partial implementation,
 * add the class to the @c interface_impls list and add an @c interface_is
 * typedef to the class declaration.
 */
// @{

struct HostFolder
{
	typedef make_list<IShellFolder>::result interfaces;
    typedef make_list<>::result             source_interfaces;
    typedef make_list<CHostFolder>::result  interface_impls;
    typedef swish_typelib                   type_library;
    static const wchar_t* name() { return L"HostFolder Component"; }
    enum { major_version = 1, minor_version = 0 };
};

struct RemoteFolder
{
	typedef make_list<IShellFolder>::result  interfaces;
    typedef make_list<>::result              source_interfaces;
    typedef make_list<CRemoteFolder>::result interface_impls;
    typedef swish_typelib                    type_library;
    static const wchar_t* name() { return L"RemoteFolder Component"; }
    enum { major_version = 1, minor_version = 0 };
};

// @}

namespace comet {

template<> struct comtype<swish_typelib> {
    static const IID& uuid()
    { static const GUID iid = LIBID_SwishLib; return iid; }
    typedef nil base;
};

template<> struct comtype<HostFolder> {
    static const IID& uuid()
    { static const GUID iid = CLSID_CHostFolder; return iid; }
    typedef nil base;
};

template<> struct comtype<RemoteFolder> {
    static const IID& uuid()
    { static const GUID iid = CLSID_CRemoteFolder; return iid; }
    typedef nil base;
};

}

template<>
class custom_registration<HostFolder>
{
	uuid_t m_uuid;
	regkey m_my_computer_namespace_key;
	regkey m_approved_extensions_key;

public:
	custom_registration<HostFolder>()
		: m_uuid(comet::comtype<HostFolder>::uuid())
	{
		m_my_computer_namespace_key = regkey(HKEY_LOCAL_MACHINE).open(
			_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\")
			_T("MyComputer\\NameSpace"));
		m_approved_extensions_key = regkey(HKEY_LOCAL_MACHINE).open(
			_T("Software\\Microsoft\\Windows\\CurrentVersion\\")
			_T("Shell Extensions\\Approved"));
	};

	void on_register(const TCHAR *filename)
	{
		regkey clsid = regkey(HKEY_CLASSES_ROOT).open(_T("CLSID"));
		clsid[_T("InfoTip")] = _T("Remote file-system access via SFTP");
		clsid[_T("TileInfo")] =
			_T("prop:{28636AA6-953D-11D2-B5D6-00C04FD918D0} 5;")
			_T("{b816a850-5022-11dc-9153-0090f5284f85} 2;")
			_T("{E3E0584C-B788-4A5A-BB20-7F5A44C9ACDD} 7");

		clsid.create(_T("ShellFolder"))[_T("Attributes")] = 2684354560;
		clsid.create(_T("DefaultIcon"))[_T("")] = _T("shell32.dll,9");

		regkey k = m_my_computer_namespace_key.create(m_uuid.t_str());
		k[_T("Removal Message")] =
			_T("Please don't remove Swish this way. Uninstall it using ")
			_T("Control Panel");
		m_approved_extensions_key[m_uuid.t_str()] =
			_T("Swish HostFolder");
	}

	void on_unregister(const TCHAR *filename)
	{
		m_approved_extensions_key.delete_value(m_uuid.t_str());
		m_my_computer_namespace_key.delete_subkey(m_uuid.t_str());
	}
};

template<>
class custom_registration<RemoteFolder>
{
	uuid_t m_uuid;
	regkey m_my_computer_namespace_key;
	regkey m_approved_extensions_key;

public:
	custom_registration<RemoteFolder>()
		: m_uuid(comet::comtype<RemoteFolder>::uuid())
	{
		m_approved_extensions_key = regkey(HKEY_LOCAL_MACHINE).open(
			_T("Software\\Microsoft\\Windows\\CurrentVersion\\")
			_T("Shell Extensions\\Approved"));
	};

	void on_register(const TCHAR *filename)
	{
		regkey clsid = regkey(HKEY_CLASSES_ROOT).open(_T("CLSID"));
		clsid[_T("InfoTip")] = _T("Remote file-system access via SFTP");
		clsid[_T("TileInfo")] =
			_T("prop:{B725F130-47EF-101A-A5F1-02608C9EEBAC}, 12;")
			_T("{B725F130-47EF-101A-A5F1-02608C9EEBAC, 14}");

		clsid.create(_T("ShellFolder"))[_T("Attributes")] = 2684354560;
		clsid.create(_T("DefaultIcon"))[_T("")] = _T("shell32.dll,9");

		m_approved_extensions_key[m_uuid.t_str()] =	_T("Swish SFTP Folder");
	}

	void on_unregister(const TCHAR *filename)
	{
		m_approved_extensions_key.delete_value(m_uuid.t_str());
	}
};

template<>
class coclass_implementation<HostFolder> : public comet::coclass<HostFolder>
{
public:
	static const wchar_t* get_progid() { return L"Swish.HostFolder"; }
};

template<>
class coclass_implementation<RemoteFolder> :
	public comet::coclass<RemoteFolder>
{
public:
	static const wchar_t* get_progid() { return L"Swish.RemoteFolder"; }
};

/**
 * Type library description.
 */
struct swish_typelib
{
	typedef make_list<HostFolder, RemoteFolder>::result coclasses;
    enum { major_version = 1, minor_version = 0 };
};


namespace {

typedef comet::com_server<swish_typelib> server; ///< Comet module type

}

#pragma region DLL entry points

/** DLL Entry Point. */
extern "C" BOOL WINAPI DllMain(
	HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	return server::DllMain(hInstance, dwReason, lpReserved);
}

/** Used to determine whether the DLL can be unloaded by OLE. */
STDAPI DllCanUnloadNow()
{
    return server::DllCanUnloadNow();
}

/** Return a class factory to create an object of the requested type. */
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return server::DllGetClassObject(rclsid, riid, ppv);
}

/**
 * Add entries to the system registry.
 *
 * Registers object, typelib and all interfaces in typelib.
 */
STDAPI DllRegisterServer()
{
    return server::DllRegisterServer();
}

/** Remove entries from the system registry. */
STDAPI DllUnregisterServer()
{
	return server::DllUnregisterServer();
}

#pragma endregion