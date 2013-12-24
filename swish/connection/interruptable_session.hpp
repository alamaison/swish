/**
    @file

    A session that can die mid-way through an operation.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_CONNECTION_INTERRUPTABLE_SESSION_HPP
#define SWISH_CONNECTION_INTERRUPTABLE_SESSION_HPP

#include "swish/connection/connection_spec.hpp"

#include <ssh/session.hpp>

#include <memory> // auto_ptr

namespace swish {
namespace connection {

class interruptable_session
{
public:
    ssh::host_key hostkey() const;

    std::vector<std::string> authentication_methods(
        const std::string& username);

    bool authenticated() const;

    bool authenticate_by_password(
        const std::string& username, const std::string& password);

    template<typename ChallengeResponder>
    bool authenticate_interactively(
        const std::string& username, ChallengeResponder responder)
    {
        return m_session->authenticate_interactively(username, responder);
    }

    void authenticate_by_key_files(
        const std::string& username, const boost::filesystem::path& public_key,
        const boost::filesystem::path& private_key,
        const std::string& passphrase);

    ssh::agent_identities agent_identities();

    ssh::filesystem::sftp_filesystem connect_to_filesystem();

    /**
     * Forcibly disconnect the session.
     *
     * Causes all future uses of the object to throw exceptions.
     */
    void terminate();

private:
    std::auto_ptr<ssh::session> m_session;
};

}}

#endif