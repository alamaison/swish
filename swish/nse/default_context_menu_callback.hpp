/**
    @file

    Handler for Explorer Default Context Menu messages.

    @if license

    Copyright (C) 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_NSE_DEFAULT_CONTEXT_MENU_CALLBACK_HPP
#define SWISH_NSE_DEFAULT_CONTEXT_MENU_CALLBACK_HPP
#pragma once

#include <comet/ptr.h> // com_ptr

#include <string>

#include <Windows.h> // HRESULT, HWND, LPARAM, WPARAM, HMENU, IDataObject

namespace swish {
namespace nse {

class default_context_menu_callback
{
public:

	virtual ~default_context_menu_callback();

	/**
	 * Cracks the @c DFM_* callback messages and dispatches them to handlers.
	 */
	HRESULT operator()(
		HWND hwnd, comet::com_ptr<IDataObject> selection, 
		UINT menu_message_id, WPARAM wparam, LPARAM lparam);

private:

	/// @name  DFM_* message handlers
	// @{

	/**
	 * A message was sent to the callback that we don't know how to crack.
	 *
	 * This method gives subclasses an opportunity to handle messages that we
	 * don't understand or new messages that Microsoft add in the future.
	 *
	 * The default implementation returns E_NOTIMPL.  Override this method in
	 * a subclass to capture unhandled messages.
	 *
	 * @warning  Any implementation must return E_NOTIMPL for messages it
	 *           doesn't recognise.  Failure to do so can cause the default
	 *           context menu to fail entirely.
	 */
	virtual HRESULT on_unknown_dfm(
		HWND hwnd_view, comet::com_ptr<IDataObject> selection, 
		UINT menu_message_id, WPARAM wparam, LPARAM lparam);

	/**
	 * The default context menu is giving us a chance to add custom items.
	 *
	 * Before returning you must set @a minimum_id to be higher than the 
	 * highest command ID you added to the menu.  The best way to do this is
	 * to increment @minimum_id for each menu item you add.
	 *
	 * Any changes we make should respect the rules specified via the flags.
	 *
	 * Return true to tell the shell to add default verbs such as Open,
	 * Explorer and Print to the menu. Return false to prevent this.
	 *
	 * The default implementation adds no items to the menu and returns true.
	 * Override this method in a subclass to change the behaviour.
	 */
	virtual bool merge_context_menu(
		HWND hwnd_view, comet::com_ptr<IDataObject> selection,
		HMENU hmenu, UINT first_item_index, UINT& minimum_id, UINT maximum_id,
		UINT allowed_changes_flags);

	/**
	 * One of the context menu commands was invoked.
	 *
	 * This could be any of the commands we added via merge_context_menu or
	 * even one of the DFM_CMD_* values which the shell adds for us.
	 *
	 * Return false to tell the shell to handle the command for us.  It may
	 * have an inbuilt action or it may just do nothing.  Return true means
	 * that we completely handled the action.
	 *
	 * The default implementation just returns true to get default shell
	 * behaviour. Override this method in a subclass to change the behaviour.
	 */
	virtual bool invoke_command(
		HWND hwnd_view, comet::com_ptr<IDataObject> selection,
		UINT item_offset, const std::wstring& arguments);

	/**
	 * One of the context menu commands was invoked.
	 *
	 * This could be any of the commands we added via merge_context_menu or
	 * even one of the DFM_CMD_* values which the shell adds for us.
	 *
	 * Return false to tell the shell to handle the command for us.  It may
	 * have an inbuilt action or it may just do nothing.  Return true means
	 * that we completely handled the action.
	 *
	 * The default implementation just returns false to get default shell
	 * behaviour. Override this method in a subclass to change the behaviour.
	 *
	 * @note The context menu site will not be set if compiled with
	 *       NTDDI_VERSION < NTDDI_VISTA.
	 */
	virtual bool invoke_command(
		HWND hwnd_view, comet::com_ptr<IDataObject> selection,
		UINT item_offset, const std::wstring& arguments,
		DWORD behaviour_flags, UINT minimum_id, UINT maximum_id,
		const CMINVOKECOMMANDINFO& invocation_details,
		comet::com_ptr<IUnknown> context_menu_site);

	/**
	 * Convert menu command ID offset to verb string.
	 */
	virtual void verb(
		HWND hwnd_view, comet::com_ptr<IDataObject> selection, 
		UINT command_id_offset, std::string& verb_out);
	virtual void verb(
		HWND hwnd_view, comet::com_ptr<IDataObject> selection, 
		UINT command_id_offset, std::wstring& verb_out);

	/**
	 * The shell is asking which item in the menu it should make default.
	 *
	 * Set the parameter to the desired ID and return true to set the default.
	 * Return false to ask the shell to choose the default itself.
	 *
	 * The default implementation returns true to make the shell choose the
	 * default.  Override this method in a subclass to change the behaviour.
	 */
	virtual bool default_menu_item(
		HWND hwnd_view, comet::com_ptr<IDataObject> selection,
		UINT& default_command_id);

	// @}

};

}} // namespace swish::nse

#endif
