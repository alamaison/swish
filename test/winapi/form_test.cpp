/**
    @file

    Tests for forms.

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

#include <test/common_boost/helpers.hpp> // wstring output

#include <winapi/gui/form.hpp> // test subject

#include <winapi/gui/controls/button.hpp> // button
#include <winapi/gui/controls/checkbox.hpp> // checkbox
#include <winapi/gui/controls/edit.hpp> // edit
#include <winapi/gui/controls/label.hpp> // label

#include <boost/bind.hpp> // bind
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/test/unit_test.hpp>

using winapi::gui::form;
using winapi::gui::controls::button;
using winapi::gui::controls::checkbox;
using winapi::gui::controls::edit;
using winapi::gui::controls::label;

using boost::bind;
BOOST_AUTO_TEST_SUITE(form_tests)

/**
 * Create a form without showing it.
 */
BOOST_AUTO_TEST_CASE( create_form )
{
	form frm(L"my title", 30, 30, 30, 40);
	frm.show();
	BOOST_CHECK_EQUAL(frm.text(), L"my title");
}

/**
 * Create a form with an empty title.
 */
BOOST_AUTO_TEST_CASE( create_form_no_title )
{
	form frm(L"", 30, 30, 30, 40);
	frm.show();
	BOOST_CHECK_EQUAL(frm.text(), L"");
}

/**
 * Put a button on a form.
 */
BOOST_AUTO_TEST_CASE( form_with_button )
{
	form frm(L"my title", 100, 50, 30, 40);
	
	button hello(
		L"Hello", 30, 20, 0, 0, bind(&form::end, boost::ref(frm)));
	frm.add_control(hello);
	frm.show();
	BOOST_CHECK_EQUAL(frm.text(), L"my title");
	BOOST_CHECK_EQUAL(hello.text(), L"Hello");
}

namespace {

	void beep() { ::MessageBeep(0); }

}

/**
 * Put two buttons on a form.
 */
BOOST_AUTO_TEST_CASE( form_with_two_controls )
{
	form frm(L"Pick one", 200, 50, 30, 40);
	
	button hello(
		L"Oh noes!", 50, 20, 10, 10, bind(&form::end, boost::ref(frm)));
	button parp(L"Parp", 50, 20, 70, 10, beep);
	frm.add_control(hello);
	frm.add_control(parp);
	frm.show();
}

/**
 * Put two different controls on a form.
 */
BOOST_AUTO_TEST_CASE( form_with_different_controls )
{
	form frm(L"A button and a box went to tea", 200, 50, 30, 40);
	
	button hello(
		L"Hello", 30, 20, 10, 10, bind(&form::end, boost::ref(frm)));
	edit text_box(L"Some text", 70, 14, 70, 10, false, beep);
	frm.add_control(hello);
	frm.add_control(text_box);
	//frm.show();
}

/**
 * Test that control template alignment is being done correctly.
 *
 * Change the control alignment by varying the title text by one character at
 * a time to cycle though the four alignment possibilities:
 * aligned, off-by-one, off-by-two, off-by-three (not necessarily in that
 * order).
 */
BOOST_AUTO_TEST_CASE( four_different_alignments )
{
	wchar_t* titles[] = { L"Hello", L"Helloo", L"Hellooo", L"Helloooo" };

	BOOST_FOREACH(wchar_t* title, titles)
	{
		form frm(L"You'll see me four times", 200, 50, 30, 40);
		
		button hello(
			title, 60, 20, 10, 10, bind(&form::end, boost::ref(frm)));
		label lab(L"press the button to exit", 50, 20, 70, 10);
		frm.add_control(hello);
		frm.add_control(lab);
		//frm.show();
	}
}

/**
 * Put a button on a form using inline temporary construction.
 *
 * The add_control method should copy the new button in such a way that
 * it works one the temporary is destroyed.
 */
BOOST_AUTO_TEST_CASE( form_with_button_inline_contructor )
{
	form frm(L"my title", 100, 50, 30, 40);
	frm.add_control(
		button(
			L"Hello", 30, 20, 0, 0, bind(&form::end, boost::ref(frm))));
	//frm.show();
}


/**
 * 
 */
BOOST_AUTO_TEST_CASE( one_control_updates_another )
{
	form frm(L"Multipass", 220, 50, 30, 40);
	
	button close(
		L"Close", 30, 20, 10, 10, bind(&form::end, boost::ref(frm)));
	label lab(L"My old text", 50, 20, 160, 15);
	button change(
		L"Click me to change him", 100, 20, 50, 10,
		bind(&label::text, boost::ref(lab), L"I got new!"));

	frm.add_control(close);
	frm.add_control(lab);
	frm.add_control(change);

	frm.show();

	BOOST_CHECK_EQUAL(lab.text(), L"I got new!");
}

BOOST_AUTO_TEST_SUITE_END();
