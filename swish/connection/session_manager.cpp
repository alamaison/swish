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

#include "session_manager.hpp"

#include "swish/connection/session_pool.hpp"

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp> // seconds
#include <boost/function.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp> // call_once
#include <boost/uuid/random_generator.hpp>

#include <list>
#include <map>
#include <memory> // auto_ptr
#include <vector>

using comet::com_ptr;

using boost::adaptors::transformed;
using boost::bind;
using boost::call_once;
using boost::condition_variable;
using boost::function;
using boost::mutex;
using boost::noncopyable;
using boost::once_flag;
using boost::posix_time::seconds;
using boost::uuids::random_generator;
using boost::uuids::uuid;

using std::auto_ptr;
using std::list;
using std::map;
using std::string;
using std::vector;

namespace swish
{
namespace connection
{

namespace
{

class task_registration
{
public:
    // We tag the registration with a UUID because it, and its copies, must
    // be uniquely identifiable.  The task name is not enough as many tasks
    // make share a name.  We can't just use the object address, though,
    // because copies must be equal.
    task_registration(const string& task_name,
                      const connection_spec& specification)
        : m_tag(random_generator()()),
          m_task_name(task_name),
          m_specification(specification)
    {
    }

    // Copies take same tag

    bool operator==(const task_registration& other) const
    {
        return m_tag == other.m_tag;
    }

    string name() const
    {
        return m_task_name;
    }

    // So that unregistering doesn't have to search all unrelated connection's
    // tasks to find the matching task ID
    connection_spec specification() const
    {
        return m_specification;
    }

private:
    uuid m_tag;
    string m_task_name;
    connection_spec m_specification;
};
}

// Hides the implementation details from the session_manager.hpp file.
class session_reservation_impl : private boost::noncopyable
{
public:
    session_reservation_impl(authenticated_session& session,
                             const function<void()>& unreserve)
        : m_session(session), m_unreserve(unreserve)
    {
    }

    ~session_reservation_impl()
    {
        m_unreserve();
    }

    authenticated_session& session()
    {
        return m_session;
    }

private:
    authenticated_session& m_session;
    function<void()> m_unreserve;
};

namespace
{

// Purpose: to maintain the book of reservations in an orderly
// fashion.  This means cleaning out entries for old connection_specs that
// don't have any more tasks
class reservations_ledger
{
    typedef map<connection_spec, list<task_registration>> reservations_mapping;

public:
    void new_reservation(const connection_spec& specification,
                         const task_registration& task)
    {
        m_reservations[specification].push_back(task);
    }

    vector<task_registration>
    reservations_for_connection(const connection_spec& specification) const
    {
        reservations_mapping::const_iterator pos =
            m_reservations.find(specification);

        if (pos == m_reservations.end())
        {
            return vector<task_registration>();
        }
        else
        {
            return vector<task_registration>(pos->second.begin(),
                                             pos->second.end());
        }
    }

    void unreserve(const task_registration& task)
    {
        list<task_registration>& connection_registrations =
            m_reservations[task.specification()];

        connection_registrations.remove(task);

        // To stop us building up a map full of empty lists for connections
        // no longer in use, we remove the connection entry once it has
        // no more tasks
        if (connection_registrations.empty())
        {
            m_reservations.erase(m_reservations.find(task.specification()));
        }
    }

private:
    reservations_mapping m_reservations;
};

string extract_task_name(const task_registration& task)
{
    return task.name();
}

// Hides the implementation details from the session_manager.hpp file.
class session_manager_impl
{
public:
    bool has_session(const connection_spec& specification) const
    {
        return session_pool().has_session(specification);
    }

    session_reservation reserve_session(connection_spec specification,
                                        com_ptr<ISftpConsumer> consumer,
                                        const std::string& task_name)
    {
        task_registration task_id(task_name, specification);

        // Locking just before getting the session from the pool to make sure
        // another thread can't disconnect it just as we are about to become
        // first and only reservation (if there were other reservations
        // already, it couldn't get disconnected regardless)
        mutex::scoped_lock lock(m_reservations_guard);

        authenticated_session& session =
            session_pool().pooled_session(specification, consumer);

        m_reservations.new_reservation(specification, task_id);

        m_reservations_changed.notify_all();

        return session_reservation(new session_reservation_impl(
            session,
            bind(&session_manager_impl::unreserve_session, this, task_id)));
    }

    void
    disconnect_session(const connection_spec& specification,
                       session_manager::progress_callback notification_sink)
    {
        // Lock here so that no new reservations can be made once we've decided
        // to disconnect this one, until we disconnect it

        // Although we lock reservations of ALL sessions, not just this one,
        // it's not a big problem because we quickly unlock them if waiting
        // for tasks to unreserve this one.  If not waiting for tasks,
        // disconnecting the session is quick so also not a problem in practice.
        mutex::scoped_lock lock(m_reservations_guard);

        bool proceed_with_disconnection =
            wait_for_remaining_uses(specification, notification_sink, lock);

        if (proceed_with_disconnection)
        {
            session_pool().remove_session(specification);
        }
    }

private:
    bool wait_for_remaining_uses(
        const connection_spec& specification,
        session_manager::progress_callback notification_sink,
        mutex::scoped_lock& lock)
    {
        while (true)
        {
            vector<task_registration> reservations =
                m_reservations.reservations_for_connection(specification);

            if (reservations.empty())
            {
                // We notify the callback that tasks have completed so it can
                // shut down any progress UI.
                // Ideally, we would use a separate no-argument overload for
                // this, but that requires some way to overload
                // boost::functions. Basically, we need full type erasure
                notification_sink(vector<string>());
                return true;
            }
            // The callback controls whether we continue waiting or whether
            // we abort so that the user's UI isn't blocked
            else if (notification_sink(reservations |
                                       transformed(extract_task_name)))
            {
                // It is important to use a timed wait because we need to
                // respond to cancellation promptly.
                // If we used a regular wait we would only consult the user
                // callback, and notice that the user had cancelled, when the
                // number of tasks waiting changed.  This may be infrequent.
                // For a single long-running task, that would be the same as
                // preventing the user cancelling at all.

                // It is important that we wait using a lock on the same mutex
                // as the thread changing the reservations.  If there is only
                // one reservation and it goes away because its task completes
                // on another thread, that thread must not be able to try
                // and notify us of the change between where we check for
                // empty reservations (above) and where we wait for empty
                // reservations (below).  If that could happen, the wait would
                // have missed the final notification of end-of-reservations.
                // (see http://stackoverflow.com/a/6924160/67013).
                //
                // It's not a fatal problem, because the wait uses a
                // timeout, but we should still avoid it.

                m_reservations_changed.timed_wait(
                    lock, boost::posix_time::seconds(3));
            }
            else
            {
                return false;
            }
        }
    }

    // Used by session_registration to unregister the session when that
    // ticket object goes out of scope
    void unreserve_session(const task_registration& task_id)
    {
        mutex::scoped_lock lock(m_reservations_guard);

        m_reservations.unreserve(task_id);
    }

    session_manager_impl(){};

    mutex m_reservations_guard;
    reservations_ledger m_reservations;
    condition_variable m_reservations_changed;

public:
    static session_manager_impl& get()
    {
        call_once(m_initialise_once, do_init);
        return *m_instance;
    }

    static void do_init()
    {
        m_instance.reset(new session_manager_impl());
    }

    static once_flag m_initialise_once;
    static auto_ptr<session_manager_impl> m_instance;
};

once_flag session_manager_impl::m_initialise_once;
auto_ptr<session_manager_impl> session_manager_impl::m_instance;
}

session_reservation::session_reservation(session_reservation_impl* pimpl)
    : m_pimpl(pimpl)
{
}

session_reservation::~session_reservation() = default;
session_reservation::session_reservation(session_reservation&&) = default;
session_reservation& session_reservation::
operator=(session_reservation&&) = default;

authenticated_session& session_reservation::session()
{
    return m_pimpl->session();
}

session_reservation
session_manager::reserve_session(const connection_spec& specification,
                                 com_ptr<ISftpConsumer> consumer,
                                 const string& task_name)
{
    return session_manager_impl::get().reserve_session(specification, consumer,
                                                       task_name);
}

bool session_manager::has_session(const connection_spec& specification)
{
    return session_manager_impl::get().has_session(specification);
}

void session_manager::disconnect_session(const connection_spec& specification,
                                         progress_callback notification_sink)
{
    return session_manager_impl::get().disconnect_session(specification,
                                                          notification_sink);
}
}
}
