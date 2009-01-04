/*  DataObject creating FILE_DESCRIPTOR/FILE_CONTENTS formats from remote data.

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
*/

#pragma once
#include "DataObject.h"

/**
 * Subclass of CDataObject with responsibily for creating CFSTR_FILEDESCRIPTOR
 * and CFSTR_FILECONTENTS from remote data.
 *
 * This class creates the CFSTR_FILEDESCRIPTOR HGLOBAL data (stored in the inner
 * IDataObject) and the CFSTR_FILECONTENTS IStreams (stored locally in this 
 * object) on-demand from the list of PIDLs passed to Initialize().  This is
 * called delay-rendering.
 *
 * As these operations are expensive---they require the DataObject to contact
 * the remote server via an ISftpProvider to retrieve file data---and may not
 * be needed if the client simply wants, say, a CFSTR_SHELLIDLIST format,
 * delay-rendering is employed to postpone this expence untill we are sure
 * it is required (GetData() is called with for one of the two formats.
 */
class CSftpDataObject :
	public CDataObject,
	private CCoFactory<CSftpDataObject>
{
public:

	BEGIN_COM_MAP(CSftpDataObject)
		COM_INTERFACE_ENTRY(IUnknown)
		COM_INTERFACE_ENTRY_CHAIN(CDataObject)
	END_COM_MAP()

	using CCoFactory<CSftpDataObject>::CreateCoObject;

	static CComPtr<IDataObject> Create(
		UINT cPidl, __in_ecount_opt(cPidl) PCUITEMID_CHILD_ARRAY aPidl,
		__in PCIDLIST_ABSOLUTE pidlCommonParent, __in CConnection& conn)
	throw(...)
	{
		CComPtr<CSftpDataObject> spObject = spObject->CreateCoObject();
		
		spObject->Initialize(cPidl, aPidl, pidlCommonParent, conn);

		return spObject.p;
	}

	CSftpDataObject();
	virtual ~CSftpDataObject();

	void Initialize(
		UINT cPidl, __in_ecount_opt(cPidl) PCUITEMID_CHILD_ARRAY aPidl,
		__in PCIDLIST_ABSOLUTE pidlCommonParent, __in CConnection& conn)
	throw(...);

public: // IDataObject methods

	IFACEMETHODIMP GetData( 
		__in FORMATETC *pformatetcIn,
		__out STGMEDIUM *pmedium);
	
private:
	CConnection m_conn;

	void _DelayRenderCfFileGroupDescriptor() throw(...);
	void _DelayRenderCfFileContents() throw(...);

	typedef vector< CComPtr<IStream> > StreamList; // Contiguous indices
	StreamList _CreateFileContentsStreams(
		const vector<CRelativePidl>& vecPidls) throw(...);

	static CFileGroupDescriptor s_CreateFileGroupDescriptor(
		vector<CRelativePidl> vecPidls) throw(...);
};
