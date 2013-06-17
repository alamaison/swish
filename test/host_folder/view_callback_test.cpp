/**
    @file

    Tests for object that manages relationship with Explorer window.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    If you modify this Program, or any covered work, by linking or
    combining it with the OpenSSL project's OpenSSL library (or a
    modified version of that library), containing parts covered by the
    terms of the OpenSSL or SSLeay licenses, the licensors of this
    Program grant you additional permission to convey the resulting work.

    @endif
*/

#include <swish/host_folder/ViewCallback.hpp> // test subject

#include <test/common_boost/helpers.hpp> // BOOST_CHECK_OK
#include <test/common_boost/SwishPidlFixture.hpp> // swish_pidl

#include <winapi/gui/menu/button/string_button_description.hpp>
#include <winapi/gui/menu/item/command_item.hpp>
#include <winapi/gui/menu/item/separator_item.hpp>
#include <winapi/gui/menu/item/sub_menu_item.hpp>
#include <winapi/gui/menu/item/sub_menu_item_description.hpp>
#include <winapi/gui/menu/menu.hpp>

#include <comet/ptr.h> // com_ptr

#include <boost/test/unit_test.hpp>

#include <string>

using test::SwishPidlFixture;

using swish::host_folder::CViewCallback;

using namespace winapi::gui::menu;

using comet::com_ptr;

using std::wstring;

BOOST_FIXTURE_TEST_SUITE( view_callback_tests, SwishPidlFixture )

BOOST_AUTO_TEST_CASE( basic_init )
{
    com_ptr<IShellFolderViewCB> cb = new CViewCallback(swish_pidl());

    // TODO: Use real message-only window
    BOOST_CHECK_INTERFACE_OK(
        cb, cb->MessageSFVCB(SFVM_WINDOWCREATED, NULL, NULL));
}

namespace {

    sub_menu_item_description dummy_menu(wstring title, UINT id)
    {
        menu empty_menu;

        sub_menu_item_description menu(
            string_button_description(title), empty_menu);
        menu.id(id);

        return menu;
    }

    sub_menu_item_description dummy_tools_menu()
    {
        // Purposely not called "Tools" to test that code doesn't rely on it
        return dummy_menu(L"Bert", FCIDM_MENU_TOOLS);
    }

    sub_menu_item_description dummy_help_menu()
    {
        // Purposely not called "Help" to test that code doesn't rely on it
        return dummy_menu(L"Sally", FCIDM_MENU_HELP);
    }

    sub_menu_item_description dummy_file_menu()
    {
        // Purposely not called "File" to test that code doesn't rely on it
        return dummy_menu(L"Bob", FCIDM_MENU_FILE);
    }

    class counting_visitor
    {
    public:
        typedef size_t result_type;

        size_t operator()(const sub_menu_item& item)
        {
            return item.menu().size();
        }

        template<typename ItemType>
        size_t operator()(const ItemType&) { return 0; }
    };

}

BOOST_AUTO_TEST_CASE( merge_menu )
{
    com_ptr<IShellFolderViewCB> cb = new CViewCallback(swish_pidl());

    HMENU raw_menu = ::CreateMenu();
    // Bitten here by the Most Vexing Parse
    menu_bar bar((menu_handle::adopt_handle(raw_menu)));

    bar.insert(dummy_tools_menu());
    bar.insert(dummy_file_menu());
    bar.insert(dummy_help_menu());

    QCMINFO q = { raw_menu, 7, 42, 999, NULL };
    BOOST_CHECK_INTERFACE_OK(
        cb,
        cb->MessageSFVCB(SFVM_MERGEMENU, NULL, reinterpret_cast<LPARAM>(&q)));

    // Merge should have inserted some items
    size_t count = 0;
    for (menu_bar::iterator it = bar.begin(); it != bar.end(); ++it)
    {
        count += it->accept(counting_visitor());
    }

    BOOST_CHECK_GT(count, 0U);
}

BOOST_AUTO_TEST_CASE( merge_menu_no_tools )
{
    com_ptr<IShellFolderViewCB> cb = new CViewCallback(swish_pidl());

    HMENU raw_menu = ::CreateMenu();
    // Bitten here by the Most Vexing Parse
    menu_bar bar((menu_handle::adopt_handle(raw_menu)));

    bar.insert(dummy_file_menu());
    bar.insert(dummy_help_menu());

    QCMINFO q = { raw_menu, 7, 42, 999, NULL };
    BOOST_CHECK_INTERFACE_OK(
        cb,
        cb->MessageSFVCB(SFVM_MERGEMENU, NULL, reinterpret_cast<LPARAM>(&q)));
}

BOOST_AUTO_TEST_CASE( merge_menu_no_help )
{
    com_ptr<IShellFolderViewCB> cb = new CViewCallback(swish_pidl());

    HMENU raw_menu = ::CreateMenu();
    // Bitten here by the Most Vexing Parse
    menu_bar bar((menu_handle::adopt_handle(raw_menu)));

    bar.insert(dummy_file_menu());
    bar.insert(dummy_tools_menu());

    QCMINFO q = { raw_menu, 7, 42, 999, NULL };
    BOOST_CHECK_INTERFACE_OK(
        cb,
        cb->MessageSFVCB(SFVM_MERGEMENU, NULL, reinterpret_cast<LPARAM>(&q)));
}

BOOST_AUTO_TEST_SUITE_END()
