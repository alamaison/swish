/**
    @file

    Explorer shell view windows callback handler.

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

#ifndef SWISH_NSE_VIEW_CALLBACK_HPP
#define SWISH_NSE_VIEW_CALLBACK_HPP
#pragma once

#include "swish/nse/UICommand.hpp" // IUIElement, IEnumUICommand

#include <comet/interface.h> // comtype

#include <ShlObj.h> // IShellFolderViewCB

template<> struct comet::comtype<IShellFolderViewCB>
{
    static const IID& uuid() throw() { return IID_IShellFolderViewCB; }
    typedef IUnknown base;
};

namespace swish {
namespace nse {

class CViewCallback : public IShellFolderViewCB
{
public:

    typedef IShellFolderViewCB interface_is;

    virtual ~CViewCallback();

public: // IShellFolderViewCB

    /**
     * Callback method for shell view to inform us as things happen.
     *
     * This is the way in which the default @c IShellView object that we
     * created using @c SHCreateShellFolderView allows us to still have a say
     * in what goes on.  As things happen in the view, messages are sent to
     * this callback allowing us to react to them.
     *
     * @param message  The @c SFVM_* message type that the view is sending us.
     * @param wparam   One of the possible parameters (varies with message type).
     * @param lparam   Another possible parameter (varies with message type).
     *
     * @returns @c S_OK if we handled the message or @c E_NOTIMPL if we did not.
     */
    virtual IFACEMETHODIMP MessageSFVCB(
        UINT message, WPARAM wparam, LPARAM lparam);

protected:

    /**
     * SFVM_SELECTIONCHANGED parameter.
     *
     * Undocumented by Microsoft.  Based on public domain code at
     * http://www.whirlingdervishes.com/nselib/mfc/samples/source.php.
     *
     * Copyright (C) 1998-2003 Whirling Dervishes Software.
     */
    struct SFV_SELECTINFO
    {
        UINT uOldState; // 0
        UINT uNewState; // LVIS_SELECTED, LVIS_FOCUSED,...
        LPITEMIDLIST pidl;
    };

    /**
     * SFVM_GET_WEBVIEW_CONTENT parameter.
     *
     * Undocumented by Microsoft.  Based on public domain code at
     * http://www.whirlingdervishes.com/nselib/mfc/samples/source.php.
     *
     * Copyright (C) 1998-2003 Whirling Dervishes Software.
     */
    struct SFV_WEBVIEW_CONTENT_DATA
    {
        long l1;
        long l2;
        nse::IUIElement* pExtraTasksExpando; ///< Expando with dark title
        nse::IUIElement* pFolderTasksExpando;
        IEnumIDList* pEnumRelatedPlaces;
    };

    /**
     * SFVM_GET_WEBVIEW_TASKS parameter.
     *
     * Undocumented by Microsoft.  Based on public domain code at
     * http://www.whirlingdervishes.com/nselib/mfc/samples/source.php.
     *
     * Copyright (C) 1998-2003 Whirling Dervishes Software.
     */
    struct SFV_WEBVIEW_TASKSECTION_DATA
    {
        nse::IEnumUICommand *pEnumExtraTasks;
        nse::IEnumUICommand *pEnumFolderTasks;
    };

private:

    /// @name  SFVM_* message handlers
    // @{

    /**
     * A message was sent to the callback that we don't know how to crack.
     *
     * The message is ignored by default but can be captured by the subclasses
     * if they override on_unknown_sfvm.
     */
    virtual bool on_unknown_sfvm(UINT message, WPARAM wparam, LPARAM lparam);

    /**
     * The folder window is being created.
     *
     * The shell is notifying us of the folder view's window handle.
     */
    virtual bool on_window_created(HWND hwnd_view);

    /**
     * Which events should the shell monitor for changes?
     *
     * We are notified via SFVM_FSNOTIFY if any events indicated here occurr.
     *
     * @warning  The pidl we return in @a pidl_monitor remains owned by this
     *           object and must remain valid until this object is destroyed.
     */
    virtual bool on_get_notify(PCIDLIST_ABSOLUTE& pidl_monitor, LONG& events);

    /**
     * An event has occurred affecting one of our items.
     *
     * The event is probably the result of a SHChangeNotify of some sort.
     * Returning false prevents the default view from refreshing to reflect the
     * change.
     */
    virtual bool on_fs_notify(PCIDLIST_ABSOLUTE pidl, LONG event);

    /**
     * The view is asking us if we want to merge any items into
     * the menu it has created before it adds it to the Explorer window.
     */
    virtual bool on_merge_menu(QCMINFO& menu_info);

    /**
     * The view is telling us that something has changed about its selection
     * state.
     */
    virtual bool on_selection_changed(SFV_SELECTINFO& selection_info);

    /**
     * The view is about to display a popup menu.
     *
     * This gives us the chance to modify the menu before it is displayed.
     *
     * @param first_command_id  First ID reserved for client commands.
     * @param menu_index        Menu's index.
     * @param menu              Menu's handle.
     */
    virtual bool on_init_menu_popup(
        UINT first_command_id, int menu_index, HMENU menu);

    /**
     * The view is telling us that a menu or toolbar item has been invoked 
     * in the Explorer window and is giving us a chance to react to it.
     */
    virtual bool on_invoke_command(UINT command_id);
    
    /**
     * Specify help text for menu or toolbar items.
     */
    virtual bool on_get_help_text(
        UINT command_id, UINT buffer_size, LPTSTR buffer);
    
    /**
     * The shell view is requesting our expando title info.
     * Undocumented by Microsoft.
     *
     * @see http://www.codeproject.com/KB/shell/foldertasks.aspx
     * @see http://www.eggheadcafe.com/forumarchives/platformsdkshell/Feb2006/post25949644.asp
     */
    virtual bool on_get_webview_content(SFV_WEBVIEW_CONTENT_DATA& content_out);
    
    /**
     * The shell view is requesting our expando members.
     * Undocumented by Microsoft.
     *
     * @see http://www.codeproject.com/KB/shell/foldertasks.aspx
     * @see http://www.eggheadcafe.com/forumarchives/platformsdkshell/Feb2006/post25949644.asp
     */
    virtual bool on_get_webview_tasks(SFV_WEBVIEW_TASKSECTION_DATA& tasks_out);

    // @}

};

}} // namespace swish::nse

#endif
