/**
    @file

    Ending running sessions.

    @if license

    Copyright (C) 2013, 2014  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "CloseSession.hpp"

#include "swish/connection/session_manager.hpp"
#include "swish/shell_folder/data_object/ShellDataObject.hpp" // PidlFormat
#include "swish/remote_folder/pidl_connection.hpp" // connection_from_pidl

#include <winapi/gui/task_dialog.hpp>

#include <comet/ptr.h> // com_ptr
#include <comet/error.h> // com_error
#include <comet/uuid_fwd.h> // uuid_t

#include <boost/bind/bind.hpp>
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/locale.hpp> // translate
#include <boost/locale/encoding_utf.hpp> // utf_to_utf
#include <boost/noncopyable.hpp>
#include <boost/optional/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/future.hpp> // promise, packaged_task
#include <boost/thread/thread.hpp>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/utility/in_place_factory.hpp> // in_place

#include <cassert> // assert
#include <memory> // auto_ptr
#include <sstream> // wostringstream;
#include <string>
#include <utility> // pair

using swish::connection::session_manager;
using swish::nse::Command;
using swish::shell_folder::data_object::PidlFormat;
using swish::remote_folder::connection_from_pidl;

using namespace winapi::gui::task_dialog;
using winapi::shell::pidl::apidl_t;

using comet::com_error;
using comet::com_ptr;
using comet::uuid_t;

using boost::bind;
using boost::unique_future;
using boost::locale::conv::utf_to_utf;
using boost::locale::translate;
using boost::noncopyable;
using boost::optional;
using boost::packaged_task;
using boost::promise;
using boost::shared_ptr;
using boost::thread;

using std::auto_ptr;
using std::endl;
using std::make_pair;
using std::pair;
using std::string;
using std::wostringstream;
using std::wstring;

namespace swish {
namespace host_folder {
namespace commands {

namespace {
    const uuid_t CLOSE_SESSION_COMMAND_ID("b816a886-5022-11dc-9153-0090f5284f85");

    /**
     * Cause Explorer to refresh the UI view of the given item.
     */
    void notify_shell(const apidl_t item)
    {
        ::SHChangeNotify(
            SHCNE_UPDATEITEM, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT,
            item.get(), NULL);
    }
}

CloseSession::CloseSession(HWND hwnd, const apidl_t& folder_pidl) :
    Command(
        translate(L"&Close SFTP connection"), CLOSE_SESSION_COMMAND_ID,
        translate(L"Close the authenticated connection to the server."),
        L"shell32.dll,-11", translate(L"&Close SFTP Connection..."),
        translate(L"Close Connection")),
    m_hwnd(hwnd), m_folder_pidl(folder_pidl) {}

BOOST_SCOPED_ENUM(Command::state) CloseSession::state(
    const comet::com_ptr<IDataObject>& data_object, bool /*ok_to_be_slow*/)
const
{
    if (!data_object)
    {
        // Selection unknown.
        return state::hidden;
    }

    PidlFormat format(data_object);
    switch (format.pidl_count())
    {
    case 1:
        if (session_manager().has_session(connection_from_pidl(format.file(0))))
            return state::enabled;
        else
            return state::hidden;
    case 0:
        return state::hidden;
    default:
        // This means multiple items are selected. We disable rather than
        // hide the buttons to let the user know the option exists but that
        // we don't support multi-host session closure.
        return state::disabled;
    }
}

namespace {

    void start_marquee(progress_bar bar)
    {
        bar(marquee_progress());
    }

    template<typename PendingTaskRange>
    wstring ui_content_text(const PendingTaskRange& pending_tasks)
    {
        wostringstream content;
        content << translate(
            L"Explanation in progress dialog",
            L"The following tasks are using the session:");
        content << endl << endl;

        BOOST_FOREACH(const std::string& task_name, pending_tasks)
        {
            content << L"\x2022 ";
            content << utf_to_utf<wchar_t>(task_name);
            content << endl;
        }

        content << endl;

        content << translate(
            L"Explanation of why we are displaying progress dialog. "
            L"'them' refers to the tasks we are waiting for.",
            L"Waiting for them to finish.");

        return content.str();
    }

    void do_nothing_command() {}

    template<typename Result, typename Callable>
    pair<shared_ptr<unique_future<Result>>, shared_ptr<thread>>
    start_async(Callable operation)
    {
        packaged_task<Result> task(operation);
        shared_ptr<unique_future<Result>> result(
            new unique_future<Result>(boost::move(task.get_future())));

        shared_ptr<thread> running_task(new thread(boost::move(task)));

        return make_pair(result, running_task);
    }

    template<typename Result=void>
    class async_task_dialog_runner : private noncopyable
    {
    public:
        explicit async_task_dialog_runner(task_dialog_builder<Result> builder)
            :
        m_builder(builder),
        m_dialog(m_promised_dialog.get_future()),
        m_result(
            start_async<Result>(
                bind(&async_task_dialog_runner::dialog_loop<Result>, this)))
        {}

        ~async_task_dialog_runner()
        {
            // Ideally, we would use boost::async to run the dialog, which returns
            // a future whose destructor blocks until the dialog finishes.  Making
            // that future a member of this class then ensures the member variables
            // remain valid for entire lifetime of the async operation.
            //
            // However, Boost 1.49 doesn't have async() so we need to keep
            // the thread around and join in the destructor

            m_result.second->join();
        }

        task_dialog dialog()
        {
            // Dialog creation might have failed so we don't want to block here
            // on an event that may never happen.

            // FIXME: Horrible mess with a race condition: creation may fail
            // with an exception after we check for it.  The solution is to
            // rewrite the task dialog class to use futures.

            if (m_result.first->has_exception())
            {
                m_result.first->get();
            }
            
            return m_dialog.get();
        }

        unique_future<Result>& result()
        {
            return *(m_result.first);
        }

    private:

        void on_create(const task_dialog& dialog)
        {
            m_promised_dialog.set_value(dialog);
        }

        template<typename Result>
        Result dialog_loop()
        {
            return m_builder.show(
                bind(&async_task_dialog_runner::on_create, this, _1));
        }

        task_dialog_builder<Result> m_builder;

        // Class is not movable because thread holds reference to this instance
        // when bound to `dialog_loop`.  Moving it would leave moved thread
        // using `dialog_loop` with old `this`
        promise<task_dialog> m_promised_dialog;
        unique_future<task_dialog> m_dialog;
        pair<shared_ptr<unique_future<Result>>, shared_ptr<thread>> m_result;
    };

    class running_dialog
    {
    public:

        running_dialog(
            auto_ptr<async_task_dialog_runner<>> runner, command_id id)
            :
            m_dialog_runner(runner), m_id(id) {}

        task_dialog dialog()
        {
            return m_dialog_runner->dialog();
        }

        command_id dismissal_command_id()
        {
            return m_id;
        }

        bool dialog_has_been_dismissed()
        {
            return m_dialog_runner->result().has_value();
        }

    private:
        auto_ptr<async_task_dialog_runner<>> m_dialog_runner;
        command_id m_id;
    };

    template<typename PendingTaskRange>
    running_dialog run_task_dialog(const PendingTaskRange& pending_tasks)
    {
        task_dialog_builder<> builder(
            NULL, //m_parent_window,
            translate(
                L"Title of a progress dialog", L"Disconnecting session"),
            ui_content_text(pending_tasks), L"Swish", icon_type::information);

        builder.include_progress_bar(start_marquee);

        command_id id = builder.add_button(
            button_type::cancel, do_nothing_command);

        auto_ptr<async_task_dialog_runner<>> runner(
            new async_task_dialog_runner<>(builder));

        return running_dialog(runner, id);
    }

    class waiting_ui
    {
    public:
        template<typename PendingTaskRange>
        explicit waiting_ui(const PendingTaskRange& pending_tasks)
            : m_dialog(run_task_dialog(pending_tasks)) {}

        template<typename PendingTaskRange>
        bool update(const PendingTaskRange& pending_tasks)
        {
            if (boost::empty(pending_tasks))
            {
                m_dialog.dialog().invoke_command(
                    m_dialog.dismissal_command_id());
                return true;
            }
            else
            {
                m_dialog.dialog().content(ui_content_text(pending_tasks));

                return !m_dialog.dialog_has_been_dismissed();
            }
        }

    private:
        running_dialog m_dialog;
    };

    class disconnection_progress : private noncopyable
    {

    public:
        explicit disconnection_progress(HWND parent_window)
            :
        m_parent_window(parent_window)
        {}

        template<typename PendingTaskRange>
        bool operator()(const PendingTaskRange& pending_tasks)
        {
            if (!m_dialog)
            {
                // No need to start dialog if there are no tasks
                if (!boost::empty(pending_tasks))
                {
                    // Using in-place-factory because waiting_ui's copy
                    // constructor requires NON-const ref which optional
                    // assignment doesn't allow
                    m_dialog = in_place(pending_tasks);
                }

                return true;
            }
            else
            {
                return m_dialog->update(pending_tasks);
            }
        }

    private:

        HWND m_parent_window;
        optional<waiting_ui> m_dialog;
    };

};

void CloseSession::operator()(
    const com_ptr<IDataObject>& data_object, const com_ptr<IBindCtx>&)
const
{
    PidlFormat format(data_object);
    if (format.pidl_count() != 1)
        BOOST_THROW_EXCEPTION(com_error(E_FAIL));

    apidl_t pidl_selected = format.file(0);

    disconnection_progress progress(m_hwnd);;

    session_manager().disconnect_session(
        connection_from_pidl(pidl_selected), boost::ref(progress));

    notify_shell(pidl_selected);
}

}}} // namespace swish::host_folder::commands
