/* Windows XP web view task pane expandos.

   Copyright (C) 2010, 2013, 2015  Alexander Lamaison <swish@lammy.co.uk>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation, either version 3 of the License, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file
 * @todo  This file should probably move to the Washer project although
 *        CUICommand should stay here.
 */

#ifndef SWISH_NSE_TASK_PANE_EXPANDOS_HPP
#define SWISH_NSE_TASK_PANE_EXPANDOS_HPP
#pragma once

#include "swish/nse/command_site.hpp"
#include "swish/nse/data_object_util.hpp" // data_object_from_item_array
#include "swish/nse/detail/command_state_conversion.hpp"
                                           // command_state_to_expcmdstate
#include "swish/nse/UICommand.hpp" // IUIElement, IUICommand

#include <washer/com/catch.hpp> // WASHER_COM_CATCH_AUTO_INTERFACE
#include <washer/object_with_site.hpp> // object_with_site

#include <comet/error.h> // com_error
#include <comet/ptr.h> // com_ptr
#include <comet/server.h> // simple_object

#include <boost/preprocessor.hpp> // creating variadic pass-through contructors
#include <boost/static_assert.hpp> // BOOST_STATIC_ASSERT
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/type_traits/is_base_of.hpp> // is_base_of

#include <string>

#include <Shlwapi.h> // SHStrDupW

namespace swish {
namespace nse {

/**
 * Base class for implementations of interfaces the derive from IUIElement.
 *
 * The likely candidates are implementations of IUIElement itself and
 * IUICommand.
 *
 * This code has been factored into this templated base class as the
 * implementations must inherit from the most derived interface only.
 * Inheriting from both IUIElement and IUICommand will lead to an ambiguous
 * conversion error.
 *
 * Wraps a C++ implementation of IUIElement with code to convert it
 * to the external COM interface.  This is an NVI style approach.
 */
template<typename Interface>
class CUIElementErrorAdapterBase : public Interface
{
    BOOST_STATIC_ASSERT((boost::is_base_of<IUIElement, Interface>::value));

public:

    typedef Interface interface_is;

    /** @name IUIElement external COM methods. */
    // @{

    /**
     * Return command's title string.
     *
     * @param[in]  psiItemArray  Optional array of PIDLs that command would be
     *                           executed upon.
     * @param[out] ppszName      Location in which to return character buffer
     *                           allocated with CoTaskMemAlloc.
     */
    virtual IFACEMETHODIMP get_Name(
        IShellItemArray* psiItemArray, wchar_t** ppszName)
    {
        if (ppszName)
            *ppszName = NULL;
        else
            return E_POINTER;

        try
        {
            HRESULT hr = ::SHStrDupW(title(psiItemArray).c_str(), ppszName);
            if (FAILED(hr))
                BOOST_THROW_EXCEPTION(comet::com_error(hr));
        }
        WASHER_COM_CATCH_AUTO_INTERFACE();

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
    virtual IFACEMETHODIMP get_Icon(
        IShellItemArray* psiItemArray, wchar_t** ppszIcon)
    {
        if (ppszIcon)
            *ppszIcon = NULL;
        else
            return E_POINTER;

        try
        {
            HRESULT hr = ::SHStrDupW(icon(psiItemArray).c_str(), ppszIcon);
            if (FAILED(hr))
                BOOST_THROW_EXCEPTION(comet::com_error(hr));
        }
        WASHER_COM_CATCH_AUTO_INTERFACE();

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
    virtual IFACEMETHODIMP get_Tooltip(
        IShellItemArray* psiItemArray, wchar_t** ppszInfotip)
    {
        if (ppszInfotip)
            *ppszInfotip = NULL;
        else
            return E_POINTER;

        try
        {
            HRESULT hr =
                ::SHStrDup(tool_tip(psiItemArray).c_str(), ppszInfotip);
            if (FAILED(hr))
                BOOST_THROW_EXCEPTION(comet::com_error(hr));
        }
        WASHER_COM_CATCH_AUTO_INTERFACE();

        return S_OK;
    }

    // @}

    /** @name NVI internal interface.
     *
     * Implement this to create IUIElement instances.
     */
    // @{

    virtual std::wstring title(
        const comet::com_ptr<IShellItemArray>& items) const = 0;

    virtual std::wstring icon(
        const comet::com_ptr<IShellItemArray>& items) const = 0;

    virtual std::wstring tool_tip(
        const comet::com_ptr<IShellItemArray>& items) const = 0;

    // @}
};

/**
 * Abstract IUIElement implementation wrapper.
 *
 * Wraps a C++ implementation of IUIElement with code to convert it
 * to the external COM interface.  This is an NVI style approach.
 */
class CUIElementErrorAdapter :
    public CUIElementErrorAdapterBase<IUIElement> {};

/**
 * Abstract IUICommand implementation wrapper.
 *
 * Wraps a C++ implementation of IUICommand with code to convert it
 * to the external COM interface.  This is an NVI style approach.
 */
class CUICommandErrorAdapter : public CUIElementErrorAdapterBase<IUICommand>
{
public:

    /** @name IUICommand external COM methods. */
    // @{

    /**
     * Return command's unique GUID.
     *
     * @param[out] pguidCommandName   Location in which to return GUID.
     */
    IFACEMETHODIMP get_CanonicalName(GUID* pguidCommandName)
    {
        if (pguidCommandName)
            *pguidCommandName = GUID_NULL;
        else
            return E_POINTER;

        try
        {
            *pguidCommandName = canonical_name();
        }
        WASHER_COM_CATCH_AUTO_INTERFACE();

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
    IFACEMETHODIMP get_State(
        IShellItemArray* psiItemArray, int nRequested, //BOOL fOkToBeSlow,
        EXPCMDSTATE* pCmdState)
    {
        if (pCmdState)
            *pCmdState = 0;
        else
            return E_POINTER;

        try
        {
            *pCmdState = state(psiItemArray, (nRequested) ? true : false);
        }
        WASHER_COM_CATCH_AUTO_INTERFACE();

        return S_OK;
    }


    /**
     * Execute the code associated with this command instance.
     *
     * @param[in] psiItemArray  Optional array of PIDLs that command is
     *                          executed upon.
     * @param[in] pbc           Optional bind context.
     */
    IFACEMETHODIMP Invoke(
        IShellItemArray* psiItemArray, IBindCtx* pbc)
    {
        try
        {
            invoke(psiItemArray, pbc);
        }
        WASHER_COM_CATCH_AUTO_INTERFACE();

        return S_OK;
    }

    // @}

private:

    /** @name NVI internal interface.
     *
     * Implement this to create IUICommand instances.
     */
    // @{

    virtual const comet::uuid_t& canonical_name() const = 0;

    virtual EXPCMDSTATE state(
        const comet::com_ptr<IShellItemArray>& items,
        bool ok_to_be_slow) const = 0;

    virtual void invoke(
        const comet::com_ptr<IShellItemArray>& items,
        const comet::com_ptr<IBindCtx>& bind_ctx) const = 0;

    // @}
};

#ifndef SWISH_COMMAND_CONSTRUCTOR_MAX_ARGUMENTS
#define SWISH_COMMAND_CONSTRUCTOR_MAX_ARGUMENTS 10
#endif

#define COMMAND_ADAPTER_VARIADIC_CONSTRUCTOR(N, classname, initialiser) \
    BOOST_PP_EXPR_IF(N, template<BOOST_PP_ENUM_PARAMS(N, typename A)>) \
    explicit classname(BOOST_PP_ENUM_BINARY_PARAMS(N, A, a)) \
        : initialiser(BOOST_PP_ENUM_PARAMS(N, a)) {}

/**
 * Implements IUICommand by wrapping command functors.
 *
 * @param T  Functor which provides the same interface as Command.
 *
 * This also implements IObjectWithSite to give the command access to the window
 * it is in.
 */
template<typename T>
class CUICommand :
    public comet::simple_object<
        CUICommandErrorAdapter, washer::object_with_site>
{
public:

    typedef T command_type;

// Define pass-through contructors with variable numbers of arguments
#define BOOST_PP_LOCAL_MACRO(N) \
    COMMAND_ADAPTER_VARIADIC_CONSTRUCTOR(N, CUICommand, m_command)

#define BOOST_PP_LOCAL_LIMITS (0, SWISH_COMMAND_CONSTRUCTOR_MAX_ARGUMENTS)
#include BOOST_PP_LOCAL_ITERATE()

private:

    /**
     * Return command's unique GUID.
     */
    const comet::uuid_t& canonical_name() const
    {
        return m_command.guid();
    }

    /**
     * Return command's title string.
     *
     * @param items  Optional array of PIDLs that command would be executed
     *               upon.
     */
    std::wstring title(const comet::com_ptr<IShellItemArray>& items) const
    {
        return m_command.title(data_object_from_item_array(items));
    }

    /**
     * Return command's tool tip.
     *
     * @param items  Optional array of PIDLs that command would be executed
     *               upon.
     */
    std::wstring tool_tip(const comet::com_ptr<IShellItemArray>& items) const
    {
        return m_command.tool_tip(data_object_from_item_array(items));
    }

    /**
     * Return command's icon descriptor.
     *
     * This takes the form "shell32.dll,-249" where 249 is the icon's
     * resource ID.
     *
     * @param items  Optional array of PIDLs that command would be executed
     *               upon.
     */
    std::wstring icon(const comet::com_ptr<IShellItemArray>& items) const
    {
        return m_command.icon_descriptor(data_object_from_item_array(items));
    }

    /**
     * Return the command's state given array of PIDLs.
     *
     * @param items          Optional array of PIDLs that command would be
     *                       executed upon.
     * @param ok_to_be_slow  Indicates whether slow operations can be used
     *                       when calculating the state.  If false and slow
     *                       operations are required, throw E_PENDING.
     */
    EXPCMDSTATE state(
        const comet::com_ptr<IShellItemArray>& items, bool ok_to_be_slow)
    const
    {
        comet::com_ptr<IDataObject> data_object =
            data_object_from_item_array(items);

        return detail::command_state_to_expcmdstate(
            m_command.state(data_object, ok_to_be_slow));
    }

    /**
     * Execute the code associated with this command.
     *
     * @param items     Optional array of PIDLs that command is executed upon.
     * @param bind_ctx  Optional bind context.
     */
    void invoke(
        const comet::com_ptr<IShellItemArray>& items,
        const comet::com_ptr<IBindCtx>& bind_ctx) const
    {
        m_command(
            data_object_from_item_array(items, bind_ctx),
            command_site(m_ole_site),
            bind_ctx);
    }

    /**
     * Let the site we have been embedded in pass us a reference to itself.
     *
     * Allows the commmand to use UI and other feature of the view.
     */
    virtual void on_set_site(comet::com_ptr<IUnknown> ole_site)
    {
        m_ole_site = ole_site;
    }

    command_type m_command;
    comet::com_ptr<IUnknown> m_ole_site;
};


#undef COMMAND_ADAPTER_VARIADIC_CONSTRUCTOR
#undef SWISH_COMMAND_CONSTRUCTOR_MAX_ARGUMENTS

}} // namespace swish::nse

#endif
