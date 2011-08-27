/**
    @file

    Translation of C++ exceptions for ISftpProvider interface.

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.

    @endif
*/

#ifndef SWISH_PROVIDER_PROVIDER_ERROR_ADAPTER_HPP
#define SWISH_PROVIDER_PROVIDER_ERROR_ADAPTER_HPP
#pragma once

#include "swish/interfaces/SftpProvider.h" // ISftpProvider

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <comet/error.h> // com_error

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert

namespace swish {
namespace provider {

class provider_interface
{
public:
	virtual void initialize(BSTR user, BSTR host, UINT port) = 0;

	virtual IEnumListing* get_listing(
		ISftpConsumer* consumer, BSTR directory) = 0;

	virtual IStream* get_file(
		ISftpConsumer* consumer, BSTR file_path, BOOL writeable) = 0;

	virtual VARIANT_BOOL rename(
		ISftpConsumer* consumer, BSTR from_path, BSTR to_path) = 0;

	virtual void delete_file(ISftpConsumer* consumer, BSTR path) = 0;

	virtual void delete_directory(ISftpConsumer* consumer, BSTR path) = 0;

	virtual void create_new_file(ISftpConsumer* consumer, BSTR path) = 0;

	virtual void create_new_directory(ISftpConsumer* consumer, BSTR path) = 0;

	virtual BSTR resolve_link(ISftpConsumer* consumer, BSTR link_path) = 0;

	virtual Listing stat(
		ISftpConsumer* consumer, BSTR path, BOOL follow_links) = 0;
};

/**
 * @c Abstract ISftpProvider outer layer that translates exceptions.
 *
 * Subclass this adapter and implement @c provider_interface to get a
 * COM component supporting @c ISftpProvider.
 *
 * This adapter class handles the translation of C++ exceptions to COM error
 * codes.  It has a public COM interface which it implements and subclasses
 * provide an implementation of provider_interface to which calls are delegated.
 * The provider_interface methods are free to throw any exceptions derived
 * from std::exception as well as any that you handle using your own
 * @c comet_exception_handler override.  This class catches those exceptions
 * translates them to COM error codes and IError info and sets output
 * parameters as appropriate.
 *
 * The adapter ensures that the final COM object obeys COM rules in several
 * ways:
 *
 * - On entry to a COM method it first clears any [out]-parameters.  This is
 *   required by COM rules so that, for example, cross-apartment marshalling
 *   doesn't try to marshal uninitialised memory (see item 19 of 
 *   'Effective Com').
 * - If certain required parameters are missing, it immediately returns a
 *   COM error without calling the inner C++ method.
 * - Then it calls the inner method.
 * - Catch any exception, calling @c SetErrorInfo with as much detail as
 *   we have available, and translate it to a COM @c HRESULT.
 * - Return the HRESULT or set the [out]-params if the inner function didn't
 *   throw.
 *
 * As the return-values are no longer being used for error codes, the inner
 * methods are sometimes changed to return a value directly instead of using
 * an [out]-parameter.
 *
 * Although the adapters make use of Comet, they do not have to be instantiated
 * as Comet objects.  They work just as well using ATL::CComObject.
 *
 * Only error translation code should be included in this class.  Any
 * further C++erisation such as translating to C++ datatypes must be done in
 * by the subclasses.
 */
class provider_error_adapter : public ISftpProvider
{
public:
	typedef ISftpProvider interface_is;

	virtual IFACEMETHODIMP Initialize(
		BSTR bstrUser, BSTR bstrHost, UINT uPort)
	{
		try
		{
			impl().initialize(bstrUser, bstrHost, uPort);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP GetListing(
		ISftpConsumer* pConsumer, BSTR bstrDirectory, IEnumListing** ppEnum)
	{
		try
		{
			if (!ppEnum)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*ppEnum = NULL;

			if (!pConsumer)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			*ppEnum = impl().get_listing(pConsumer, bstrDirectory);

			assert(*ppEnum || !"No error but no retval");
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP GetFile(
		ISftpConsumer* pConsumer, BSTR bstrFilePath, BOOL fWriteable,
		IStream** ppStream)
	{
		try
		{
			if (!ppStream)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*ppStream = NULL;

			if (!pConsumer)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			*ppStream = impl().get_file(pConsumer, bstrFilePath, fWriteable);

			assert(*ppStream || !"No error but no retval");
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP Rename(
		ISftpConsumer* pConsumer, BSTR bstrFromPath, BSTR bstrToPath,
		VARIANT_BOOL* pfWasTargetOverwritten)
	{
		try
		{
			if (!pfWasTargetOverwritten)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*pfWasTargetOverwritten = VARIANT_FALSE;

			if (!pConsumer)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			*pfWasTargetOverwritten = impl().rename(
				pConsumer, bstrFromPath, bstrToPath);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP Delete(ISftpConsumer* pConsumer, BSTR bstrPath)
	{
		try
		{
			if (!pConsumer)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			impl().delete_file(pConsumer, bstrPath);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP DeleteDirectory(
		ISftpConsumer* pConsumer, BSTR bstrPath)
	{
		try
		{
			if (!pConsumer)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			impl().delete_directory(pConsumer, bstrPath);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP CreateNewFile(
		ISftpConsumer* pConsumer, BSTR bstrPath)
	{
		try
		{
			if (!pConsumer)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			impl().create_new_file(pConsumer, bstrPath);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP CreateNewDirectory(
		ISftpConsumer* pConsumer, BSTR bstrPath)
	{
		try
		{
			if (!pConsumer)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			impl().create_new_directory(pConsumer, bstrPath);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

	virtual IFACEMETHODIMP ResolveLink(
		ISftpConsumer* pConsumer, BSTR bstrLinkPath, BSTR* pbstrTargetPathOut)
	{
		try
		{
			if (!pbstrTargetPathOut)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*pbstrTargetPathOut = NULL;

			if (!pConsumer)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			*pbstrTargetPathOut = impl().resolve_link(pConsumer, bstrLinkPath);

			assert(*pbstrTargetPathOut || !"No error but no retval");
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}
	
	virtual IFACEMETHODIMP Stat(
		ISftpConsumer *pConsumer, BSTR bstrPath, BOOL fFollowLinks,
		struct Listing *pplt)
	{
		try
		{
			if (!pplt)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));
			*pplt = Listing();

			if (!pConsumer)
				BOOST_THROW_EXCEPTION(comet::com_error(E_POINTER));

			*pplt = impl().stat(pConsumer, bstrPath, fFollowLinks);
		}
		WINAPI_COM_CATCH_AUTO_INTERFACE();

		return S_OK;
	}

private:

	virtual provider_interface& impl() = 0;
};

}} // namespace swish::provider

#endif
