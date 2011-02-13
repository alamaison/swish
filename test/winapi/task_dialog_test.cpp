/**
    @file

    Tests for TaskDialog.

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

#include <winapi/gui/task_dialog.hpp> // test subject

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(task_dialog_tests)

/**
 * Create a TaskDialog without showing it.
 */
BOOST_AUTO_TEST_CASE( create )
{
	winapi::gui::task_dialog::task_dialog<void> td(
		NULL, L"Test instruction", L"Some content text\nAnd some more",
		L"Test TaskDialog");
	//td.show();
}

int throw_something() { throw std::exception("I throw if invoked"); }

/**
 * Create a TaskDialog with buttons.
 *
 * Use a callback that throws to ensure the callback isn't called unless
 * a button is clicked.
 */
BOOST_AUTO_TEST_CASE( create_with_buttons )
{
	winapi::gui::task_dialog::task_dialog<void> td(
		NULL, L"Test instruction", L"Some content text\nAnd some more",
		L"Test TaskDialog");
	td.add_button(L"Uncommon button 1", throw_something);
	td.add_button(
		winapi::gui::task_dialog::button_type::close, throw_something);
	td.add_button(
		L"Uncommon button\nWith another string underneath", throw_something);
	//td.show();
}

/**
 * Create a TaskDialog with radio buttons.
 */
BOOST_AUTO_TEST_CASE( create_with_radio_buttons )
{
	winapi::gui::task_dialog::task_dialog<void> td(
		NULL, L"Test instruction", L"Some content text\nAnd some more",
		L"Test TaskDialog");
	td.add_radio_button(27, L"Option 1");
	td.add_button(
		L"Uncommon button\nWith another string underneath", throw_something);
	td.add_radio_button(27, L"Option 2");
	//td.show();
}

/**
 * Create a TaskDialog with collapsed extended text area above fold.
 */
BOOST_AUTO_TEST_CASE( create_with_collapsed_extended_text_above )
{
	winapi::gui::task_dialog::task_dialog<> td(
		NULL, L"Test instruction", L"Some content text\nAnd some more",
		L"Test TaskDialog");
	td.extended_text(L"Detailed explanation");
	//td.show();
}

/**
 * Create a TaskDialog with expanded extended text area above fold.
 */
BOOST_AUTO_TEST_CASE( create_with_expanded_extended_text_above )
{
	winapi::gui::task_dialog::task_dialog<> td(
		NULL, L"Test instruction", L"Some content text\nAnd some more",
		L"Test TaskDialog");
	td.extended_text(
		L"Detailed explanation",
		winapi::gui::task_dialog::expansion_position::above,
		winapi::gui::task_dialog::initial_expansion_state::expanded);
	//td.show();
}

/**
 * Create a TaskDialog with collapsed extended text area below the fold.
 */
BOOST_AUTO_TEST_CASE( create_with_collapsed_extended_text_below )
{
	winapi::gui::task_dialog::task_dialog<> td(
		NULL, L"Test instruction", L"Some content text\nAnd some more",
		L"Test TaskDialog");
	td.extended_text(
		L"Detailed explanation",
		winapi::gui::task_dialog::expansion_position::below);
	//td.show();
}

/**
 * Create a TaskDialog with expanded extended text area below the fold.
 */
BOOST_AUTO_TEST_CASE( create_with_expanded_extended_text_below )
{
	winapi::gui::task_dialog::task_dialog<> td(
		NULL, L"Test instruction", L"Some content text\nAnd some more",
		L"Test TaskDialog");
	td.extended_text(
		L"Detailed explanation",
		winapi::gui::task_dialog::expansion_position::below,
		winapi::gui::task_dialog::initial_expansion_state::expanded);
	//td.show();
}

/**
 * Create a TaskDialog with custom collapsed expander text.
 */
BOOST_AUTO_TEST_CASE( create_with_custom_collapsed_expander )
{
	winapi::gui::task_dialog::task_dialog<> td(
		NULL, L"Test instruction", L"Some content text\nAnd some more",
		L"Test TaskDialog");
	td.extended_text(
		L"Detailed explanation",
		winapi::gui::task_dialog::expansion_position::default,
		winapi::gui::task_dialog::initial_expansion_state::default,
		L"Here be there &dragons");
	//td.show();
}

/**
 * Create a TaskDialog with custom expanded expander text.
 */
BOOST_AUTO_TEST_CASE( create_with_custom_expanded_expander )
{
	winapi::gui::task_dialog::task_dialog<> td(
		NULL, L"Test instruction", L"Some content text\nAnd some more",
		L"Test TaskDialog");
	td.extended_text(
		L"Detailed explanation",
		winapi::gui::task_dialog::expansion_position::default,
		winapi::gui::task_dialog::initial_expansion_state::default,
		L"", L"See! &Dragons");
	//td.show();
}

/**
 * Create a TaskDialog with both expander labels customised.
 */
BOOST_AUTO_TEST_CASE( create_with_custom_expander )
{
	winapi::gui::task_dialog::task_dialog<> td(
		NULL, L"Test instruction", L"Some content text\nAnd some more",
		L"Test TaskDialog");
	td.extended_text(
		L"Detailed explanation",
		winapi::gui::task_dialog::expansion_position::default,
		winapi::gui::task_dialog::initial_expansion_state::default,
		L"Here be there &dragons with really really really really really long tails", L"See! &Dragons");
	//td.show();
}

BOOST_AUTO_TEST_SUITE_END();
