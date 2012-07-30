/**
    @file

    Object that dispenses backend sessions by moniker.

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

#include "Dispenser.hpp"

#include "swish/interfaces/SftpProvider.h" // ISftpProvider
#include "swish/utils.hpp" // running_object_table
#include "swish/trace.hpp" // trace

#include <comet/ptr.h> // com_ptr
#include <comet/bstr.h> // bstr_t
#include <comet/error_fwd.h> // com_error
#include <comet/interface.h> // uuidof, comtype
#include <comet/threading.h> // critical_section, auto_cs

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/regex.hpp> // Regular expressions
#include <boost/lexical_cast.hpp> // lexical_cast

#include <string>

using swish::utils::com::running_object_table;
using swish::tracing::trace;

using comet::com_ptr;
using comet::bstr_t;
using comet::com_error;
using comet::uuidof;
using comet::critical_section;
using comet::auto_cs;

using boost::lexical_cast;
using boost::numeric_cast;
using boost::wregex;
using boost::wsmatch;
using boost::lexical_cast;

using std::wstring;

namespace comet {

template<> struct comtype<IOleItemContainer>
{
    static const IID& uuid() throw() { return IID_IOleItemContainer; }
    typedef IUnknown base;
};

} // namespace comet


namespace swish {
namespace provider {
namespace dispenser {

namespace {

    critical_section lock; ///< Critical section around global sessions

    /**
     * Create an item moniker with the given name and a '!' delimeter.
     *
     * e.g. !user@host:port
     */
    com_ptr<IMoniker> create_item_moniker(const bstr_t& name)
    {
        com_ptr<IMoniker> moniker;
        HRESULT hr = ::CreateItemMoniker(
            OLESTR("!"), name.c_str(), moniker.out());
        if (FAILED(hr))
            throw com_error("Couldn't create item moniker", hr);

        return moniker;
    }

    /**
     * Fetch an item from the Running Object Table.
     */
    com_ptr<IUnknown> item_from_rot(const bstr_t& name)
    {
        com_ptr<IMoniker> moniker = create_item_moniker(name);
        com_ptr<IRunningObjectTable> rot = running_object_table();

        com_ptr<IUnknown> unknown;
        HRESULT hr = rot->GetObject(moniker.in(), unknown.out());
        if (FAILED(hr))
            throw com_error(L"Couldn't find item " + name + L" in ROT", hr);

        return unknown;
    }

    const wregex item_moniker_regex(L"(.+)@(.+):(\\d+)");
    const unsigned int USER_MATCH = 1;
    const unsigned int HOST_MATCH = 2;
    const unsigned int PORT_MATCH = 3;

    /**
     * Create a new provider session from the given item moniker name.
     */
    com_ptr<ISftpProvider> create_new_session(const wstring& name)
    {
        wsmatch match;
        if (regex_match(name, match, item_moniker_regex) &&
            match.size() == 4)
        {
            bstr_t user = match[USER_MATCH];
            bstr_t host = match[HOST_MATCH];
            unsigned int port = lexical_cast<unsigned int>(
                match[PORT_MATCH].str());

            // Create SFTP Provider from ProgID and initialise
            com_ptr<ISftpProvider> provider(L"Provider.Provider");
            HRESULT hr;
            hr = provider->Initialize(user.in(), host.in(), port);
            if (FAILED(hr))
                throw com_error("Couldn't initialise Provider", hr);

            trace("Created new session: %ls") % name;
            return provider;
        }
        else
            throw com_error("Moniker failed to parse");
    }

    void get_object( 
        const bstr_t& name, DWORD dw_speed_needed, 
        const com_ptr<IBindCtx>& /*bc*/, const IID& iid, void** object_out)
    {
        // Try to get the session from the global pool
        try
        {
            item_from_rot(name).raw()->QueryInterface(iid, object_out);
        }
        catch (const com_error& e)
        {
            trace("No existing session: %s") % e.what();

            if (dw_speed_needed != BINDSPEED_INDEFINITE)
                throw com_error("Object not running", MK_E_EXCEEDEDDEADLINE);

            if (iid != uuidof<ISftpProvider>())
                throw com_error(E_NOINTERFACE);

            // No existing session; create new one and add to the pool
            *object_out = reinterpret_cast<void**>(
                create_new_session(name).detach());
        }
    }
}

// IParseDisplayName

STDMETHODIMP CDispenser::ParseDisplayName( 
    IBindCtx* /*pbc*/, LPOLESTR pszDisplayName, ULONG* pchEaten, 
    IMoniker** ppmkOut)
{
    if (!pszDisplayName) return E_INVALIDARG;
    if (!pchEaten) return E_POINTER;
    if (!ppmkOut) return E_POINTER;

    HRESULT hr;

    hr = ::CreateItemMoniker(OLESTR("!"), pszDisplayName + 1, ppmkOut);
    if (SUCCEEDED(hr))
        *pchEaten = numeric_cast<ULONG>(::wcslen(pszDisplayName));
    else
        *pchEaten = 0;

    return hr;
}

// IOleContainer

STDMETHODIMP CDispenser::EnumObjects(
    DWORD /*grfFlags*/, IEnumUnknown** ppenum)
{
    if (!ppenum) return E_POINTER;
    *ppenum = 0;
    return E_NOTIMPL;
}

STDMETHODIMP CDispenser::LockContainer(BOOL /*fLock*/)
{
    return S_OK;
}

// IOleItemContainer

STDMETHODIMP CDispenser::GetObject( 
    LPOLESTR pszItem, DWORD dwSpeedNeeded, IBindCtx* pbc, REFIID riid,
    void** ppvObject)
{
    if (!pszItem) return E_INVALIDARG;
    if (!ppvObject) return E_POINTER;

    try
    {
        *ppvObject = 0;

        auto_cs cs(lock);
        get_object(pszItem, dwSpeedNeeded, pbc, riid, ppvObject);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();
    return S_OK;
}

STDMETHODIMP CDispenser::GetObjectStorage(
    LPOLESTR /*pszItem*/, IBindCtx* /*pbc*/, REFIID /*riid*/,
    void** ppvStorage)
{
    if (!ppvStorage) return E_POINTER;
    *ppvStorage = 0;
    return MK_E_NOSTORAGE;
}

STDMETHODIMP CDispenser::IsRunning(LPOLESTR pszItem)
{
    if (!pszItem) return E_INVALIDARG;

    bstr_t item = pszItem;
    try
    {
        try
        {
            auto_cs cs(lock);
            item_from_rot(item);
        }
        catch (...)
        {
            wsmatch match;
            if (regex_match(item.w_str(), match, item_moniker_regex))
                return S_FALSE; // Name parses correctly as a session
            else
                throw com_error(MK_E_NOOBJECT); // Not one of our monikers
        }
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();
    return S_OK;
}

}}} // namespace swish::provider::dispenser
