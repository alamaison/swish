/**
    @file

	Manage remote directory as a collection of PIDLs.

    @if license

    Copyright (C) 2007, 2008, 2009, 2010, 2011
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

#include "RemotePidl.h"

#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer/SmartListing

#include <winapi/shell/pidl.hpp> // apidl_t

#include <comet/enum_iterator.h> // enum_iterator
#include <comet/ptr.h> // com_ptr

#include <boost/filesystem.hpp> // wpath

#include <string>

class CSftpDirectory
{
public:
	CSftpDirectory(
		const winapi::shell::pidl::apidl_t& directory,
		comet::com_ptr<ISftpProvider> provider,
		comet::com_ptr<ISftpConsumer> consumer);

	comet::com_ptr<IEnumIDList> GetEnum(SHCONTF flags);
	CSftpDirectory GetSubdirectory(CRemoteItemHandle directory);
	comet::com_ptr<IStream> GetFile(
		CRemoteItemHandle file, bool writeable);
	comet::com_ptr<IStream> GetFileByPath(
		const boost::filesystem::wpath& file, bool writeable);

	bool Rename(
		CRemoteItemHandle old_file, const std::wstring& new_filename);
	void Delete(CRemoteItemHandle file);
	void CreateDirectory(const std::wstring& name);

	comet::enum_iterator<IEnumListing, swish::SmartListing> begin() const;
	comet::enum_iterator<IEnumListing, swish::SmartListing> end() const;

private:
	comet::com_ptr<ISftpProvider> m_provider;  ///< Backend data provider
	comet::com_ptr<ISftpConsumer> m_consumer;  ///< UI callback
	boost::filesystem::wpath m_directory; ///< Absolute path to this directory.
	const winapi::shell::pidl::apidl_t& m_directory_pidl;
	                             ///< Absolute PIDL to this directory.
};
