/**
    @file

    Wrap a data object to show errors to user.

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

#ifndef SWISH_SHELL_FOLDER_SNITCHING_DATA_OBJECT_HPP
#define SWISH_SHELL_FOLDER_SNITCHING_DATA_OBJECT_HPP
#pragma once

#include "swish/frontend/announce_error.hpp" // rethrow_and_announce

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <comet/error.h> // com_error_from_interface
#include <comet/ptr.h> // com_ptr
#include <comet/server.h> // simple_object

#include <boost/locale.hpp> // translate
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <ObjIdl.h> // IDataObject

namespace comet {

template<> struct comtype<::IDataObject>
{
	static const ::IID& uuid() throw() { return ::IID_IDataObject; }
	typedef ::IUnknown base;
};

}

namespace swish {
namespace shell_folder {

/**
 * Layer around a data object that reports errors to the user.
 *
 * This keeps UI out of CDropTarget.  
 */
class CSnitchingDataObject : public comet::simple_object<IDataObject>
{
public:
	CSnitchingDataObject(comet::com_ptr<IDataObject> wrapped_data_object)
		: m_inner(wrapped_data_object),
		  m_file_descriptor_format_w(static_cast<CLIPFORMAT>(
		      ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW))),
		  m_file_descriptor_format_a(static_cast<CLIPFORMAT>(
		      ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA))),
		  m_file_contents_format(static_cast<CLIPFORMAT>(
		      ::RegisterClipboardFormat(CFSTR_FILECONTENTS))),
		  m_error_cycle_marker(FORMATETC())
	{
	}

public: // IDataObject methods

    virtual HRESULT STDMETHODCALLTYPE GetData( 
        FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
	{
		HRESULT hr = m_inner->GetData(pformatetcIn, pmedium);
		try
		{
			try
			{
				// Only capture the delay rendered formats.  Everything else
				// should have been caught when this data object was created
				// and for some formats an error from GetData is standard
				// operating procedure, not something that we should report.
				if (pformatetcIn->cfFormat == m_file_descriptor_format_w ||
//					pformatetcIn->cfFormat == m_file_descriptor_format_a ||
					pformatetcIn->cfFormat == m_file_contents_format)
				{
					if (FAILED(hr))
						BOOST_THROW_EXCEPTION(
							comet::com_error_from_interface(m_inner, hr));
				}
			}
			catch (...)
			{
				// DV_E_FORMATETC is used when we might have the data, just
				// not in the requested format.  It should not be reported.
				if (hr == DV_E_FORMATETC)
					throw;

				// HACK:
				// The shell asks for different versions of the same format
				// (such as CFSTR_FILEDESCRIPTORA/CFSTR_FILEDESCRIPTORW) and
				// different DVASPECTs.  As one fails it tries the next.
				// However, we only want to report the error once
				// so we record what the first failing case was and won't
				// show the error message again unless we see that exact format
				// requested again.
				// The theory being that the calling code is not going to try
				// a format again that we already said no to unless the user
				// initiated the operation again in which case we *do* want to
				// show the error message again.
				// Yes, this is a hack; a different sequence of format requests
				// might cause some weird behaviour.  However, it we mustn't
				// display the error message repeatedly and this approach is a
				// slight improvement on showing the message strictly once only.
				if (m_error_cycle_marker.cfFormat == 0)
				{
					m_error_cycle_marker = *pformatetcIn;
				}
				else if (
					pformatetcIn->cfFormat != m_error_cycle_marker.cfFormat ||
					pformatetcIn->dwAspect != m_error_cycle_marker.dwAspect ||
					pformatetcIn->lindex != m_error_cycle_marker.lindex ||
					pformatetcIn->ptd != m_error_cycle_marker.ptd ||
					pformatetcIn->tymed != m_error_cycle_marker.tymed)
				{
					throw;
				}

				// HACK HACK HACK:
				// Yes, we are creating a dialogue here even though we don't know
				// if UI is even allowed.  Yes, our UI won't have a proper
				// parent window.  Yes, it is disgusting.  No, there doesn't
				// seem to be an alternative if we want to report
				// a drag-and-drop error to the user.
				// The shell doesn't give us an HWND when creating this data object.
				// It doesn't do anything with IObjectWithSite while using this
				// data object.  SFVM_DIDDRAGDROP is only called is the
				// drag-and-drop *succeeded*.
				// I'm out of options.  Let's just hope the shell doesn't often
				// need no-UI drag-and-drop.
				swish::frontend::rethrow_and_announce(
					NULL,
					boost::locale::translate("Unable to access the item"),
					boost::locale::translate("You might not have permission."),
					true);
			}
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();
		return hr;
	}
    
    virtual HRESULT STDMETHODCALLTYPE GetDataHere( 
        FORMATETC *pformatetc, STGMEDIUM *pmedium)
	{
		return m_inner->GetDataHere(pformatetc, pmedium);
	}
    
    virtual HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC *pformatetc)
	{
		return m_inner->QueryGetData(pformatetc);
	}
    
    virtual HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc( 
        FORMATETC *pformatectIn, FORMATETC *pformatetcOut)
	{
		return m_inner->GetCanonicalFormatEtc(pformatectIn, pformatetcOut);
	}
    
    virtual HRESULT STDMETHODCALLTYPE SetData( 
        FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
	{
		return m_inner->SetData(pformatetc, pmedium, fRelease);
	}
    
    virtual HRESULT STDMETHODCALLTYPE EnumFormatEtc( 
        DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
	{
		return m_inner->EnumFormatEtc(dwDirection, ppenumFormatEtc);
	}
    
    virtual HRESULT STDMETHODCALLTYPE DAdvise( 
        FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink,
        DWORD *pdwConnection)
	{
		return m_inner->DAdvise(pformatetc, advf, pAdvSink, pdwConnection);
	}
    
    virtual HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection)
	{
		return m_inner->DUnadvise(dwConnection);
	}
    
    virtual HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
	{
		return m_inner->EnumDAdvise(ppenumAdvise);
	}

private:
	comet::com_ptr<IDataObject> m_inner;

	/** @name Registered CLIPFORMATS */
	// @{
	CLIPFORMAT m_file_descriptor_format_w;     ///< CFSTR_FILEDESCRIPTORW
	CLIPFORMAT m_file_descriptor_format_a;     ///< CFSTR_FILEDESCRIPTORA
	CLIPFORMAT m_file_contents_format;         ///< CFSTR_FILECONTENTS
	// @}

	FORMATETC m_error_cycle_marker;
};

}} // namespace swish::shell_folder

#endif