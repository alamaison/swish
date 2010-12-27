/**
    @file

	Manage remote directory as a collection of PIDLs.

    @if license

    Copyright (C) 2007, 2008, 2009, 2010
    Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#pragma once

#include "HostPidl.h"  // PIDL wrapper classes
#include "RemotePidl.h"

#include <comet/bstr.h> // bstr_t
#include <comet/ptr.h> // com_ptr

struct ISftpConsumer;
struct ISftpProvider;

class CSftpDirectory
{
public:
	CSftpDirectory(
		CAbsolutePidlHandle pidlDirectory,
		comet::com_ptr<ISftpProvider> provider,
		comet::com_ptr<ISftpConsumer> consumer);

	ATL::CComPtr<IEnumIDList> GetEnum(SHCONTF grfFlags);
	CSftpDirectory GetSubdirectory(__in CRemoteItemHandle pidl);
	ATL::CComPtr<IStream> GetFile(
		__in CRemoteItemHandle pidl, bool writeable);
	ATL::CComPtr<IStream> GetFileByPath(
		PCWSTR pwszPath, bool writeable);

	bool Rename(
		__in CRemoteItemHandle pidlOldFile, __in PCWSTR pwszNewFilename);
	void Delete(__in CRemoteItemHandle pidl);

private:
	comet::com_ptr<ISftpProvider> m_provider;  ///< Backend data provider
	comet::com_ptr<ISftpConsumer> m_consumer;  ///< UI callback
	comet::bstr_t m_directory;        ///< Absolute path to this directory.
	CAbsolutePidl m_pidlDirectory;    ///< Absolute PIDL to this directory.
};
