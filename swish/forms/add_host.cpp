/**
    @file

    New host dialogue.

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

#include "add_host.hpp"

#include "swish/remotelimits.h"
#include "swish/shell_folder/host_management.hpp" // ConnectionExists

#include <ezel/controls/button.hpp> // button
#include <ezel/controls/edit.hpp> // edit
#include <ezel/controls/icon.hpp> // icon
#include <ezel/controls/label.hpp> // label
#include <ezel/controls/line.hpp> // line
#include <ezel/controls/spinner.hpp> // spinner
#include <ezel/form.hpp> // form

#include <winapi/dynamic_link.hpp> // module_handle
#include <winapi/gui/icon.hpp> // load_icon

#include <boost/bind.hpp> // bind
#include <boost/lexical_cast.hpp> // lexical_cast
#include <boost/locale.hpp> // translate
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <exception> // exception
#include <string>

using swish::host_management::ConnectionExists;

using winapi::module_handle;
using ezel::form;
using winapi::gui::hicon;
using winapi::gui::load_icon;
using namespace ezel::controls;

using boost::bad_lexical_cast;
using boost::bind;
using boost::lexical_cast;
using boost::locale::translate;

using std::wstring;

namespace swish {
namespace forms {

namespace {
	
	const int DEFAULT_PORT = 22;

	const wchar_t* FORBIDDEN_CHARS = L"@: \t\n\r\b\"'\\";
	const wchar_t* FORBIDDEN_PATH_CHARS = L"\"\t\n\r\b\\";

	const wchar_t* ICON_MODULE = L"user32.dll";
	const int ICON_ERROR = 103;
	const int ICON_INFO = 104;
	const int ICON_SIZE = 16;

	/**
	 * Host information entry dialog box.
	 *
	 * The dialog obtains SSH connection information from the user.
	 *
	 * Text fields:
	 * - "Name:" Friendly name for connection
	 * - "User:" SSH acount user name
	 * - "Host:" Remote host address/name
	 * - "Path:" Path for initial listing
	 * Numeric field:
	 * - "Port:" TCP/IP port to connect over
	 *
	 * @image html "add_host.png" "Dialogue box appearance"
	 */
	class AddHostForm
	{
	public:
		AddHostForm(HWND owner)
			:
			m_form(translate("New SFTP Connection"), 0, 0, 275, 176),
			m_cancelled(true),
			m_name_box(edit(L"", 42, 9, 222, 13)),
			m_host_box(
				edit(L"", 42, 58, 156, 13, edit::style::force_lowercase)),
			m_port_box(
				edit(L"", 228, 58, 26, 13, edit::style::only_allow_numbers)),
			m_port_spinner(
				spinner(
					254, 58, 10, 13, MIN_PORT, MAX_PORT, DEFAULT_PORT,
					spinner::style::no_thousand_separator)),
			m_user_box(edit(L"", 42, 76, 156, 13)),
			m_path_box(edit(L"", 42, 115, 222, 13)),
			m_status(
				label(
					L"", 23, 158, 105, 20,
					label::style::ampersand_not_special)),
			m_icon(icon(2,153,21,20)),
			m_ok(
				button(
					translate("Create Connection"), 132, 155, 80, 14, true))
		{
			// every time a field is changed we revalidate all the fields,
			// enable or disable the OK button and a display a status message
			// if needed
			m_form.on_change().connect(
				bind(&AddHostForm::update_validity, this));
			m_port_box.on_text_changed().connect(
				bind(&AddHostForm::update_validity, this));

			m_form.add_control(
				label(translate("#New Host#&Label:"), 12, 11, 28, 8));
			m_form.add_control(m_name_box);

			m_form.add_control(
				label(translate(
					"For example: \"Home Computer\"."), 42, 25, 228, 18));
			m_form.add_control(
				label(translate(
					"Specify the details of the computer and account you would "
					"like to connect to:"), 12, 45, 258, 18));
			
			m_form.add_control(
				label(translate("#New Host#&Host:"), 12, 60, 30, 8));
			m_form.add_control(m_host_box);

			m_form.add_control(
				label(translate("#New Host#&Port:"), 204, 60, 18, 8));
			m_form.add_control(m_port_box);
			m_form.add_control(m_port_spinner);

			m_form.add_control(
				label(translate("#New Host#&User:"), 12, 78, 56, 8));
			m_form.add_control(m_user_box);

			m_form.add_control(
				label(translate(
					"Specify the directory on the server that you would like "
					"Swish to start the connection in:"), 12, 96, 258, 18));

			m_form.add_control(
				label(translate("#New Host#P&ath:"), 12, 117, 35, 8));
			m_form.add_control(m_path_box);
			m_form.add_control(
				label(
					translate("Example: /home/yourusername"),
					42, 131, 104, 8));
			
			m_form.add_control(line(0, 147, 277));

			m_ok.on_click().connect(bind(&AddHostForm::on_ok, this));
			m_form.add_control(m_ok);

			button cancel(translate("Cancel"), 216, 155, 50, 14);
			cancel.on_click().connect(m_form.killer());
			m_form.add_control(cancel);

			m_form.add_control(m_status);
			m_form.add_control(m_icon);

			m_error = load_icon(
				module_handle(ICON_MODULE), ICON_ERROR, ICON_SIZE, ICON_SIZE);
			m_information = load_icon(
				module_handle(ICON_MODULE), ICON_INFO, ICON_SIZE, ICON_SIZE);
			
			update_validity();
			m_form.show(owner);
		}

		/** @name Accessors */
		// @{
		bool was_cancelled() const { return m_cancelled; }

		const std::wstring name() const { return m_name_box.text(); }
		const std::wstring host() const { return m_host_box.text(); }
		const std::wstring user() const { return m_user_box.text(); }
		int port() const { return lexical_cast<int>(m_port_box.text()); }
		const std::wstring path() const { return m_path_box.text(); }
		// @}

	private:		

		/** @name Field validity */
		// @{

		/**
		 * Check if the value in the dialog box Name field is valid.
		 *
		 * Criteria:
		 * - The field must not contain more than @ref MAX_LABEL_LEN
		 *   characters.
		 */
		bool is_valid_name() const
		{
			return name().length() <= MAX_LABEL_LEN;
		}

		/**
		 * Check if the value in the dialog box Host field is valid.
		 *
		 * Criteria:
		 * - The field must not contain more than @ref MAX_HOSTNAME_LEN
		 *   characters and must not contain any characters from
		 *   @ref FORBIDDEN_CHARS
		 *
		 * @todo Use a regexp to do this properly.
		 */
		bool is_valid_host() const
		{
			wstring text = host();
			return text.length() <= MAX_HOSTNAME_LEN &&
				text.find_first_of(FORBIDDEN_CHARS) == wstring::npos;
		}

		/**
		 * Check if the value in the dialog box User field is valid.
		 *
		 * Criteria:
		 * - The field must not contain more than @ref MAX_USERNAME_LEN
		 *   characters and must not contain any characters from
		 *   @ref FORBIDDEN_CHARS.
		 *
		 * @todo The validity criteria are woefully inadequate:
		 * - There are many characters that are not allowed in usernames.
		 * - Windows usernames can contain spaces.  These must be escaped.
		 */
		bool is_valid_user() const
		{
			wstring text = user();
			return text.length() <= MAX_USERNAME_LEN &&
				text.find_first_of(FORBIDDEN_CHARS) == wstring::npos;
		}

		/**
		 * Checks if the value in the dialog box Port field is valid.
		 *
		 * Criteria:
		 * - The field must contain a number between 0 and 65535
		 *   (@ref MAX_PORT).
		 */
		bool is_valid_port() const
		{
			try
			{
				return port() >= MIN_PORT && port() <= MAX_PORT;
			}
			catch (const bad_lexical_cast&) { return false; }
		}

		/**
		 * Checks if the value in the dialog box Path field is valid.
		 *
		 * Criteria:
		 * - The path field must not contain more than @ref MAX_PATH_LEN
		 *   characters and must not contain any characters from
		 *   @ref FORBIDDEN_PATH_CHARS.
		 *
		 * @todo The validity criteria are woefully inadequate:
		 * - Paths can contain almost any character.  Some will have to be
		 *   escaped.
		 */
		bool is_valid_path() const
		{
			wstring text = path();
			return text.length() <= MAX_PATH_LEN &&
				text.find_first_of(FORBIDDEN_PATH_CHARS) == wstring::npos;
		}

		bool all_fields_complete() const
		{
			return !(name().empty() || host().empty() || user().empty() ||
				path().empty());
		}

		// @}

			
		/** @name Event handlers */
		// @{

		void on_ok()
		{
			m_form.end();
			m_cancelled = false;
		}

		/**
		 * Disable the OK button if a field in the dialog is invalid.
		 *
		 * Also set the status icon and message.
		 */
		void update_validity()
		{
			bool enable_ok = false;
			bool info_icon_if_error = false;

			if (!is_valid_name())
			{
				m_status.text(
					translate(
						"The name cannot be longer than 30 characters."));
			}
			else if (!is_valid_host())
			{
				m_status.text(translate("The host name is invalid."));
			}
			else if (!is_valid_port())
			{
				m_status.text(
					translate(
						"The port is not valid (between 0 and 65535)."));
			}
			else if (!is_valid_user())
			{
				m_status.text(translate("The username is invalid."));
			}
			else if (!is_valid_path())
			{
				m_status.text(translate("The path is invalid."));
			}
			else if (ConnectionExists(name()))
			{
				m_status.text(
					translate(
						"A connection with the same label already exists. "
						"Please try another."));
			}
			else if (!all_fields_complete())
			{
				info_icon_if_error = true;
				m_status.text(translate("Complete all fields."));
			}
			else
			{
				enable_ok = true;
			}
			
			if (!enable_ok)
			{
				m_icon.change_icon(
					(info_icon_if_error) ?
						m_information.get() : m_error.get());
			}

			m_icon.visible(!enable_ok);
			m_status.visible(!enable_ok);
			m_ok.enable(enable_ok);
		}

		// @}

		form m_form;
		bool m_cancelled;
		
		/** @name GUI controls */
		// @{
		edit m_name_box;
		edit m_host_box;
		edit m_port_box;
		spinner m_port_spinner;
		edit m_user_box;
		edit m_path_box;
		label m_status; ///< Status message window
		icon m_icon;    ///< Status icon display area
		button m_ok;
		// @}

		/** @name Preloaded icons */
		// @{
		hicon m_error;       ///< Small icon displaying a red error cross
		hicon m_information; ///< Small icon displaying a blue 'i' symbol
		// @}
	};
}

/**
 * Display add host dialogue box and return the details entered by the user.
 *
 * @throws  If the user cancels the dialogue.
 */
host_info add_host(HWND owner)
{
	AddHostForm host_form(owner);

	if (host_form.was_cancelled())
		BOOST_THROW_EXCEPTION(std::exception("user cancelled form"));

	host_info info = {
		host_form.name(), host_form.host(), host_form.user(),
		host_form.port(), host_form.path() };

	return info;
}
	
}} // namespace swish::forms
