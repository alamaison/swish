/**
    @file

	Wrap CDropTarget to show errors to user.

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

#include "SnitchingDropTarget.hpp"

#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE
#include <winapi/gui/message_box.hpp> // message_box

#include <boost/locale.hpp> // translate
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/shared_ptr.hpp>  // shared_ptr

#include <comet/error.h> // com_error
#include <comet/ptr.h>  // com_ptr

#include <iosfwd> // wstringstream

using namespace winapi::gui::message_box;

using boost::filesystem::wpath;
using boost::locale::translate;
using boost::shared_ptr;

using comet::com_error;
using comet::com_ptr;

using std::wstringstream;

namespace swish {
namespace drop_target {

CSnitchingDropTarget::CSnitchingDropTarget(
	HWND hwnd_owner, com_ptr<ISftpProvider> provider,
	com_ptr<ISftpConsumer> consumer, const wpath& remote_path,
	shared_ptr<CopyCallback> callback)
	:
	m_inner(new CDropTarget(provider, consumer, remote_path, callback)),
	m_hwnd_owner(hwnd_owner) {}

STDMETHODIMP CSnitchingDropTarget::SetSite(IUnknown* pUnkSite)
{
	try
	{
		com_ptr<IObjectWithSite> obj = try_cast(m_inner);
		return obj->SetSite(pUnkSite);
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();

	return S_OK;
}

STDMETHODIMP CSnitchingDropTarget::GetSite(REFIID riid, void** ppvSite)
{
	try
	{
		com_ptr<IObjectWithSite> obj = try_cast(m_inner);
		return obj->GetSite(riid, ppvSite);
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();

	return S_OK;
}

STDMETHODIMP CSnitchingDropTarget::DragEnter( 
	IDataObject* pdo, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	return m_inner->DragEnter(pdo, grfKeyState, pt, pdwEffect);
}

STDMETHODIMP CSnitchingDropTarget::DragOver( 
	DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	return m_inner->DragOver(grfKeyState, pt, pdwEffect);
}

STDMETHODIMP CSnitchingDropTarget::DragLeave()
{
	return m_inner->DragLeave();
}

/**
 * Report any error encountered during Drop to the user with a GUI message.
 */
STDMETHODIMP CSnitchingDropTarget::Drop( 
	IDataObject* pdo, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	try
	{
		HRESULT hr = m_inner->Drop(pdo, grfKeyState, pt, pdwEffect);
		if (FAILED(hr))
		{
			com_error error(com_error_from_interface(m_inner, hr));
			if (m_hwnd_owner && error.hr() != E_ABORT)
			{
				wstringstream message;
				message << translate(
					"An error occurred while transferring files:");
				message << "\n\n";
				message << error.w_str();
				message_box(
					m_hwnd_owner, message.str(), L"Swish", box_type::ok,
					icon_type::error);
			}
		}

		return hr;
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();

	return S_OK;
}

}} // namespace swish::drop_target
