/**
    @file

    Manage remote directory as a collection of PIDLs.

    @if license

    Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012, 2013
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

#include "swish/provider/sftp_provider.hpp" // sftp_provider, ISftpConsumer
#include "swish/remote_folder/remote_pidl.hpp" // remote_itemid_view

#include <washer/shell/pidl.hpp> // apidl_t

#include <comet/ptr.h> // com_ptr

#include <ssh/path.hpp>

#include <boost/shared_ptr.hpp>

#include <string>

class CSftpDirectory
{
public:
    CSftpDirectory(
        const washer::shell::pidl::apidl_t& directory,
        boost::shared_ptr<swish::provider::sftp_provider> provider);

    comet::com_ptr<IEnumIDList> GetEnum(SHCONTF flags);
    CSftpDirectory GetSubdirectory(
        const washer::shell::pidl::cpidl_t& directory);
    comet::com_ptr<IStream> GetFile(
        const washer::shell::pidl::cpidl_t& file, bool writeable);
    comet::com_ptr<IStream> GetFileByPath(
        const ssh::filesystem::path& file, bool writeable);

    bool exists(const washer::shell::pidl::cpidl_t& file);

    bool Rename(
        const washer::shell::pidl::cpidl_t& old_file,
        const std::wstring& new_filename,
        comet::com_ptr<ISftpConsumer> consumer);
    void Delete(
        const washer::shell::pidl::cpidl_t& file);
    washer::shell::pidl::cpidl_t CreateDirectory(const std::wstring& name);
    washer::shell::pidl::apidl_t ResolveLink(
        const washer::shell::pidl::cpidl_t& item);

private:
    boost::shared_ptr<swish::provider::sftp_provider> m_provider;
                                                      ///< Backend data provider
    ssh::filesystem::path m_directory; ///< Absolute path to this directory.
    const washer::shell::pidl::apidl_t m_directory_pidl;
                                 ///< Absolute PIDL to this directory.
};
