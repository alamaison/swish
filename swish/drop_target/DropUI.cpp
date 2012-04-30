/**
    @file

    User-interaction for DropTarget.

    @if license

    Copyright (C) 2010, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "DropUI.hpp"

#include "swish/trace.hpp" // trace

#include <winapi/gui/message_box.hpp> // message_box
#include <winapi/gui/windows/window.hpp> // window to hide progress dialog

#include <comet/error.h> // com_error
#include <comet/ptr.h> // com_ptr

#include <boost/locale.hpp> // translate, wformat
#include <boost/scoped_ptr.hpp> // scoped_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <iosfwd> // wstringstream
#include <string>

#include <OleIdl.h> // IOleWindow, IOleInPlaceFrame
#include <shobjidl.h> // IShellView, IShellBrowser

using swish::tracing::trace;

using namespace winapi::gui::message_box;
using winapi::gui::window;
using winapi::gui::progress;

using comet::com_error;
using comet::com_ptr;

using boost::locale::translate;
using boost::locale::wformat;
using boost::filesystem::wpath;
using boost::scoped_ptr;

using std::auto_ptr;
using std::wstringstream;
using std::wstring;

template<> struct comet::comtype<IOleWindow>
{
	static const IID& uuid() throw() { return IID_IOleWindow; }
	typedef IUnknown base;
};

template<> struct comet::comtype<IOleInPlaceFrame>
{
	static const IID& uuid() throw() { return IID_IOleInPlaceFrame; }
	typedef IOleInPlaceUIWindow base;
};

template<> struct comet::comtype<IShellBrowser>
{
	static const IID& uuid() throw() { return IID_IShellBrowser; }
	typedef IOleWindow base;
};

template<> struct comet::comtype<IShellView>
{
	static const IID& uuid() throw() { return IID_IShellView; }
	typedef IOleWindow base;
};

namespace swish {
namespace drop_target {

namespace {

	/**
	 * Set site UI modality.
	 *
	 * There are many types of OLE site with subtly different EnableModeless
	 * methods.  Try them in turn until one works.
	 *
	 * @todo  Add more supported site types.
	 */
	void modal(com_ptr<IUnknown> site, bool state)
	{
		BOOL enable = (state) ? TRUE : FALSE;

		if (com_ptr<IShellBrowser> browser = com_cast(site))
		{
			HRESULT hr = browser->EnableModelessSB(enable);
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(com_error_from_interface(browser, hr));
		}
		else if (com_ptr<IOleInPlaceFrame> ole_frame = com_cast(site))
		{
			HRESULT hr = ole_frame->EnableModeless(enable);
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(com_error_from_interface(ole_frame, hr));
		}
		else if (com_ptr<IShellView> view = com_cast(site))
		{
			HRESULT hr = view->EnableModeless(state);
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(com_error_from_interface(view, hr));
		}
		else
		{
			BOOST_THROW_EXCEPTION(com_error("No supported site found"));
		}
	}

	/**
	 * Prevent the OLE site from showing UI for scope of this object.
	 *
	 * The idea here is that we are about to display something like a modal
	 * dialogue box and we don't want our OLE container, such as the Explorer
	 * browser, showing its own non-modal (or even modal?) UI at the same time.
	 *
	 * OLE sites provides the EnableModeless method to disable UI for a
	 * time and this class makes sure it is reenabled safely when we go out
	 * of scope.
	 *
	 * If we fail to call this method because, for instance we can't find a
	 * suitable site we swallow the error.  This failure isn't serious enough
	 * to warrant aborting whatever wider task we're trying to achieve.
	 */
	class AutoModal
	{
	public:

		AutoModal(com_ptr<IUnknown> site) : m_site(site)
		{
			try
			{
				modal(m_site, false);
			}
			catch (const std::exception& e)
			{
				trace("Unable to make OLE site non-modal: %s") % e.what();
			}
		}

		~AutoModal()
		{
			try
			{
				modal(m_site, true);
			}
			catch (const std::exception& e)
			{
				trace("AutoModal threw in destructor: %s") % e.what();
				assert(false);
			}
		}

	private:
		com_ptr<IUnknown> m_site;
	};

	/**
	 * Ask windowed OLE container for its window handle.
	 *
	 * There are different types of OLE object with which could support this
	 * operation.  Try them in turn until one works.
     *
	 * @todo  Add more supported OLE object types.
	 */
	HWND hwnd_from_ole_window(com_ptr<IUnknown> ole_window)
	{
		HWND hwnd = NULL;

		if (com_ptr<IOleWindow> window = com_cast(ole_window))
		{
			window->GetWindow(&hwnd);
		}
		else if (com_ptr<IShellView> view = com_cast(ole_window))
		{
			view->GetWindow(&hwnd);
		}

		return hwnd;
	}
	

	/**
	 * Exception-safe lifetime manager for an IProgressDialog object.
	 *
	 * Calls StartProgressDialog when created and StopProgressDialog when
	 * destroyed.
	 */
	class AutoStartProgressDialog : public Progress
	{
	public:
		AutoStartProgressDialog(
			com_ptr<IProgressDialog> progress_instance, HWND hwnd,
			const wstring& title, com_ptr<IUnknown> ole_site=NULL)
			:
			m_inner(
				new	progress(
					progress_instance, hwnd, title,
					progress::modality::non_modal,
					progress::time_estimation::automatic_time_estimate,
					progress::bar_type::progress,
					progress::minimisable::yes,
					progress::cancellability::cancellable, ole_site))
		{}

		/**
		 * Has the user cancelled the operation via the progress dialogue?
		 */
		bool user_cancelled() const
		{
			return m_inner->user_cancelled();
		}

		/**
		 * Set the indexth line of the display to the given text.
		 */
		void line(DWORD index, const wstring& text)
		{
			m_inner->line(index, text);
		}

		/**
		 * Set the indexth line of the display to the given path.
		 *
		 * Uses the inbuilt path compression.
		 */
		void line_path(DWORD index, const wpath& path)
		{
			m_inner->line_as_compressable_path(index, path);
		}

		/**
		 * Update the indicator to show current progress level.
		 */
		void update(ULONGLONG so_far, ULONGLONG out_of)
		{
			m_inner->update(so_far, out_of);
		}

	private:
		scoped_ptr<progress> m_inner;
	};

	/**
	 * Disables an OLE window for duration of its scope and reenables after.
	 */
	class ScopedDisabler
	{
	public:
		ScopedDisabler(com_ptr<IUnknown> ole_window)
			: m_ole_window(ole_window), m_hwnd(hwnd_from_ole_window(ole_window))
		{
			if (m_hwnd)
			{
				window<wchar_t> w(m_hwnd);
				w.enable(false);
			}
		}

		~ScopedDisabler()
		{
			if (m_hwnd)
			{
				window<wchar_t> w(m_hwnd);
				w.enable(true);
			}
		}
	private:
		com_ptr<IUnknown> m_ole_window;
		HWND m_hwnd;
	};
}

DropUI::DropUI(HWND hwnd_owner) : m_hwnd_owner(hwnd_owner) {}

/**
 * Associate with a container site.
 *
 * The DropTarget is only informed of its site just before the call to Drop
 * (after this object has been created) so it informs us of the site once it
 * knows.
 */
void DropUI::site(com_ptr<IUnknown> ole_site) { m_ole_site = ole_site; }

/**
 * Does user give permission to overwrite remote target file?
 */
bool DropUI::can_overwrite(const wpath& target)
{
	if (!m_hwnd_owner)
		return false;

	wstringstream message;
	message << wformat(translate(
		"This folder already contains a file named '{1}'."))
		% target.filename();
	message << "\n\n";
	message << translate("Would you like to replace it?");

	AutoModal modal_scope(m_ole_site); // force container non-modal

	ScopedDisabler disable_progress(m_progress); // force-hide progress as it
	                                             // gets in the way of other UI

	button_type::type button = message_box(
		m_hwnd_owner, message.str(), translate("Confirm File Replace"),
		box_type::yes_no_cancel, icon_type::question);
	switch (button)
	{
	case button_type::yes:
		return true;
	case button_type::no:
		return false;
	case button_type::cancel:
	default:
		BOOST_THROW_EXCEPTION(com_error(E_ABORT));
	}
}

/**
 * Pass ownership of a progress display scope to caller.
 *
 * We hang on to the real progress dialog so that we can hide it if and when we
 * show other dialogs (something the built-in Explorer FTP extension doesn't
 * do and really should).
 *
 * The caller gets a Progress object whose lifetime determines when the dialog
 * is started and ended.  When it goes out of scope the dialog is stopped and
 * disappears.  In other words, the progress dialog is safely stopped even
 * if an exception is thrown.
 */
auto_ptr<Progress> DropUI::progress()
{
	if (!m_hwnd_owner)
		m_hwnd_owner = hwnd_from_ole_window(m_ole_site);

	if (!m_hwnd_owner)
		trace("Creating UI without a parent Window");

	m_progress = com_ptr<IProgressDialog>(CLSID_ProgressDialog);

	return auto_ptr<Progress>(
		new AutoStartProgressDialog(
			m_progress, m_hwnd_owner,
			translate("Progress", "Copying..."), m_ole_site));
}

}} // namespace swish::drop_target
