/**
    @file

    libssh2-based SFTP provider component.

    @if license

    Copyright (C) 2008, 2009, 2010, 2012, 2013
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

#ifndef SWISH_PROVIDER_PROVIDER_HPP
#define SWISH_PROVIDER_PROVIDER_HPP
#pragma once

#include "swish/connection/session_manager.hpp" // session_reservation
#include "swish/provider/sftp_provider.hpp"

#include <boost/move/move.hpp> // BOOST_RV_REF
#include <boost/shared_ptr.hpp> // shared_ptr

namespace swish {
namespace provider {

class provider;

class CProvider : public sftp_provider
{
public:

    explicit CProvider(
        BOOST_RV_REF(swish::connection::session_reservation) session_ticket);

    virtual directory_listing listing(const sftp_provider_path& directory);

    virtual comet::com_ptr<IStream> get_file(
        std::wstring file_path, std::ios_base::openmode open_mode);

    virtual VARIANT_BOOL rename(
        ISftpConsumer* consumer, BSTR from_path, BSTR to_path);

    virtual void remove_all(BSTR path);

    virtual void create_new_directory(BSTR path);

    virtual BSTR resolve_link(BSTR link_path);

    virtual sftp_filesystem_item stat(
        const sftp_provider_path& path, bool follow_links);

private:
    boost::shared_ptr<provider> m_provider;
};

}} // namespace swish::provider

#endif
