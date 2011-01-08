/**
    @file

    TaskDialog C++ wrapper.

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

#ifndef WINAPI_GUI_TASK_DIALOG_HPP
#define WINAPI_GUI_TASK_DIALOG_HPP
#pragma once

#include <winapi/dynamic_link.hpp> // proc_address

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/integer_traits.hpp> // integer_traits
#include <boost/function.hpp> // function
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <comet/error.h>

#include <algorithm> // transform
#include <stdexcept> // invalid_argument
#include <string>
#include <vector>
#include <map>
#include <utility> // pair

#include <commctrl.h> // TASKDIALOG values

#pragma comment(linker, \
    "\"/manifestdependency:type='Win32' "\
    "name='Microsoft.Windows.Common-Controls' "\
    "version='6.0.0.0' "\
    "processorArchitecture='*' "\
    "publicKeyToken='6595b64144ccf1df' "\
    "language='*'\"")

#if _WIN32_WINNT < 0x0600 && !defined(TD_WARNING_ICON)

//
// Our task dialog class is designed to fail gracefully even on versions of
// Windows without TaskDialog support so we don't require
// NTDDI_VERSION >= NTDDI_VISTA.  However, we must provide our own definitions
// of the task dialog structs/enums as the Windows SDK excludes the when
// building for pre-Vista Windows.
//

#ifdef _WIN32
#include <pshpack1.h>
#endif

typedef HRESULT (CALLBACK *PFTASKDIALOGCALLBACK)(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData);

enum _TASKDIALOG_FLAGS
{
	TDF_ENABLE_HYPERLINKS               = 0x0001,
	TDF_USE_HICON_MAIN                  = 0x0002,
	TDF_USE_HICON_FOOTER                = 0x0004,
	TDF_ALLOW_DIALOG_CANCELLATION       = 0x0008,
	TDF_USE_COMMAND_LINKS               = 0x0010,
	TDF_USE_COMMAND_LINKS_NO_ICON       = 0x0020,
	TDF_EXPAND_FOOTER_AREA              = 0x0040,
	TDF_EXPANDED_BY_DEFAULT             = 0x0080,
	TDF_VERIFICATION_FLAG_CHECKED       = 0x0100,
	TDF_SHOW_PROGRESS_BAR               = 0x0200,
	TDF_SHOW_MARQUEE_PROGRESS_BAR       = 0x0400,
	TDF_CALLBACK_TIMER                  = 0x0800,
	TDF_POSITION_RELATIVE_TO_WINDOW     = 0x1000,
	TDF_RTL_LAYOUT                      = 0x2000,
	TDF_NO_DEFAULT_RADIO_BUTTON         = 0x4000,
	TDF_CAN_BE_MINIMIZED                = 0x8000
};
typedef int TASKDIALOG_FLAGS;

typedef enum _TASKDIALOG_MESSAGES
{
	TDM_NAVIGATE_PAGE                       = WM_USER + 101,
	TDM_CLICK_BUTTON                        = WM_USER + 102,
	TDM_SET_MARQUEE_PROGRESS_BAR            = WM_USER + 103,
	TDM_SET_PROGRESS_BAR_STATE              = WM_USER + 104,
	TDM_SET_PROGRESS_BAR_RANGE              = WM_USER + 105,
	TDM_SET_PROGRESS_BAR_POS                = WM_USER + 106,
	TDM_SET_PROGRESS_BAR_MARQUEE            = WM_USER + 107,
	TDM_SET_ELEMENT_TEXT                    = WM_USER + 108,
	TDM_CLICK_RADIO_BUTTON                  = WM_USER + 110,
	TDM_ENABLE_BUTTON                       = WM_USER + 111,
	TDM_ENABLE_RADIO_BUTTON                 = WM_USER + 112,
	TDM_CLICK_VERIFICATION                  = WM_USER + 113,
	TDM_UPDATE_ELEMENT_TEXT                 = WM_USER + 114,
	TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE = WM_USER + 115,
	TDM_UPDATE_ICON                         = WM_USER + 116 
} TASKDIALOG_MESSAGES;

typedef enum _TASKDIALOG_NOTIFICATIONS
{
	TDN_CREATED                         = 0,
	TDN_NAVIGATED                       = 1,
	TDN_BUTTON_CLICKED                  = 2,
	TDN_HYPERLINK_CLICKED               = 3,
	TDN_TIMER                           = 4,
	TDN_DESTROYED                       = 5,
	TDN_RADIO_BUTTON_CLICKED            = 6,
	TDN_DIALOG_CONSTRUCTED              = 7,
	TDN_VERIFICATION_CLICKED            = 8,
	TDN_HELP                            = 9,
	TDN_EXPANDO_BUTTON_CLICKED          = 10
} TASKDIALOG_NOTIFICATIONS;

struct TASKDIALOG_BUTTON
{
	int     nButtonID;
	PCWSTR  pszButtonText;
};

typedef enum _TASKDIALOG_ELEMENTS
{
	TDE_CONTENT,
	TDE_EXPANDED_INFORMATION,
	TDE_FOOTER,
	TDE_MAIN_INSTRUCTION
} TASKDIALOG_ELEMENTS;

typedef enum _TASKDIALOG_ICON_ELEMENTS
{
	TDIE_ICON_MAIN,
	TDIE_ICON_FOOTER
} TASKDIALOG_ICON_ELEMENTS;

#define TD_WARNING_ICON         MAKEINTRESOURCEW(-1)
#define TD_ERROR_ICON           MAKEINTRESOURCEW(-2)
#define TD_INFORMATION_ICON     MAKEINTRESOURCEW(-3)
#define TD_SHIELD_ICON          MAKEINTRESOURCEW(-4)

enum _TASKDIALOG_COMMON_BUTTON_FLAGS
{
	TDCBF_OK_BUTTON            = 0x0001,
	TDCBF_YES_BUTTON           = 0x0002,
	TDCBF_NO_BUTTON            = 0x0004,
	TDCBF_CANCEL_BUTTON        = 0x0008,
	TDCBF_RETRY_BUTTON         = 0x0010,
	TDCBF_CLOSE_BUTTON         = 0x0020
};
typedef int TASKDIALOG_COMMON_BUTTON_FLAGS;

struct TASKDIALOGCONFIG
{
	UINT                           cbSize;
	HWND                           hwndParent;
	HINSTANCE                      hInstance;
	TASKDIALOG_FLAGS               dwFlags;
	TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons;
	PCWSTR                         pszWindowTitle;
	union
	{
		HICON  hMainIcon;
		PCWSTR pszMainIcon;
	};
	PCWSTR                         pszMainInstruction;
	PCWSTR                         pszContent;
	UINT                           cButtons;
	const TASKDIALOG_BUTTON        *pButtons;
	int                            nDefaultButton;
	UINT                           cRadioButtons;
	const TASKDIALOG_BUTTON        *pRadioButtons;
	int                            nDefaultRadioButton;
	PCWSTR                         pszVerificationText;
	PCWSTR                         pszExpandedInformation;
	PCWSTR                         pszExpandedControlText;
	PCWSTR                         pszCollapsedControlText;
	union
	{
		HICON  hFooterIcon;
		PCWSTR pszFooterIcon;
	};
	PCWSTR                         pszFooter;
	PFTASKDIALOGCALLBACK           pfCallback;
	LONG_PTR                       lpCallbackData;
	UINT                           cxWidth;
};

#ifdef _WIN32
#include <poppack.h>
#endif

#endif


namespace winapi {
namespace gui {
namespace task_dialog {

typedef boost::function<
	HRESULT (const TASKDIALOGCONFIG*, int*, int*, BOOL*)> tdi_function;

namespace detail {

	tdi_function bind_task_dialog_indirect()
	{
		return winapi::proc_address<
			HRESULT (WINAPI *)(const TASKDIALOGCONFIG*, int*, int*, BOOL*)>(
			"comctl32.dll", "TaskDialogIndirect");
	}
}

namespace button_type
{
	enum type
	{
		ok,
		cancel,
		yes,
		no,
		retry,
		close
	};
}

namespace icon_type
{
	enum type
	{
		none,
		warning,
		error,
		information,
		shield
	};
}

namespace detail {

	TASKDIALOG_COMMON_BUTTON_FLAGS button_to_tdcbf(button_type::type id)
	{
		switch (id)
		{
		case button_type::ok:
			return TDCBF_OK_BUTTON;
		case button_type::cancel:
			return TDCBF_CANCEL_BUTTON;
		case button_type::yes:
			return TDCBF_YES_BUTTON;
		case button_type::no:
			return TDCBF_NO_BUTTON;
		case button_type::retry:
			return TDCBF_RETRY_BUTTON;
		case button_type::close:
			return TDCBF_CLOSE_BUTTON;
		default:
			BOOST_THROW_EXCEPTION(
				std::invalid_argument("Unknown button type"));
		}
	}

	const wchar_t* icon_to_tdicon(icon_type::type type)
	{
		switch (type)
		{
		case icon_type::none:
			return NULL;
		case icon_type::warning:
			return TD_WARNING_ICON;
		case icon_type::error:
			return TD_ERROR_ICON;
		case icon_type::information:
			return TD_INFORMATION_ICON;
		case icon_type::shield:
			return TD_SHIELD_ICON;
		default:
			BOOST_THROW_EXCEPTION(
				std::invalid_argument("Unknown icon type"));
		}
	}

	TASKDIALOG_BUTTON convert_td_button(
		const std::pair<int, std::wstring>& button)
	{
		TASKDIALOG_BUTTON tdb;
		tdb.nButtonID = button.first;
		tdb.pszButtonText = button.second.c_str();
		return tdb;
	}
}

/**
 * Wrapper around the Windows TaskDialog.
 *
 * It calls TaskDialogIndirect by binding to it dynamically so will fail
 * gracefully by throwing an exception on versions of Windows prior to Vista.
 *
 * @param T  Type of value returned by the button callbacks and @c show().
 */
template<typename T=void>
class task_dialog
{
public:
	typedef boost::function<T ()> button_callback;
	typedef std::pair<int, std::wstring> button;

	/**
	 * Create a TaskDialog.
	 *
	 * @param parent_hwnd            Handle to parent window, may be NULL.
	 * @param main_instruction       Text of main instruction line.
	 * @param content                Text of dialogue body.
	 * @param window_title           Title of dialogue window.
	 * @param icon                   Icon to display or none.  One of 
	 *                               @c icon_type.
	 * @param cancellation_callback  Function to call if dialogue is cancelled.
	 *                               Use this if you aren't going to add a
	 *                               common cancel button to the dialogue but
	 *                               still want it to respond to Alt+F4, Esc,
	 *                               etc., as though a cancel button had been
	 *                               clicked.
	 * @param use_command_links      If true (default) display custom buttons
	 *                               as large panes arranged vertically in the
	 *                               body of the dialog.  Otherwise, display
	 *                               them with the common buttons arranged
	 *                               horizontally at the bottom.
	 * @param td_implementation      Implementation of TaskDialogIndirect. By
	 *                               default this is the stock implementation
	 *                               from comctl32.dll but can be changed to a
	 *                               custom implementation (e.g. a TaskDialog
	 *                               emulator for earlier versions of Windows).
	 */
	task_dialog(
		HWND parent_hwnd, const std::wstring& main_instruction,
		const std::wstring& content, const std::wstring& window_title,
		icon_type::type icon=icon_type::none,
		bool use_command_links=true,
		button_callback cancellation_callback=button_callback(),
		tdi_function td_implementation=detail::bind_task_dialog_indirect())
		:
		m_task_dialog_indirect(td_implementation),
		m_hwnd(parent_hwnd),
		m_main_instruction(main_instruction),
		m_content(content),
		m_window_title(window_title),
		m_icon(icon),
		m_cancellation_callback(cancellation_callback),
		m_use_command_links(use_command_links),
		m_common_buttons(0),
		m_default_button(0),
		m_default_radio_button(0)
		{}

	/**
	 * Display the Task dialog and return when a button is clicked.
	 *
	 * @returns any value returned by the clicked button's callback.
	 */
	T show()
	{
		// basic
		TASKDIALOGCONFIG tdc = {0};
		tdc.cbSize = sizeof(TASKDIALOGCONFIG);

		tdc.hwndParent = m_hwnd;

		// strings
		tdc.pszMainInstruction = m_main_instruction.c_str();
		tdc.pszContent = m_content.c_str();
		tdc.pszWindowTitle = m_window_title.c_str();
		tdc.pszMainIcon = detail::icon_to_tdicon(m_icon);

		// flags
		if (m_use_command_links && !m_buttons.empty())
			tdc.dwFlags |= TDF_USE_COMMAND_LINKS;
		if (!m_cancellation_callback.empty())
		{
			tdc.dwFlags |= TDF_ALLOW_DIALOG_CANCELLATION;
			m_callbacks[IDCANCEL] = m_cancellation_callback;
		}

		// common buttons
		tdc.dwCommonButtons = m_common_buttons;

		// custom buttons
		std::vector<TASKDIALOG_BUTTON> buttons;
		if (!m_buttons.empty())
		{
			std::transform(
				m_buttons.begin(), m_buttons.end(),
				back_inserter(buttons), detail::convert_td_button);
			
			tdc.cButtons = boost::numeric_cast<int>(buttons.size());
			tdc.pButtons = &buttons[0];
		}
		tdc.nDefaultButton = m_default_button;

		// radio buttons
		std::vector<TASKDIALOG_BUTTON> radio_buttons;
		if (!m_radio_buttons.empty())
		{
			std::transform(
				m_radio_buttons.begin(), m_radio_buttons.end(),
				back_inserter(radio_buttons), detail::convert_td_button);
			tdc.cRadioButtons = boost::numeric_cast<int>(radio_buttons.size());
			tdc.pRadioButtons = &radio_buttons[0];
		}
		tdc.nDefaultRadioButton = m_default_radio_button;

		int which_button;
		HRESULT hr = m_task_dialog_indirect(&tdc, &which_button, NULL, NULL);
		if (hr != S_OK)
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(comet::com_error(hr)) << 
				boost::errinfo_api_function("TaskDialogIndirect"));

		if (m_callbacks.empty())
			return T(); // windows may add a default button if we didn't

		return m_callbacks[which_button]();
	}


	/**
	 * Add a common button such as OK or Cancel to the bottom of the dialogue.
	 *
	 * @param type      Button type.  One from @c button_type.
	 * @param callback  Function to call if this button is clicked.  If the
	 *                  function returns a value it will be returned from the
	 *                  call to @c show().
	 * @param default   If true, set this to be the default button.
	 */
	void add_button(
		button_type::type type, button_callback callback, bool default=false)
	{
		assert(
			m_callbacks.find(detail::button_to_tdcbf(type)) == 
			m_callbacks.end());

		m_common_buttons |= detail::button_to_tdcbf(type);
		m_callbacks[type] = callback;
		if (default)
			m_default_button = type;
	}

	/**
	 * Add a custom button to the dialogue.
	 *
	 * Buttons will be displayed in the order they are added.  If this
	 * task_dialog was contructed with command links enabled then these
	 * buttons will be diplayed in the bopdy of the dialogue arranged
	 * vertically.  Otherwise, they will appear with the common buttons
	 * at the bottom of the dialogue, arrange horizontally.
	 *
	 * @param caption   Text to show on the button.  If command links are
	 *                  enabled then then any text after the first newline
	 *                  will appear as secondary text on the link button.
	 * @param callback  Function to call if this button is clicked.  If the
	 *                  function returns a value it will be returned from the
	 *                  call to @c show().
	 * @param default   If true, set this to be the default button.
	 */
	void add_button(
		const std::wstring& caption, button_callback callback,
		bool default=false)
	{
		// the common button IDs start at 1 so we generate IDs for the custom
		// buttons starting with maxint to make collisions as unlikely as
		// possible
		// the next available index in the table is the maximum int minus
		// the number of custom buttons we have already added
		// adding common buttons won't affect the next index because they're
		// stored seperately.

		int next_id = 
			boost::integer_traits<int>::const_max - 
			boost::numeric_cast<int>(m_buttons.size());
		assert(m_callbacks.find(next_id) == m_callbacks.end());

		m_buttons.push_back(button(next_id, caption));
		m_callbacks[next_id] = callback;
		if (default)
			m_default_button = next_id;
	}

	/**
	 * Add a radio button to the dialog.
	 *
	 * They are displayed in the order they are added.
	 *
	 * @todo  Find a way to actually get back which button was chosen.
	 */
	void add_radio_button(
		int id, const std::wstring& caption, bool default=false)
	{
		m_radio_buttons.push_back(button(id, caption));
		if (default)
			m_default_radio_button = id;
	}

private:
	tdi_function m_task_dialog_indirect;
	HWND m_hwnd;
	std::wstring m_main_instruction;
	std::wstring m_content;
	std::wstring m_window_title;
	icon_type::type m_icon;
	bool m_use_command_links;
	button_callback m_cancellation_callback;

	/** @name  Button State */
	// @{
	TASKDIALOG_COMMON_BUTTON_FLAGS m_common_buttons; ///< Common dialog buttons
	std::vector<button> m_buttons; ///< Buttons with strings owned by us
	std::map<int, button_callback> m_callbacks; /// Map button IDs to callbacks
	std::vector<button> m_radio_buttons; ///< Radio buttons, strings owned by us
	int m_default_button;
	int m_default_radio_button;
	// @}
};

}}} // namespace winapi::gui::task_dialog

#endif
