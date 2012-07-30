/**
    @file

    Explorer tool-bar command button implementation classes.

    @if license

    Copyright (C) 2010, 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "explorer_command.hpp"

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <comet/enum.h> // stl_enumeration
#include <comet/ptr.h> // com_ptr
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/foreach.hpp> // BOOST_FOREACH

#include <Shlwapi.h> // SHStrDup

using comet::com_error;
using comet::com_ptr;
using comet::stl_enumeration;
using comet::uuid_t;

template<> struct comet::enumerated_type_of<IEnumExplorerCommand>
{ typedef IExplorerCommand* is; };

template<> struct comet::impl::type_policy<IExplorerCommand*>
{
    template<typename S>
    static void init(IExplorerCommand*& p, const S& s) 
    {  p = s.get(); p->AddRef(); }

    static void clear(IExplorerCommand*& p) { p->Release(); }    
};

namespace swish {
namespace nse {

#pragma region CExplorerCommandProvider implementation

/**
 * Create an ExplorerCommandProvider from exisiting ExplorerCommands.
 *
 * Store the ordered vector of commands and build a mapping from GUIDs
 * to IExplorerCommands for use when looking up via GetCommand.
 */
CExplorerCommandProvider::CExplorerCommandProvider(
    const ordered_commands& commands) : m_commands(commands)
{
    BOOST_FOREACH(comet::com_ptr<IExplorerCommand>& c, m_commands)
    {
        uuid_t guid;
        HRESULT hr = c->GetCanonicalName(guid.out());
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error(hr));
        m_guid_mapping[guid] = c;
    }
}

STDMETHODIMP CExplorerCommandProvider::GetCommands(
    IUnknown* /*punkSite*/, const IID& riid, void** ppv)
{
    if (ppv)
        *ppv = NULL;
    else
        return E_POINTER;

    try
    {
        com_ptr<IEnumExplorerCommand> commands =
            stl_enumeration<IEnumExplorerCommand>::create(
                m_commands, get_unknown());
        return commands->QueryInterface(riid, ppv);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

STDMETHODIMP CExplorerCommandProvider::GetCommand(
    const GUID& rguidCommandId, const IID& riid, void** ppv)
{
    if (ppv)
        *ppv = NULL;
    else
        return E_POINTER;

    try
    {
        command_map::const_iterator item = m_guid_mapping.find(rguidCommandId);
        if (item == m_guid_mapping.end())
            BOOST_THROW_EXCEPTION(com_error(E_FAIL));

        return item->second->QueryInterface(riid, ppv);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

#pragma endregion

#pragma region CExplorerCommandErrorAdapter implementation

/**
 * Return command's title string.
 *
 * @param[in]  psiItemArray  Optional array of PIDLs that command would be
 *                           executed upon.
 * @param[out] ppszName      Location in which to return character buffer
 *                           allocated with CoTaskMemAlloc.
 */
STDMETHODIMP CExplorerCommandErrorAdapter::GetTitle(
    IShellItemArray* psiItemArray, wchar_t** ppszName)
{
    if (ppszName)
        *ppszName = NULL;
    else
        return E_POINTER;

    try
    {
        HRESULT hr = ::SHStrDup(title(psiItemArray).c_str(), ppszName);
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error(hr));
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

/**
 * Return command's icon descriptor.
 *
 * This takes the form "shell32.dll,-249" where 249 is the icon's resource ID.
 *
 * @param[in]  psiItemArray  Optional array of PIDLs that command would be
 *                           executed upon.
 * @param[out] ppszIcon      Location in which to return character buffer
 *                           allocated with CoTaskMemAlloc.
 */
STDMETHODIMP CExplorerCommandErrorAdapter::GetIcon(
    IShellItemArray* psiItemArray, wchar_t** ppszIcon)
{
    if (ppszIcon)
        *ppszIcon = NULL;
    else
        return E_POINTER;

    try
    {
        HRESULT hr = ::SHStrDup(icon(psiItemArray).c_str(), ppszIcon);
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error(hr));
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

/**
 * Return command's tool tip.
 *
 * @param[in]  psiItemArray  Optional array of PIDLs that command would be
 *                           executed upon.
 * @param[out] ppszInfotip   Location in which to return character buffer
 *                           allocated with CoTaskMemAlloc.
 */
STDMETHODIMP CExplorerCommandErrorAdapter::GetToolTip(
    IShellItemArray* psiItemArray, wchar_t** ppszInfotip)
{
    if (ppszInfotip)
        *ppszInfotip = NULL;
    else
        return E_POINTER;

    try
    {
        HRESULT hr = ::SHStrDup(tool_tip(psiItemArray).c_str(), ppszInfotip);
        if (FAILED(hr))
            BOOST_THROW_EXCEPTION(com_error(hr));
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

/**
 * Return command's unique GUID.
 *
 * @param[out] pguidCommandName   Location in which to return GUID.
 */
STDMETHODIMP CExplorerCommandErrorAdapter::GetCanonicalName(
    GUID* pguidCommandName)
{
    if (pguidCommandName)
        *pguidCommandName = GUID_NULL;
    else
        return E_POINTER;

    try
    {
        *pguidCommandName = canonical_name();
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

/**
 * Return the command's state given array of PIDLs.
 *
 * @param[in]  psiItemArray  Optional array of PIDLs that command would be
 *                           executed upon.
 * @param[in]  fOkToBeSlow   Indicated whether slow operations can be used
 *                           when calculating the state.
 * @param[out] pCmdState     Location in which to return the state flags.
 */
STDMETHODIMP CExplorerCommandErrorAdapter::GetState(
    IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState)
{
    if (pCmdState)
        *pCmdState = 0;
    else
        return E_POINTER;

    try
    {
        *pCmdState = state(psiItemArray, (fOkToBeSlow) ? true : false);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

/**
 * Execute the code associated with this command instance.
 *
 * @param[in] psiItemArray  Optional array of PIDLs that command is
 *                          executed upon.
 * @param[in] pbc           Optional bind context.
 */
STDMETHODIMP CExplorerCommandErrorAdapter::Invoke(
    IShellItemArray* psiItemArray, IBindCtx* pbc)
{
    try
    {
        invoke(psiItemArray, pbc);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

STDMETHODIMP CExplorerCommandErrorAdapter::GetFlags(EXPCMDFLAGS* pFlags)
{
    if (pFlags)
        *pFlags = 0;
    else
        return E_POINTER;

    try
    {
        *pFlags = flags();
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

STDMETHODIMP CExplorerCommandErrorAdapter::EnumSubCommands(
    IEnumExplorerCommand** ppEnum)
{
    if (ppEnum)
        *ppEnum = NULL;
    else
        return E_POINTER;

    try
    {
        *ppEnum = subcommands().detach();
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

#pragma endregion

}} // namespace swish::nse
