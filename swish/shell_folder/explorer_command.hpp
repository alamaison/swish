/**
    @file

    Explorer tool-bar command button implementation classes.

    @if licence

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

#pragma once

#include <comet/server.h> // simple_object
#include <comet/ptr.h> // com_ptr
#include <comet/uuid_fwd.h> // uuid

#include <boost/function.hpp> // function

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
namespace shell_folder {
namespace explorer_command {

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


class CExplorerCommand : public comet::simple_object<IExplorerCommand>
{
public:

	/**
	 * Signature for functions invoked by CExplorerCommands.
	 */
	typedef boost::function<
		void(
			const comet::com_ptr<IShellItemArray>&,
			const comet::com_ptr<IBindCtx>&) >
		command;

	CExplorerCommand(
		const std::wstring& title, const comet::uuid_t& guid,
		const command& func,
		const std::wstring& tool_tip=std::wstring(),
		const std::wstring& icon=std::wstring());

	/** @name IExplorerCommand methods. */
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

	std::wstring m_title;
	GUID m_guid;
	command m_func;
	std::wstring m_tool_tip;
	std::wstring m_icon;
};

}}} // namespace swish::shell_folder::explorer_command
