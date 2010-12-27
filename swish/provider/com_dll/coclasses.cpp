/**
    @file

    Externally COM-creatable aspects of libssh2-based SFTP component.

    @if license

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

#include "swish/provider/Provider.hpp" // CProvider
#include "swish/provider/dispenser/Dispenser.hpp" // CDispenser
#include "swish/provider/dispenser/DelegateDispenser.hpp" // CDelegateDispenser

#include <comet/server.h>

using swish::provider::CProvider;
using swish::provider::dispenser::CDispenser;
using swish::provider::dispenser::CDelegateDispenser;

using comet::make_list;
using comet::com_ptr;
using comet::module;

struct provider_typelib;

#pragma region Component descriptor types

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
struct Provider
{
	typedef make_list<ISftpProvider>::result interfaces;
    typedef make_list<>::result              source_interfaces;
    typedef make_list<CProvider>::result     interface_impls;
    typedef provider_typelib                 type_library;
    static const wchar_t* name() { return L"Provider Component"; }
    enum { major_version = 1, minor_version = 0 };
};

struct RealDispenser
{
	typedef make_list<IOleItemContainer>::result interfaces;
    typedef make_list<>::result                  source_interfaces;
    typedef make_list<CDispenser>::result        interface_impls;
    typedef provider_typelib                     type_library;
    static const wchar_t* name() { return L"RealDispenser Component"; }
    enum { major_version = 1, minor_version = 0 };
};

struct Dispenser
{
    typedef make_list<IOleItemContainer>::result  interfaces;
    typedef make_list<>::result                   source_interfaces;
    typedef make_list<CDelegateDispenser>::result interface_impls;
    typedef provider_typelib                      type_library;
    static const wchar_t* name() { return L"Dispenser Component"; }
    enum { major_version = 1, minor_version = 0 };
};

// @}
#pragma endregion

namespace comet {

template<> struct comtype<provider_typelib> {
    static const IID& uuid()
    { static const GUID iid = { 0xB816A861, 0x5022, 0x11DC, { 0x91, 0x53, 0x00, 0x90, 0xF5, 0x28, 0x4F, 0x85 } }; return iid; }
    typedef nil base;
};

template<> struct comtype<Provider> {
    static const IID& uuid()
    { static const GUID iid = { 0xB816A862, 0x5022, 0x11DC, { 0x91, 0x53, 0x00, 0x90, 0xF5, 0x28, 0x4F, 0x85 } }; return iid; }
    typedef nil base;
};

template<> struct comtype<RealDispenser> {
    static const IID& uuid()
    { static const GUID iid = { 0xB816A863, 0x5022, 0x11DC, { 0x91, 0x53, 0x00, 0x90, 0xF5, 0x28, 0x4F, 0x85 } }; return iid; }
    typedef nil base;
};

template<> struct comtype<Dispenser> {
    static const IID& uuid()
    { static const GUID iid = { 0xB816A864, 0x5022, 0x11DC, { 0x91, 0x53, 0x00, 0x90, 0xF5, 0x28, 0x4F, 0x85 } }; return iid; }
    typedef nil base;
};

template<> struct comtype<IParseDisplayName>
{
	static const IID& uuid() throw() { return IID_IParseDisplayName; }
	typedef IUnknown base;
};

template<> struct comtype<IOleContainer>
{
	static const IID& uuid() throw() { return IID_IOleContainer; }
	typedef IParseDisplayName base;
};

template<> struct comtype<IOleItemContainer>
{
	static const IID& uuid() throw() { return IID_IOleItemContainer; }
	typedef IOleContainer base;
};

}

namespace {

/**
 * Base for components with static class-object lifetimes.
 *
 * Similar to comet::static_object but with templated module locking.
 */
template<typename ITF_LIST, bool LOCK_MODULE>
class class_object_base : public comet::implement_qi<ITF_LIST>
{
public:
	STDMETHOD_(ULONG, AddRef)()
	{
		if (LOCK_MODULE) module().lock();
		return 2;
	}

	STDMETHOD_(ULONG, Release)()
	{
		if (LOCK_MODULE) module().unlock();
		return 1;
	}

	STDMETHOD(LockServer)(BOOL bLock)
	{
		if (bLock)
			module().lock();
		else
			module().unlock();
		return S_OK;
	}
};

}

template<bool LOCK_MODULE>
struct comet::impl::factory_builder_aux<
	comet::impl::ft_standard, coclass_implementation<Dispenser>, LOCK_MODULE>
{
	typedef class_object_base<Dispenser::interface_impls, LOCK_MODULE>
		is_factory;
};

template<bool LOCK_MODULE>
struct comet::impl::factory_builder_aux<
	comet::impl::ft_standard, coclass_implementation<RealDispenser>,
	LOCK_MODULE>
{
	typedef class_object_base<RealDispenser::interface_impls, LOCK_MODULE>
		is_factory;
};

template<>
class coclass_implementation<Dispenser> : public CDelegateDispenser
{
public:
	enum { factory_type = comet::impl::ft_standard };
	enum { thread_model = comet::thread_model::Free };
	static const wchar_t* get_progid() { return L"Provider.Dispenser"; }
};

template<>
class coclass_implementation<RealDispenser> : public CDispenser
{
public:
	enum { factory_type = comet::impl::ft_standard };
	enum { thread_model = comet::thread_model::Both };
	static const wchar_t* get_progid() { return L"Provider.RealDispenser"; }
};

template<>
class coclass_implementation<Provider> :
	public comet::coclass<Provider, comet::thread_model::Apartment>
{
public:
	static const wchar_t* get_progid() { return L"Provider.Provider"; }
};

/**
 * Type library description.
 */
struct provider_typelib
{
	typedef make_list<Provider, RealDispenser, Dispenser>::result coclasses;
    enum { major_version = 1, minor_version = 0 };
};

namespace {

typedef comet::com_server<provider_typelib> server; ///< Comet module type

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
