/* Explorer tool-bar command button implementation classes.

   Copyright (C) 2010, 2011, 2013, 2015
   Alexander Lamaison <swish@lammy.co.uk>

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

#ifndef SWISH_NSE_EXPLORER_COMMAND_HPP
#define SWISH_NSE_EXPLORER_COMMAND_HPP
#pragma once

#include "swish/nse/command_site.hpp"
#include "swish/nse/detail/command_state_conversion.hpp"
                                               // command_state_to_expcmdstate

#include <washer/object_with_site.hpp> // object_with_site

#include <comet/error.h> // com_error
#include <comet/ptr.h> // com_ptr
#include <comet/server.h> // simple_object
#include <comet/uuid_fwd.h> // uuid

#include <boost/preprocessor.hpp> // creating variadic pass-through contructors
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <shobjidl.h> // IExplorerCommandProvider/IExplorerCommand

#include <string>
#include <map>
#include <vector>

template<> struct comet::comtype<IExplorerCommand>
{
    static const IID& uuid() throw() { return IID_IExplorerCommand; }
    typedef IUnknown base;
};

template<> struct comet::comtype<IExplorerCommandProvider>
{
    static const IID& uuid() throw() { return IID_IExplorerCommandProvider; }
    typedef IUnknown base;
};

template<> struct comet::comtype<IEnumExplorerCommand>
{
    static const IID& uuid() throw() { return IID_IEnumExplorerCommand; }
    typedef IUnknown base;
};

namespace swish {
namespace nse {

class CExplorerCommandProvider :
    public comet::simple_object<IExplorerCommandProvider>
{
public:

    typedef std::vector<comet::com_ptr<IExplorerCommand> > ordered_commands;
    typedef std::map<comet::uuid_t, comet::com_ptr<IExplorerCommand> >
        command_map;

    CExplorerCommandProvider(const ordered_commands& commands);

    /**
     * Return an Explorer command instance.
     *
     * @param[in] punkSite  Optional, pointer through which to set a site.
     * @param[in] riid      IID of the requested interface (typically
     *                      IEnumExplorerCommand).
     * @param[out] ppv      Location in which to return the requested
     *                      interface.
     */
    IFACEMETHODIMP GetCommands(
        IUnknown* punkSite, const IID& riid, void** ppv);

    /**
     * Return an enumerator of IExplorerCommand instances.
     *
     * @param[in] rguidCommandId  GUID of the requested command.
     * @param[in] riid            IID of the requested interface (typically
     *                            IExplorerCommand).
     * @param[out] ppv            Location in which to return the requested
     *                            interface.
     */
    IFACEMETHODIMP GetCommand(
        const GUID& rguidCommandId, const IID& riid, void** ppv);

private:

    ordered_commands m_commands;
    command_map m_guid_mapping;
};

/**
 * Abstract IExplorerCommand implementation wrapper.
 *
 * Wraps a C++ implementation of IExplorerCommand with code to convert it
 * to the external COM interface.  This is an NVI style approach.
 */
class CExplorerCommandErrorAdapter : public IExplorerCommand
{
public:

    typedef IExplorerCommand interface_is;

    /** @name IExplorerCommand external COM methods. */
    // @{

    IFACEMETHODIMP GetTitle(
        IShellItemArray* psiItemArray, wchar_t** ppszName);

    IFACEMETHODIMP GetIcon(
        IShellItemArray* psiItemArray, wchar_t** ppszIcon);

    IFACEMETHODIMP GetToolTip(
        IShellItemArray* psiItemArray, wchar_t** ppszInfotip);

    IFACEMETHODIMP GetCanonicalName(GUID* pguidCommandName);

    IFACEMETHODIMP GetState(
        IShellItemArray* psiItemArray, BOOL fOkToBeSlow,
        EXPCMDSTATE* pCmdState);

    IFACEMETHODIMP Invoke(
        IShellItemArray* psiItemArray, IBindCtx* pbc);

    IFACEMETHODIMP GetFlags(EXPCMDFLAGS* pFlags);

    IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand** ppEnum);

    // @}

private:

    /** @name NVI internal interface.
     *
     * Implement this to create IExplorerCommand instances.
     */
    // @{
    virtual const comet::uuid_t& canonical_name() const = 0;
    virtual std::wstring title(
        const comet::com_ptr<IShellItemArray>& items) const = 0;
    virtual std::wstring tool_tip(
        const comet::com_ptr<IShellItemArray>& items) const = 0;
    virtual std::wstring icon(
        const comet::com_ptr<IShellItemArray>& items) const = 0;
    virtual EXPCMDSTATE state(
        const comet::com_ptr<IShellItemArray>& items,
        bool ok_to_be_slow) const = 0;
    virtual EXPCMDFLAGS flags() const = 0;
    virtual comet::com_ptr<IEnumExplorerCommand> subcommands() const = 0;
    virtual void invoke(
        const comet::com_ptr<IShellItemArray>& items,
        const comet::com_ptr<IBindCtx>& bind_ctx) const = 0;
    // @}
};


#ifndef SWISH_COMMAND_CONSTRUCTOR_MAX_ARGUMENTS
#define SWISH_COMMAND_CONSTRUCTOR_MAX_ARGUMENTS 10
#endif

#define EXPLORER_COMMAND_VARIADIC_CONSTRUCTOR(N, classname, initialiser) \
    BOOST_PP_EXPR_IF(N, template<BOOST_PP_ENUM_PARAMS(N, typename A)>) \
    explicit classname(BOOST_PP_ENUM_BINARY_PARAMS(N, A, a)) \
        : initialiser(BOOST_PP_ENUM_PARAMS(N, a)) {}

/**
 * Implements IExplorerCommands by wrapping command functors.
 *
 * @param T  Functor which provides the same interface as Command.
 *
 * This also implements IObjectWithSite to give the command access to the window
 * it is in.
 */
template<typename T>
class CExplorerCommand :
    public comet::simple_object<
        CExplorerCommandErrorAdapter, washer::object_with_site>
{
public:

    typedef T command_type;

// Define pass-through constructors with variable numbers of arguments
#define BOOST_PP_LOCAL_MACRO(N) \
    EXPLORER_COMMAND_VARIADIC_CONSTRUCTOR(N, CExplorerCommand, m_command)

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
        return m_command.title(items);
    }

    /**
     * Return command's tool tip.
     *
     * @param items  Optional array of PIDLs that command would be executed
     *               upon.
     */
    std::wstring tool_tip(const comet::com_ptr<IShellItemArray>& items) const
    {
        return m_command.tool_tip(items);
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
        return m_command.icon_descriptor(items);
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
        return detail::command_state_to_expcmdstate(
            m_command.state(items, ok_to_be_slow));
    }

    EXPCMDFLAGS flags() const
    {
        return 0;
    }

    comet::com_ptr<IEnumExplorerCommand> subcommands() const
    {
        BOOST_THROW_EXCEPTION(comet::com_error(E_NOTIMPL));
        return NULL;
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
        m_command(items, command_site(m_ole_site), bind_ctx);
    }

    /**
     * Let the site we have been embedded in pass us a reference to itself.
     *
     * Allows the commmand to use UI and other feature of the view.
     */
    void on_set_site(comet::com_ptr<IUnknown> ole_site)
    {
        m_ole_site = ole_site;
    }

    command_type m_command;
    comet::com_ptr<IUnknown> m_ole_site;
};

#undef EXPLORER_COMMAND_VARIADIC_CONSTRUCTOR
#undef SWISH_COMMAND_CONSTRUCTOR_MAX_ARGUMENTS

}} // namespace swish::nse

#endif
