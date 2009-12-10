/**
    @file

    libssh2-based SFTP provider component.

    @if licence

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#pragma once

#include "SessionFactory.hpp"                // CSession
#include "swish/shell_folder/SftpProvider.h" // ISftpProvider/Consumer
#include "swish/ComSTLContainer.hpp"        // CComSTLContainer

#include "swish/atl.hpp"                    // Common ATL setup
#include <atlstr.h>                          // CString

#include <boost/shared_ptr.hpp>

#include <list>

namespace swish {
namespace provider {

class ATL_NO_VTABLE CProvider :
	public ISftpProvider,
	public ATL::CComObjectRoot
{
public:

	BEGIN_COM_MAP(CProvider)
		COM_INTERFACE_ENTRY(ISftpProvider)
	END_COM_MAP()

	/** @name ATL Constructors */
	// @{
	CProvider();
	DECLARE_PROTECT_FINAL_CONSTRUCT();
	HRESULT FinalConstruct();
	void FinalRelease();
	// @}

	/** @name ISftpProvider methods */
	// @{
	IFACEMETHODIMP Initialize(
		__in ISftpConsumer *pConsumer,
		__in BSTR bstrUser, __in BSTR bstrHost, UINT uPort );
	IFACEMETHODIMP SwitchConsumer(
		__in ISftpConsumer *pConsumer );
	IFACEMETHODIMP GetListing(
		__in BSTR bstrDirectory, __out IEnumListing **ppEnum );
	IFACEMETHODIMP GetFile(
		__in BSTR bstrFilePath, __in BOOL fWriteable,
		__out IStream **ppStream );
	IFACEMETHODIMP Rename(
		__in BSTR bstrFromPath, __in BSTR bstrToPath,
		__deref_out VARIANT_BOOL *pfWasTargetOverwritten );
	IFACEMETHODIMP Delete(
		__in BSTR bstrPath );
	IFACEMETHODIMP DeleteDirectory(
		__in BSTR bstrPath );
	IFACEMETHODIMP CreateNewFile(
		__in BSTR bstrPath );
	IFACEMETHODIMP CreateNewDirectory(
		__in BSTR bstrPath );
	// @}

private:
	ISftpConsumer *m_pConsumer;    ///< Callback to consuming object
	BOOL m_fInitialized;           ///< Flag if Initialize() has been called
	boost::shared_ptr<CSession> m_session; ///< SSH/SFTP session
	ATL::CString m_strUser;        ///< Holds username for remote connection
	ATL::CString m_strHost;        ///< Hold name of remote host
	UINT m_uPort;                  ///< Holds remote port to connect to

	HRESULT _Connect();
	void _Disconnect();

	ATL::CString _GetLastErrorMessage();
	ATL::CString _GetSftpErrorMessage( ULONG uError );

	HRESULT _RenameSimple( __in_z const char* szFrom, __in_z const char* szTo );
	HRESULT _RenameRetryWithOverwrite(
		__in ULONG uPreviousError,
		__in_z const char* szFrom, __in_z const char* szTo, 
		__out ATL::CString& strError );
	HRESULT _RenameAtomicOverwrite(
		__in_z const char* szFrom, __in_z const char* szTo, 
		__out ATL::CString& strError );
	HRESULT _RenameNonAtomicOverwrite(
		const char* szFrom, const char* szTo, ATL::CString& strError );

	HRESULT _Delete(
		__in_z const char *szPath, __out ATL::CString& strError );
	HRESULT _DeleteDirectory(
		__in_z const char *szPath, __out ATL::CString& strError );
	HRESULT _DeleteRecursive(
		__in_z const char *szPath, __out ATL::CString& strError );
};

typedef ATL::CComObject<swish::CComSTLContainer< std::list<Listing> > >
	CComListingHolder;

}} // namespace swish::provider

/**
 * Copy-policy for use by enumerators of Listing items.
 */
template<>
class ATL::_Copy<Listing>
{
public:
	static HRESULT copy(Listing* p1, const Listing* p2)
	{
		p1->bstrFilename = SysAllocStringLen(
			p2->bstrFilename, ::SysStringLen(p2->bstrFilename));
		p1->uPermissions = p2->uPermissions;
		p1->bstrOwner = SysAllocStringLen(
			p2->bstrOwner, ::SysStringLen(p2->bstrOwner));
		p1->bstrGroup = SysAllocStringLen(
			p2->bstrGroup, ::SysStringLen(p2->bstrGroup));
		p1->uUid = p2->uUid;
		p1->uGid = p2->uGid;
		p1->uSize = p2->uSize;
		p1->cHardLinks = p2->cHardLinks;
		p1->dateModified = p2->dateModified;
		p1->dateAccessed = p2->dateAccessed;

		return S_OK;
	}
	static void init(Listing* p)
	{
		::ZeroMemory(p, sizeof(Listing));
	}
	static void destroy(Listing* p)
	{
		::SysFreeString(p->bstrFilename);
		::SysFreeString(p->bstrOwner);
		::SysFreeString(p->bstrGroup);
		::ZeroMemory(p, sizeof(Listing));
	}
};

typedef ATL::CComEnumOnSTL<IEnumListing, &__uuidof(IEnumListing), Listing, 
	ATL::_Copy<Listing>, std::list<Listing> > CComEnumListing;
