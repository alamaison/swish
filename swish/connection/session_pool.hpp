/**
    @file

    Pool of reusable SFTP connections.

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

#ifndef SWISH_CONNECTION_SESSION_POOL_HPP
#define SWISH_CONNECTION_SESSION_POOL_HPP
#pragma once

#include "swish/connection/connection_spec.hpp"
#include "swish/provider/sftp_provider.hpp"

#include <boost/shared_ptr.hpp>

#include <string>

namespace swish {
namespace connection {

/**
 * Per-process pool of sessions.
 *
 * All instances of this class share the same pool of sessions.
 */
class session_pool
{
public:

    /**
     * Returns a running SFTP session based on the given specification.
     * 
     * If an appropriate SFTP session already exists in the pool,
     * that connection is reused.  Otherwise a new one is created, and added
     * to the pool.
     */
    boost::shared_ptr<swish::provider::sftp_provider> pooled_session(
        const connection_spec& specification);
    
    /**
     * Is a connection with the given specification in the pool?
     *
     * Indicates whether the session matches one already running or whether
     * the session would need to to be created anew, should the caller decide to
     * call pooled_session().
     */
    bool has_session(const connection_spec& specification) const;

    /**
     * Remove the specified session from the pool.
     */
    void remove_session(const connection_spec& specification);

};

}} // namespace swish::connection

#endif
