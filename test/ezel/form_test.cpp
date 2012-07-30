/**
    @file

    Tests for forms.

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

#include <test/common_boost/helpers.hpp> // wstring output

#include <ezel/form.hpp> // test subject

#include <ezel/controls/button.hpp> // button
#include <ezel/controls/checkbox.hpp> // checkbox
#include <ezel/controls/edit.hpp> // edit
#include <ezel/controls/label.hpp> // label

#include <boost/bind.hpp> // bind
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/test/unit_test.hpp>

using ezel::form;
using ezel::controls::button;
using ezel::controls::checkbox;
using ezel::controls::edit;
using ezel::controls::label;

using boost::bind;

BOOST_AUTO_TEST_SUITE(form_tests)

namespace {

    class form1
    {
    public:
        form1() : m_form(L"my title", 30, 40, 30, 30)
        {
            m_form.on_activate().connect(
                boost::bind(&form1::test_creation_and_die, this));
            m_form.show();
        }


        void test_creation_and_die(/*bool by_mouse*/)
        {
//            BOOST_CHECK(!by_mouse);
            BOOST_CHECK_EQUAL(m_form.text(), L"my title");
            m_form.end();
        }

        form& get_form() { return m_form; }

    private:
        form m_form;
    };

    class form2
    {
    public:
        form2() : m_form(L"", 30, 40, 30, 30)
        {
            m_form.on_create().connect(
                boost::bind(&form2::test_creation_and_die, this));
            m_form.show();
        }


        bool test_creation_and_die()
        {
            BOOST_CHECK_EQUAL(m_form.text(), L"");
            m_form.end();
            return true;
        }

        form& get_form() { return m_form; }

    private:
        form m_form;
    };

    /**
     * Monitor text change event.
     */
    class form3
    {
    public:
        form3() :
            m_form(L"initial text", 30, 40, 30, 30),
            m_change_detected(false)
        {
            m_form.on_create().connect(
                boost::bind(&form3::test_and_die, this));
            m_form.on_text_changed().connect(
                boost::bind(&form3::text_changed, this));
            m_form.show();
        }

        void text_changed()
        {
            m_change_detected = true;
        }

        bool test_and_die()
        {
            BOOST_CHECK_EQUAL(m_form.text(), L"initial text");
            m_form.text(L"changed text");
            BOOST_CHECK(m_change_detected);
            BOOST_CHECK_EQUAL(m_form.text(), L"changed text");
            m_form.end();
            return true;
        }

    private:
        form m_form;
        bool m_change_detected;
    };
}

/**
 * Create a form and test some basic properties.  Then destroy it and test
 * them again.
 */
BOOST_AUTO_TEST_CASE( create_form )
{
    form1 frm;
    BOOST_CHECK_EQUAL(frm.get_form().text(), L"my title");
}

/**
 * Create a form with an empty title.
 */
BOOST_AUTO_TEST_CASE( create_form_no_title )
{
    form2 frm;
    BOOST_CHECK_EQUAL(frm.get_form().text(), L"");
}

/**
 * Test that we can react to changes in form properties.
 * In other words, test that events work for forms.
 */
BOOST_AUTO_TEST_CASE( create_form_change_title )
{
    form3 frm;
}

/**
 * Put a button on a form.
 */
BOOST_AUTO_TEST_CASE( form_with_button )
{
    form frm(L"my title", 30, 40, 100, 50);
    
    button hello(L"Hello", 0, 0, 30, 20);
    hello.on_click().connect(frm.killer());
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
    form frm(L"Pick one", 30, 40, 200, 50);
    
    button hello(L"Oh noes!", 10, 10, 50, 20, true);
    hello.on_click().connect(frm.killer());

    button parp(L"Parp", 70, 10, 50, 20);
    parp.on_click().connect(beep);

    frm.add_control(hello);
    frm.add_control(parp);
    frm.show();
}

/**
 * Put two different controls on a form.
 */
BOOST_AUTO_TEST_CASE( form_with_different_controls )
{
    form frm(L"A button and a box went to tea", 30, 40, 200, 50);
    
    button hello(L"Hello", 10, 10, 30, 20, true);
    hello.on_click().connect(frm.killer());
    frm.add_control(hello);

    edit text_box(L"Some text", 70, 10, 70, 14, edit::style::default);
    text_box.on_update().connect(beep);
    frm.add_control(text_box);

    frm.show();
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
        form frm(L"You'll see me four times", 30, 40, 200, 50);
        
        button hello(title, 10, 10, 60, 20);
        hello.on_click().connect(frm.killer());

        label lab(L"press the button to exit", 70, 10, 50, 20);
        frm.add_control(hello);
        frm.add_control(lab);
        frm.show();
    }
}

/**
 * Put a button on a form using inline temporary construction.
 *
 * The add_control method should copy the new button in such a way that
 * it works once the temporary is destroyed.
 */
BOOST_AUTO_TEST_CASE( form_with_button_inline_contructor )
{
    form frm(L"my title", 30, 40, 100, 50);
    
    button close(L"Close", 40, 25, 60, 20, true);
    close.on_click().connect(frm.killer());
    frm.add_control(close);
    
    frm.add_control(button(L"I do nothing", 0, 0, 75, 20));

    frm.show();
}

/**
 * Link two controls.
 */
BOOST_AUTO_TEST_CASE( one_control_updates_another )
{
    form frm(L"Multipass", 30, 40, 220, 50);
    
    button close(L"Close", 10, 10, 30, 20);
    close.on_click().connect(frm.killer());

    label lab(L"My old text", 160, 15, 50, 20);
    button change(L"Click me to change him", 50, 10, 100, 20, true);
    change.on_click().connect(
        bind(&label::text, boost::ref(lab), L"I got new!"));

    frm.add_control(change);
    frm.add_control(close);
    frm.add_control(lab);

    frm.show();

    BOOST_CHECK_EQUAL(lab.text(), L"I got new!");
}

/**
 * Chain two events (beep end end).
 */
BOOST_AUTO_TEST_CASE( chain_events )
{
    form frm(L"I should beep then die", 30, 40, 100, 50);
    
    button ping(L"Ping!", 0, 0, 100, 50);

    ping.on_click().connect(beep);
    ping.on_click().connect(frm.killer());

    frm.add_control(ping);

    frm.show();
}

BOOST_AUTO_TEST_SUITE_END();
