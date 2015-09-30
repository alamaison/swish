/* Copyright (C) 2015  Alexander Lamaison <swish@lammy.co.uk>

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

#ifndef SWISH_NSE_COMMAND_SITE_HPP
#define SWISH_NSE_COMMAND_SITE_HPP

#include <washer/window/window.hpp>

#include <comet/ptr.h> // com_ptr

#include <boost/optional/optional.hpp>

#include <ObjIdl.h> // IDataObject, IBindCtx

namespace swish {
namespace nse {

/**
 * OLE site with window fallback.
 *
 * The Windows shell shituation for when you can show UI is unclear: should you
 * use the HWND passed in by the shell when it calls your NSE methods, or should
 * you use the OLE site.  Neither method is available everywhere.
 * IExplorerCommands, created via a call to CreateViewObject, never get an HWND,
 * but are treated as an OLE site.  Commands invoked via the context menu
 * integration have an HWND, but no OLE Site.  Since Vista they can receive an
 * OLE site via INVOKECOMMANDEX, but there is no guarantee that code will be
 * invoked that way and, if compiled with support for Windows XP, that argument
 * will not be available on any platform.
 *
 * One thing is clear: our UI must always have an owner window, otherwise bad
 * things may happen (see The Old New Thing book).
 *
 * The strategy we adopt here is to use this class to abstract over precisely
 * where the owner window information may arrive from.  The commands can just
 * ask this class for the window and, if any window is obtainable from any
 * source, the window is returned.  Creation sites must initialise this class
 * with whichever window sources they have: OLE site, window handle, or both.
 *
 * If ui_owner returns an uninitialised value, the calling code must not try to
 * show any UI.
 *
 * This class also makes the OLE site available, if present, for commands that
 * need more specific UI control, such as the ability to set a file icon into
 * rename mode.  This may not be avaible, and the calling code must handle that
 * possibility.
 */
class command_site
{
public:

    /**
     * A site where no UI interaction is permitted.
     */
    command_site();

    /**
     * A site where UI interaction is permitted via the OLE site.
     */
    explicit command_site(comet::com_ptr<IUnknown> ole_site);

    /**
     * A site where UI interaction is permitted via an OLE site or via a window.
     *
     * The window, if initialised, is a fallback for the UI owner if the OLE
     * site was NULL or was not able to provide a window.
     */
    command_site(
        comet::com_ptr<IUnknown> ole_site,
        const boost::optional<
            washer::window::window<wchar_t>>& ui_owner_fallback);

    boost::optional<washer::window::window<wchar_t>> ui_owner() const throw();

    comet::com_ptr<IUnknown> ole_site() const throw();

private:
    comet::com_ptr<IUnknown> m_ole_site;
    boost::optional<washer::window::window<wchar_t>> m_ui_owner_fallback;
};

}} // namespace swish::nse

#endif
