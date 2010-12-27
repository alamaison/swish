/**
    @file

    Explorer tool-bar command button implementation classes.

    @if license

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_NSE_EXPLORER_COMMAND_HPP
#define SWISH_NSE_EXPLORER_COMMAND_HPP
#pragma once

#include "swish/nse/data_object_util.hpp" // data_object_from_item_array

#include <comet/error.h> // com_error
#include <comet/ptr.h> // com_ptr
#include <comet/server.h> // simple_object
#include <comet/uuid_fwd.h> // uuid

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
class CExplorerCommandImpl : public comet::simple_object<IExplorerCommand>
{
public:

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

/**
 * Implements IExplorerCommands by wrapping command functors.
 *
 * @param T  Functor which provides the same interface as Command.  It must
 *           be copyable and all methods must be const.
 */
template<typename T>
class CExplorerCommand : public CExplorerCommandImpl
{
public:

	CExplorerCommand(const T& command) : m_command(command) {}

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
		EXPCMDSTATE state = ECS_ENABLED;
		comet::com_ptr<IDataObject> data_object = 
			data_object_from_item_array(items);

		if (m_command.disabled(data_object, ok_to_be_slow))
			state |= ECS_DISABLED;
		if (m_command.hidden(data_object, ok_to_be_slow))
			state |= ECS_HIDDEN;
		return state;
	}

	EXPCMDFLAGS flags() const
	{
		return 0;
	}

	comet::com_ptr<IEnumExplorerCommand> subcommands() const
	{
		BOOST_THROW_EXCEPTION(comet::com_error(E_NOTIMPL));
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
		m_command(data_object_from_item_array(items, bind_ctx), bind_ctx);
	}

	T m_command;
};

/**
 * Create an CExplorerCommand implementation from a Command instance.
 */
template<typename T>
inline comet::com_ptr<IExplorerCommand> make_explorer_command(T command)
{
	return new CExplorerCommand<T>(command);
}

}} // namespace swish::nse

#endif
