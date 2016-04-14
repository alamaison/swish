// Copyright 2013, 2016 Alexander Lamaison

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef SWISH_CONNECTION_SESSION_FACTORY_HPP
#define SWISH_CONNECTION_SESSION_FACTORY_HPP

#include "swish/connection/authenticated_session.hpp"
#include "swish/connection/connection_spec.hpp"
#include "swish/provider/sftp_provider.hpp" // ISftpConsumer

#include <comet/ptr.h>

#include <boost/function.hpp>
#include <boost/range/any_range.hpp>

#include <memory>
#include <string>

namespace swish
{
namespace connection
{

class session_reservation_impl;

/**
 * Ticket that prevents a session being disconnected.
 *
 * A caller may use a session if-and-only-if they a ticket for it.
 * Using a session without a ticket may lead to the session being
 * destroyed at an unexpected moment and is undefined behaviour.
 */
class session_reservation
{
public:
    /**
     * Returns reference to reserved session.
     *
     * Only guaranteed valid for the lifetime of this reservation (or any
     * moved-to destinations).  The reference must not be stored for use
     * after this reservation is destroyed.
     */
    authenticated_session& session();

    session_reservation(session_reservation_impl* pimpl);

    // Required because session_reservation_impl is incomplete
    // See http://stackoverflow.com/a/9954553/67013
    ~session_reservation();

    // Required because destructor is declared
    session_reservation(session_reservation&&);
    session_reservation& operator=(session_reservation&&);

private:
    std::unique_ptr<session_reservation_impl> m_pimpl;
};

// ALL Swish sessions (except in unit tests) must be created through this
// factory to register their interest so that the disconnection code knows
// which, if any, tasks are preventing disconnection.

class session_manager
{
public:
    typedef boost::any_range<std::string, boost::forward_traversal_tag,
                             std::string, std::ptrdiff_t> task_name_range;

    typedef boost::function<bool(const task_name_range&)> progress_callback;

    /**
     * Register interest in a session.
     *
     * Caller receives a ticket containing a reference to the session.  The
     * session cannot be disconnected until the ticket is destroyed so callers
     * should hold tickets for the minimum amount of time.
     *
     * The session and any objects it creates are only valid for the lifetime
     * of the ticket.  The caller must not hold a reference to the session or
     * its createes after the reservation is destroyed as call to
     * `disconnect_session` will disconnect and destroy the session.  Any
     * subsequent uses of those references would cause a crash.
     */
    session_reservation reserve_session(const connection_spec& specification,
                                        comet::com_ptr<ISftpConsumer> consumer,
                                        const std::string& task_name);

    /**
     * Is a connection with the given specification already connected?
     *
     * Indicates whether the session matches one already running or whether
     * the session would need to to be created anew, should the caller decide to
     * call `reserve_session`.
     */
    bool has_session(const connection_spec& specification);

    /**
     * Disconnect and destroy the session matching the specification.
     *
     * If tasks have reserved the session, the call will block until they
     * all give up their tickets.  The `notification_sink` will be called:
     * - initially, with the names of the pending tasks
     * - again, each time a pending task gives up its reservation
     * - with an empty range when there are no more (or never were any) pending
     *   tasks
     */
    void disconnect_session(const connection_spec& specification,
                            progress_callback notification_sink);
};
}
}

#endif
