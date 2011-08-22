/**
    @file

	Handler for Explorer Default Context Menu messages.

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

#include "swish/nse/default_context_menu_callback.hpp"

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH

#include <comet/error.h> // com_error
#include <comet/ptr.h> // com_ptr

#include <string>

#include <Windows.h> // HRESULT, HWND, lparam, wparam, HMENU, IDataObject

using comet::com_error;
using comet::com_ptr;

using std::string;
using std::wstring;

namespace swish {
namespace nse {

default_context_menu_callback::~default_context_menu_callback() {}

HRESULT default_context_menu_callback::operator()(
	HWND hwnd, com_ptr<IDataObject> data_object, 
	UINT menu_message_id, WPARAM wparam, LPARAM lparam)
{
	try
	{
		switch (menu_message_id)
		{
		case DFM_MERGECONTEXTMENU:
			{
				QCMINFO* info = reinterpret_cast<QCMINFO*>(lparam);
				if (info == NULL)
					BOOST_THROW_EXCEPTION(com_error(E_POINTER));

				bool also_add_default_verbs = merge_context_menu(
					hwnd, data_object, info->hmenu, info->indexMenu,
					info->idCmdFirst, info->idCmdLast,
					static_cast<UINT>(wparam));

				return (also_add_default_verbs) ? S_OK : S_FALSE;
			}
		case DFM_INVOKECOMMAND:
			{
				const wchar_t* arguments = reinterpret_cast<PCWSTR>(lparam);
				bool handled = invoke_command(
					hwnd, data_object, static_cast<UINT>(wparam),
					(arguments == NULL) ? wstring() : arguments);

				return (handled) ? S_OK : S_FALSE;
			}
		case DFM_INVOKECOMMANDEX:
			{
				DFMICS* dfmics = reinterpret_cast<DFMICS*>(lparam);
				if (dfmics == NULL)
					BOOST_THROW_EXCEPTION(com_error(E_POINTER));

				const wchar_t* arguments =
					reinterpret_cast<PCWSTR>(dfmics->lParam);

				if (dfmics->pici == NULL)
					BOOST_THROW_EXCEPTION(com_error(E_POINTER));

				bool handled = invoke_command(
					hwnd, data_object, static_cast<UINT>(wparam),
					(arguments == NULL) ? wstring() : arguments, dfmics->fMask,
					dfmics->idCmdFirst, dfmics->idDefMax, *(dfmics->pici),
#if (NTDDI_VERSION >= NTDDI_VISTA)
					dfmics->punkSite);
#else
					NULL);
#endif

				return (handled) ? S_OK : S_FALSE;
			}
		case DFM_GETVERBA:
			{
				string result;
				verb(
					hwnd, data_object, static_cast<UINT>(LOWORD(wparam)),
					result);

				UINT buffer_len = static_cast<UINT>(HIWORD(wparam));
				if ((result.size() + 1) > buffer_len)
					BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

				char* buffer = reinterpret_cast<char*>(lparam);
				if (buffer == NULL)
					BOOST_THROW_EXCEPTION(com_error(E_POINTER));

#pragma warning(push)
#pragma warning(disable: 4996)
				result.copy(buffer, buffer_len);
#pragma warning(pop)
				buffer[buffer_len - 1] = char();
				return S_OK;
			}
		case DFM_GETVERBW:
			{
				wstring result;
				verb(
					hwnd, data_object, static_cast<UINT>(LOWORD(wparam)),
					result);

				UINT buffer_len = static_cast<UINT>(HIWORD(wparam));
				if ((result.size() + 1) > buffer_len)
					BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

				wchar_t* buffer = reinterpret_cast<wchar_t*>(lparam);
				if (buffer == NULL)
					BOOST_THROW_EXCEPTION(com_error(E_POINTER));

#pragma warning(push)
#pragma warning(disable: 4996)
				result.copy(buffer, buffer_len);
#pragma warning(pop)
				buffer[buffer_len - 1] = wchar_t();
				return S_OK;
			}
		case DFM_GETDEFSTATICID:
			{
				UINT* command_id_out = reinterpret_cast<UINT*>(lparam);
				if (command_id_out == NULL)
					BOOST_THROW_EXCEPTION(com_error(E_POINTER));

				bool use_default = !default_menu_item(
					hwnd, data_object, *command_id_out);

				return (use_default) ? S_FALSE : S_OK;
			}
		default:
			return on_unknown_dfm(
				hwnd, data_object, menu_message_id, wparam, lparam);
		}
	}
	WINAPI_COM_CATCH();
}

HRESULT default_context_menu_callback::on_unknown_dfm(
	HWND /*hwnd_view*/, com_ptr<IDataObject> /*data_object*/, 
	UINT /*menu_message_id*/, WPARAM /*wparam*/, LPARAM /*lparam*/)
{
	return E_NOTIMPL; // Required for Windows 7 to show any menu at all
}

bool default_context_menu_callback::merge_context_menu(
	HWND /*hwnd_view*/, com_ptr<IDataObject> /*data_object*/, HMENU /*menu*/,
	UINT /*first_item_index*/, UINT /*minimum_id*/, UINT /*maximum_id*/,
	UINT /*allowed_changes_flags*/)
{
	return true;
}

bool default_context_menu_callback::invoke_command(
	HWND /*hwnd_view*/, com_ptr<IDataObject> /*data_object*/,
	UINT /*item_offset*/, const wstring& /*arguments*/)
{
	return false;
}

bool default_context_menu_callback::invoke_command(
	HWND /*hwnd_view*/, com_ptr<IDataObject> /*data_object*/,
	UINT /*item_offset*/, const wstring& /*arguments*/,
	DWORD /*behaviour_flags*/, UINT /*minimum_id*/, UINT /*maximum_id*/,
	const CMINVOKECOMMANDINFO& /*invocation_details*/,
	comet::com_ptr<IUnknown> /*context_menu_site*/)
{
	return false;
}
	
void default_context_menu_callback::verb(
	HWND /*hwnd_view*/, com_ptr<IDataObject> /*data_object*/, 
	UINT /*command_id_offset*/, string& /*verb_out*/)
{
}

void default_context_menu_callback::verb(
	HWND /*hwnd_view*/, com_ptr<IDataObject> /*data_object*/, 
	UINT /*command_id_offset*/, wstring& /*verb_out*/)
{
}

bool default_context_menu_callback::default_menu_item(
	HWND /*hwnd_view*/, com_ptr<IDataObject> /*data_object*/,
	UINT& /*default_command_id*/)
{
	return false;
}


}} // namespace swish::nse
