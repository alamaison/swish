/*  DataObject creating FILE_DESCRIPTOR/FILE_CONTENTS formats from remote data.

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include "data_object/FileGroupDescriptor.hpp"  // FileGroupDescriptor
#include "RemotePidl.h"

#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer

#include <comet/ptr.h> // com_ptr

#include <vector>

/**
 * Subclass of CDataObject which, additionally, creates CFSTR_FILEDESCRIPTOR
 * and CFSTR_FILECONTENTS from remote data on demand.
 *
 * This class creates the CFSTR_FILEDESCRIPTOR HGLOBAL data and delegates its
 * storage to the superclass (which will, in turn, delegate it to the inner
 * object provided by the system).  
 *
 * This class also creates CFSTR_FILECONTENTS data (as IStreams) as they are 
 * requested.  Although the superclass (CDataObject) can---as with the file
 * group descriptor---store these for later, we no longer use this as doing 
 * so keeps a file-handle open to every file ever requested. This would
 * cause a large transfer to fail part way through.  Instead, we create the
 * IStreams afresh on every request.  These file-handles will close when the
 * client Releases the IStream.
 *
 * These operations are expensive---they require the DataObject to contact
 * the remote server via an ISftpProvider to retrieve file data---and may not
 * be needed if the client simply wants, say, a CFSTR_SHELLIDLIST format,
 * so delay-rendering is employed to postpone this expense until we are sure
 * it is required (GetData() is called with for one of the two formats).
 *
 * If the CFSTR_FILEDESCRIPTOR format is requested and any of the initial
 * PIDLs are directories, the PIDLs are expanded to include every item
 * anywhere with those directory trees.  Unfortunately, this is a @b very
 * expensive operation but the shell design doesn't give any way to provide
 * a partial file group descriptor.
 */
class CSftpDataObject :
	public CDataObject,
	private swish::CCoFactory<CSftpDataObject>
{
public:

	BEGIN_COM_MAP(CSftpDataObject)
		COM_INTERFACE_ENTRY(IUnknown)
		COM_INTERFACE_ENTRY_CHAIN(CDataObject)
	END_COM_MAP()

	using swish::CCoFactory<CSftpDataObject>::CreateCoObject;

	static ATL::CComPtr<IDataObject> Create(
		UINT cPidl, __in_ecount_opt(cPidl) PCUITEMID_CHILD_ARRAY aPidl,
		__in PCIDLIST_ABSOLUTE pidlCommonParent,
		__in ISftpProvider *pProvider, __in ISftpConsumer* pConsumer)
	throw(...)
	{
		ATL::CComPtr<CSftpDataObject> spObject = spObject->CreateCoObject();
		
		spObject->Initialize(
			cPidl, aPidl, pidlCommonParent, pProvider, pConsumer);

		return spObject.p;
	}

	CSftpDataObject();
	virtual ~CSftpDataObject();

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	HRESULT FinalConstruct();

	void Initialize(
		UINT cPidl, __in_ecount_opt(cPidl) PCUITEMID_CHILD_ARRAY aPidl,
		__in PCIDLIST_ABSOLUTE pidlCommonParent,
		__in ISftpProvider *pProvider, __in ISftpConsumer *pConsumer)
	throw(...);

public: // IDataObject methods

	IFACEMETHODIMP GetData( 
		__in FORMATETC *pformatetcIn,
		__out STGMEDIUM *pmedium);
	
private:
	/**
	 * Top-level PIDL types. These represent currently-selected items
	 * and will always be single-level children of m_pidlCommonParent.
	 */
	// @{
	typedef CRemoteItem TopLevelPidl;
	typedef std::vector<TopLevelPidl> TopLevelList;
	// @}

	/**
	 * Expanded types.  The are the types that the top-level PIDLs are expanded
	 * into when a file group descriptor is requested.  They can represent all
	 * the items in or below the top-level and are needed in order to store 
	 * entire directory trees in an IDataObject.
	 */
	// @{
	typedef swish::shell_folder::data_object::Descriptor ExpandedItem;
	typedef std::vector<ExpandedItem> ExpandedList;
	// @}

	ATL::CComPtr<ISftpProvider> m_spProvider; ///< Connection to backend
	ATL::CComPtr<ISftpConsumer> m_spConsumer; ///< UI callback

	/** @name Cached PIDLs */
	// @{
	CAbsolutePidl m_pidlCommonParent;    ///< Parent of PIDLs in m_pidls
	TopLevelList m_pidls;                ///< Top-level PIDLs (the selection)
	// @}

	/** @name Registered CLIPFORMATS */
	// @{
	CLIPFORMAT m_cfPreferredDropEffect;  ///< CFSTR_PREFERREDDROPEFFECT
	CLIPFORMAT m_cfFileDescriptor;       ///< CFSTR_FILEDESCRIPTOR
	CLIPFORMAT m_cfFileContents;         ///< CFSTR_FILECONTENTS
	// @}

	void _RenderCfPreferredDropEffect() throw(...);

	/** @name Delay-rendering */
	//@{
	bool m_fExpandedPidlList;         ///< Have we expanded top-level PIDLs?
	bool m_fRenderedDescriptor;       ///< Have we rendered FileGroupDescriptor

	void _DelayRenderCfFileGroupDescriptor() throw(...);
	STGMEDIUM _DelayRenderCfFileContents(long lindex) throw(...);

	HGLOBAL _CreateFileGroupDescriptor();
	comet::com_ptr<IStream> _CreateFileContentsStream(long lindex) throw(...);

	void _ExpandPidlsInto(__inout ExpandedList& descriptors) const throw(...);
	void _ExpandTopLevelPidlInto(
		const TopLevelPidl& pidl, __inout ExpandedList& descriptors)
		const throw(...);
	void _ExpandDirectoryTreeInto(
		const CAbsolutePidl& pidlParent, const CRelativePidl& pidlDirectory,
		__inout ExpandedList& descriptors) const throw(...);
	comet::com_ptr<IEnumIDList> _GetEnumAll(const CAbsolutePidl& pidl)
		const throw(...);
	inline bool _WantProgressDialogue() const throw();
	// @}
};
