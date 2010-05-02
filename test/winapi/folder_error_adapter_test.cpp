/**
    @file

    Tests for folder exception translation adapters.

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

#include <winapi/shell/folder_error_adapters.hpp> // test subject

#include <comet/bstr.h> // bstr_t
#include <comet/ptr.h> // com_ptr
#include <comet/server.h> // simple_object

#include <boost/test/unit_test.hpp>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <string>

using winapi::shell::folder_error_adapter;
using winapi::shell::folder2_error_adapter;
using winapi::shell::shell_details_error_adapter;

using comet::com_ptr;
using comet::com_error;
using comet::bstr_t;
using comet::simple_object;

using std::string;

BOOST_AUTO_TEST_SUITE(folder_error_adapter_tests)

namespace {

	/**
	 * Does a very basic IShellFolder(1) implementation compile using
	 * the error translation class folder_error_adapter?
	 */
	class error_folder : public simple_object<folder_error_adapter>
	{
	public:
		PIDLIST_RELATIVE parse_display_name(
			HWND, IBindCtx*, const wchar_t*, ULONG*)
		{ return NULL; }

		IEnumIDList* enum_objects(HWND, SHCONTF flags)
		{
			if (flags == 0)
				BOOST_THROW_EXCEPTION(std::exception("Test error message"));
			else
				BOOST_THROW_EXCEPTION(com_error("Wibble", E_NOTIMPL));
		}
		
		void bind_to_object(
			PCUIDLIST_RELATIVE, IBindCtx*, const IID&, void** interface_out)
		{ *interface_out = NULL; }

		void bind_to_storage(
			PCUIDLIST_RELATIVE, IBindCtx*, const IID&, void** interface_out)
		{ *interface_out = NULL; }

		int compare_ids(
			LPARAM, PCUIDLIST_RELATIVE, PCUIDLIST_RELATIVE)
		{ return 0; }

		void create_view_object(HWND, const IID&, void** interface_out)
		{ *interface_out = NULL; }
		
		void get_attributes_of(UINT, PCUITEMID_CHILD_ARRAY, SFGAOF*)
		{}

		void get_ui_object_of(
			HWND, UINT, PCUITEMID_CHILD_ARRAY, const IID&,
			void** interface_out)
		{ *interface_out = NULL; }

		STRRET get_display_name_of(PCUITEMID_CHILD, SHGDNF)
		{ return STRRET(); }

		PITEMID_CHILD set_name_of(HWND, PCUITEMID_CHILD, const wchar_t*,SHGDNF)
		{ return NULL; }
	};

	/**
	 * Does an IShellFolder2 implementation compile using
	 * the error translation class folder2_error_adapter?
	 */
	class error_folder2 : public simple_object<folder2_error_adapter>
	{
	public: // folder_base_interface

		PIDLIST_RELATIVE parse_display_name(
			HWND, IBindCtx*, const wchar_t*, ULONG*)
		{ return NULL; }

		IEnumIDList* enum_objects(HWND, SHCONTF)
		{ return NULL; }
		
		void bind_to_object(
			PCUIDLIST_RELATIVE, IBindCtx*, const IID&, void** interface_out)
		{ *interface_out = NULL; }

		void bind_to_storage(
			PCUIDLIST_RELATIVE, IBindCtx*, const IID&, void** interface_out)
		{ *interface_out = NULL; }

		int compare_ids(
			LPARAM, PCUIDLIST_RELATIVE, PCUIDLIST_RELATIVE)
		{ return 0; }

		void create_view_object(HWND, const IID&, void** interface_out)
		{ *interface_out = NULL; }
		
		void get_attributes_of(UINT, PCUITEMID_CHILD_ARRAY, SFGAOF*)
		{}

		void get_ui_object_of(
			HWND, UINT, PCUITEMID_CHILD_ARRAY, const IID&,
			void** interface_out)
		{ *interface_out = NULL; }

		STRRET get_display_name_of(PCUITEMID_CHILD, SHGDNF)
		{ return STRRET(); }

		PITEMID_CHILD set_name_of(
			HWND, PCUITEMID_CHILD, const wchar_t*, SHGDNF)
		{ return NULL; }

	public: // folder2_base_interface

		GUID get_default_search_guid()
		{ return GUID_NULL; }

		IEnumExtraSearch* enum_searches()
		{ return NULL; }

		void get_default_column(ULONG*, ULONG*)
		{}

		SHCOLSTATEF get_default_column_state(UINT)
		{ return 0; }

		VARIANT get_details_ex(PCUITEMID_CHILD, const SHCOLUMNID*)
		{ return VARIANT(); }

		SHELLDETAILS get_details_of(PCUITEMID_CHILD, UINT)
		{ return SHELLDETAILS(); }

		SHCOLUMNID map_column_to_scid(UINT)
		{ return SHCOLUMNID(); }
	};

	/**
	 * Does the basic IShellFolder(1) implementation still compile if we
	 * add support for IShellDetails using shell_details_error_adapter?
	 */
	class error_folder_with_shell_details :
		public simple_object<
			folder_error_adapter, shell_details_error_adapter>
	{
	public: // folder_base_interface

		PIDLIST_RELATIVE parse_display_name(
			HWND, IBindCtx*, const wchar_t*, ULONG*)
		{ return NULL; }

		IEnumIDList* enum_objects(HWND, SHCONTF)
		{ return NULL; }
		
		void bind_to_object(
			PCUIDLIST_RELATIVE, IBindCtx*, const IID&, void** interface_out)
		{ *interface_out = NULL; }

		void bind_to_storage(
			PCUIDLIST_RELATIVE, IBindCtx*, const IID&, void** interface_out)
		{ *interface_out = NULL; }

		int compare_ids(
			LPARAM, PCUIDLIST_RELATIVE, PCUIDLIST_RELATIVE)
		{ return 0; }

		void create_view_object(HWND, const IID&, void** interface_out)
		{ *interface_out = NULL; }
		
		void get_attributes_of(UINT, PCUITEMID_CHILD_ARRAY, SFGAOF*)
		{}

		void get_ui_object_of(
			HWND, UINT, PCUITEMID_CHILD_ARRAY, const IID&,
			void** interface_out)
		{ *interface_out = NULL; }

		STRRET get_display_name_of(PCUITEMID_CHILD, SHGDNF)
		{ return STRRET(); }

		PITEMID_CHILD set_name_of(
			HWND, PCUITEMID_CHILD, const wchar_t*, SHGDNF)
		{ return NULL; }

	public: // shell_details_base_interface

		SHELLDETAILS get_details_of(PCUITEMID_CHILD, UINT)
		{ return SHELLDETAILS(); }

		bool column_click(UINT) { return false; }
	};

	/**
	 * Does the IShellFolder2 implementation still compile if we
	 * add support for IShellDetails using shell_details_error_adapter
	 * despite the fact that our GetDetailsOf method clashes and technically
	 * has TWO implementations that delegate to the same virtual method!?
	 */
	class error_folder2_with_shell_details :
		public simple_object<
			folder2_error_adapter, shell_details_error_adapter>
	{
	public: // folder_base_interface

		PIDLIST_RELATIVE parse_display_name(
			HWND, IBindCtx*, const wchar_t*, ULONG*)
		{ return NULL; }

		IEnumIDList* enum_objects(HWND, SHCONTF)
		{ return NULL; }
		
		void bind_to_object(
			PCUIDLIST_RELATIVE, IBindCtx*, const IID&, void** interface_out)
		{ *interface_out = NULL; }

		void bind_to_storage(
			PCUIDLIST_RELATIVE, IBindCtx*, const IID&, void** interface_out)
		{ *interface_out = NULL; }

		int compare_ids(
			LPARAM, PCUIDLIST_RELATIVE, PCUIDLIST_RELATIVE)
		{ return 0; }

		void create_view_object(HWND, const IID&, void** interface_out)
		{ *interface_out = NULL; }
		
		void get_attributes_of(UINT, PCUITEMID_CHILD_ARRAY, SFGAOF*)
		{}

		void get_ui_object_of(
			HWND, UINT, PCUITEMID_CHILD_ARRAY, const IID&,
			void** interface_out)
		{ *interface_out = NULL; }

		STRRET get_display_name_of(PCUITEMID_CHILD, SHGDNF)
		{ return STRRET(); }

		PITEMID_CHILD set_name_of(
			HWND, PCUITEMID_CHILD, const wchar_t*, SHGDNF)
		{ return NULL; }

	public: // folder2_base_interface

		GUID get_default_search_guid()
		{ return GUID_NULL; }

		IEnumExtraSearch* enum_searches()
		{ return NULL; }

		void get_default_column(ULONG*, ULONG*)
		{}

		SHCOLSTATEF get_default_column_state(UINT)
		{ return 0; }

		VARIANT get_details_ex(PCUITEMID_CHILD, const SHCOLUMNID*)
		{ return VARIANT(); }

		// and for shell_details_base_interface
		SHELLDETAILS get_details_of(PCUITEMID_CHILD, UINT)
		{ return SHELLDETAILS(); }

		SHCOLUMNID map_column_to_scid(UINT)
		{ return SHCOLUMNID(); }

	public: // shell_details_base_interface

		bool column_click(UINT) { return false; }
	};
}

/**
 * Create IShellFolder implementation.
 */
BOOST_AUTO_TEST_CASE( create )
{
	com_ptr<IShellFolder>(new error_folder());
}

/**
 * Create IShellFolder2 implementation.
 */
BOOST_AUTO_TEST_CASE( create2 )
{
	com_ptr<IShellFolder>(new error_folder2());
}

/**
 * Test error mechanism.
 *
 * Passing a NULL pointer leads to a simple error without an explicit
 * description being thrown.  An error message can be obtained from the HRESULT
 * but that is not tested here.
 */
BOOST_AUTO_TEST_CASE( error )
{
	com_ptr<IShellFolder> fld(new error_folder());

	BOOST_CHECK_EQUAL(::SetErrorInfo(0, NULL), S_OK);
	HRESULT hr = fld->EnumObjects(0, 0, 0);

	BOOST_CHECK_EQUAL(hr, E_POINTER);

	com_ptr<IErrorInfo> ei = comet::impl::GetErrorInfo();
	bstr_t str;

	BOOST_CHECK_EQUAL(ei->GetDescription(str.out()), S_OK);
	BOOST_CHECK_EQUAL(str.s_str(), "");

	BOOST_CHECK_EQUAL(ei->GetSource(str.out()), S_OK);
	BOOST_CHECK(str.s_str().find("EnumObjects") != string::npos);
}

/**
 * Test translation of std::exception to COM error mechanism.
 */
BOOST_AUTO_TEST_CASE( error_std )
{
	com_ptr<IShellFolder> fld(new error_folder());

	BOOST_CHECK_EQUAL(::SetErrorInfo(0, NULL), S_OK);
	com_ptr<IEnumIDList> objects;
	HRESULT hr = fld->EnumObjects(0, 0, objects.out());

	BOOST_CHECK_EQUAL(hr, E_FAIL);

	com_ptr<IErrorInfo> ei = comet::impl::GetErrorInfo();
	bstr_t str;

	BOOST_CHECK_EQUAL(ei->GetDescription(str.out()), S_OK);
	BOOST_CHECK_EQUAL(str.s_str(), "Test error message");

	BOOST_CHECK_EQUAL(ei->GetSource(str.out()), S_OK);
	BOOST_CHECK(str.s_str().find("EnumObjects") != string::npos);
}

/**
 * Test throwing com_error with explicit description.
 */
BOOST_AUTO_TEST_CASE( error_description )
{
	com_ptr<IShellFolder> fld(new error_folder());

	BOOST_CHECK_EQUAL(::SetErrorInfo(0, NULL), S_OK);
	com_ptr<IEnumIDList> objects;
	HRESULT hr = fld->EnumObjects(0, 1, objects.out());

	BOOST_CHECK_EQUAL(hr, E_NOTIMPL);

	com_ptr<IErrorInfo> ei = comet::impl::GetErrorInfo();
	bstr_t str;

	BOOST_CHECK_EQUAL(ei->GetDescription(str.out()), S_OK);
	BOOST_CHECK_EQUAL(str.s_str(), "Wibble");

	BOOST_CHECK_EQUAL(ei->GetSource(str.out()), S_OK);
	BOOST_CHECK(str.s_str().find("EnumObjects") != string::npos);
}

/**
 * Test IShellDetails.
 */
BOOST_AUTO_TEST_CASE( column_click )
{
	com_ptr<IShellDetails> fld(new error_folder2_with_shell_details());

	HRESULT hr = fld->ColumnClick(0);
	BOOST_CHECK_EQUAL(hr, S_FALSE);
}

BOOST_AUTO_TEST_SUITE_END();
