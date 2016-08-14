// Copyright 2009, 2010, 2011, 2012, 2016 Alexander Lamaison

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

#include "openssh_fixture.hpp"

#include "swish/connection/session_pool.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/locale.hpp>
#include <boost/optional.hpp>
#include <boost/process.hpp>
#include <boost/process/mitigate.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread/thread.hpp> // this_thread

#include <algorithm>
#include <cstdlib>
#include <iomanip> // quoted
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using boost::algorithm::join;
using boost::filesystem::path;
using boost::iostreams::file_descriptor_sink;
using boost::iostreams::file_descriptor_source;
using boost::iostreams::file_descriptor_flags;
using boost::iostreams::stream;
using boost::lexical_cast;
using boost::locale::conv::from_utf;
using boost::locale::conv::to_utf;
using boost::locale::generator;
using boost::locale::util::get_system_locale;
using boost::optional;
using boost::process::child;
using boost::process::create_pipe;
using boost::process::execute;
using boost::process::initializers::bind_stderr;
using boost::process::initializers::bind_stdout;
using boost::process::initializers::run_exe;
using boost::process::search_path;
using boost::process::initializers::set_cmd_line;
using boost::process::pipe;
using boost::process::wait_for_exit;

using std::basic_ostringstream;
using std::locale;
using std::map;
using std::ostream_iterator;
using std::ostringstream;
using std::runtime_error;
using std::string;
using std::transform;
using std::vector;
using std::wstring;

namespace
{ // private

const string SSHD_PRIVATE_KEY_FILE = "fixture_dsakey";
const string SSHD_PUBLIC_KEY_FILE = "fixture_dsakey.pub";
const string SSHD_WRONG_PRIVATE_KEY_FILE = "fixture_wrong_dsakey";
const string SSHD_WRONG_PUBLIC_KEY_FILE = "fixture_wrong_dsakey.pub";

template <typename ArgType>
string prepare_argument_for_log(const ArgType& argument);

template <>
string prepare_argument_for_log(const std::string& argument)
{
    return argument;
}

template <>
string prepare_argument_for_log(const std::wstring& argument)
{
    generator gen;
    locale loc = gen(get_system_locale());
    return from_utf(argument, loc);
}

template <typename ArgSequence>
string
error_message_from_stderr(const string& command, const ArgSequence& arguments,
                          const stream<file_descriptor_source>& stderr_stream)
{
    ostringstream message;
    message << quoted(command) << " ";

    vector<string> prepped_arguments;
    transform(arguments.begin(), arguments.end(),
              back_inserter(prepped_arguments),
              prepare_argument_for_log<ArgSequence::value_type>);

    ostream_iterator<string>(message, " ");
    copy(prepped_arguments.begin(), prepped_arguments.end(),
         ostream_iterator<string>(message, " "));
    message << " failed: " << stderr_stream.rdbuf() << std::flush;

    return message.str();
}

template <typename Out, typename ArgSequence>
Out single_value_from_executable(const path& executable,
                                 const ArgSequence& arguments)
{
    pipe stdout_pipe = create_pipe();
    pipe stderr_pipe = create_pipe();
    file_descriptor_sink stdout_sink(stdout_pipe.sink,
                                     file_descriptor_flags::close_handle);
    file_descriptor_sink stderr_sink(stderr_pipe.sink,
                                     file_descriptor_flags::close_handle);
    file_descriptor_source stdout_source(stdout_pipe.source,
                                         file_descriptor_flags::close_handle);
    file_descriptor_source stderr_source(stderr_pipe.source,
                                         file_descriptor_flags::close_handle);

    stream<file_descriptor_source> stderr_stream(stderr_source);
    stream<file_descriptor_source> stdout_stream(stdout_source);

    // We have to format the arguments into a command-line ourselves here
    // because Boost.Process 0.5 doesn't handle arguments with quotes properly
    // (see https://github.com/BorisSchaeling/boost-process/issues/17)
    vector<ArgSequence::value_type> quoted_arguments;
    transform(begin(arguments), end(arguments), back_inserter(quoted_arguments),
              [](const auto& argument)
              {
                  basic_ostringstream<ArgSequence::value_type::value_type>
                      quoted_argument;
                  quoted_argument << quoted(argument);
                  return quoted_argument.str();
              });
    auto command_line = join(
        quoted_arguments,
        ArgSequence::value_type(1, ArgSequence::value_type::value_type(' ')));

    child process = execute(run_exe(executable), set_cmd_line(command_line),
                            bind_stdout(stdout_sink), bind_stderr(stderr_sink));

    Out out;
    int status = BOOST_PROCESS_EXITSTATUS(wait_for_exit(process));
    if (status == 0)
    {
        stdout_sink.close();
        stdout_stream >> out;
        return out;
    }
    else
    {
        //        BOOST_FAIL(prepare_argument_for_log(command_line));
        stderr_sink.close();
        BOOST_THROW_EXCEPTION(runtime_error(error_message_from_stderr(
            executable.string(), quoted_arguments, stderr_stream)));
    }
}

template <typename Out, typename ArgSequence>
Out single_value_from_command(const string& command,
                              const ArgSequence& arguments)
{
// Yuk.  Bad Boost.Process API doesn't provide both overloads at once
#if defined(_UNICODE) || defined(UNICODE)
    generator gen;
    locale loc = gen(get_system_locale());
    wstring wide_command = to_utf<wchar_t>(command, loc);
    path command_executable = search_path(wide_command);

    vector<wstring> wide_arguments;
    wide_arguments.push_back(wide_command);
    transform(begin(arguments), end(arguments), back_inserter(wide_arguments),
              [loc](const string& arg)
              {
                  return to_utf<wchar_t>(arg, loc);
              });
    return single_value_from_executable<Out>(command_executable,
                                             wide_arguments);
#else
    path command_executable = search_path(command);

    return single_value_from_executable<Out>(command_executable, arguments);
#endif
}

template <typename Out, typename ArgSequence>
Out single_value_from_docker_command(const ArgSequence& arguments)
{
    return single_value_from_command<Out>("docker", arguments);
}

template <typename Out, typename ArgSequence>
Out single_value_from_docker_machine_command(const ArgSequence& arguments)
{
    return single_value_from_command<Out>("docker-machine", arguments);
}

template <typename ArgSequence>
void run_docker_command(const ArgSequence& arguments)
{
    single_value_from_docker_command<string>(arguments);
}

optional<string> docker_machine_name()
{
    char* docker_machine_name = std::getenv("DOCKER_MACHINE_NAME");
    if (docker_machine_name)
    {
        return docker_machine_name;
    }
    else
    {
        return optional<string>();
    }
}
}

extern "C" {
extern void ERR_free_strings();
extern void ERR_remove_state(unsigned long);
extern void EVP_cleanup();
extern void CRYPTO_cleanup_all_ex_data();
extern void ENGINE_cleanup();
extern void CONF_modules_unload(int);
extern void CONF_modules_free();
extern void RAND_cleanup();
}

namespace
{

struct global_fixture
{
    global_fixture()
    {
        // Ensure the docker image has been built
        vector<string> build_command = {"build", "-t", "swish/openssh_server",
                                        "ssh_server"};
        run_docker_command(build_command);
    }

    ~global_fixture()
    {
        // We call this here as a bit of a hack to stop memory-leak
        // detection incorrectly detecting OpenSSL global data as a
        // leak
        swish::connection::session_pool().destroy();
        ::RAND_cleanup();
        ::ENGINE_cleanup();
        ::CONF_modules_unload(1);
        ::CONF_modules_free();
        ::EVP_cleanup();
        ::ERR_free_strings();
        ::ERR_remove_state(0);
        ::CRYPTO_cleanup_all_ex_data();
    }
};
}

BOOST_GLOBAL_FIXTURE(global_fixture);

namespace test
{
namespace fixtures
{
openssh_fixture::openssh_fixture()
{
    vector<string> docker_command = {"run", "--detach", "-P",
                                     "swish/openssh_server"};
    m_container_id = single_value_from_docker_command<string>(docker_command);
    m_host = ask_docker_for_host();
    m_port = ask_docker_for_port();
}

openssh_fixture::~openssh_fixture()
{
    try
    {
        stop_server();
    }
    catch (...)
    {
    }
}

void openssh_fixture::stop_server()
{
    vector<string> stop_command = {"stop", m_container_id};
    run_docker_command(stop_command);
}

void openssh_fixture::restart_server()
{
    stop_server();

    vector<string> docker_command = {"run", "--detach", "-p",
                                     lexical_cast<string>(m_port) + ":22",
                                     "swish/openssh_server"};
    m_container_id = single_value_from_docker_command<string>(docker_command);
    // We make sure we bind to the same port in the new docker container.  Do we
    // have to do anything to ensure we get the same IP?  Presumably not unless
    // docker-machine changed machine in the middle of restarting.
    assert(ask_docker_for_host() == m_host);
    assert(ask_docker_for_port() == m_port);
}

string openssh_fixture::host() const
{
    return m_host;
}

wstring openssh_fixture::whost() const
{
    generator gen;
    locale loc = gen(get_system_locale());
    return to_utf<wchar_t>(m_host, loc);
}

string openssh_fixture::ask_docker_for_host() const
{
    optional<string> active_docker_machine = docker_machine_name();
    if (active_docker_machine)
    {
        // This can be flaky when tests run in parallel (see
        // https://github.com/docker/machine/issues/2612), so we retry a few
        // times with exponential backoff if it fails
        int attempt_no = 0;
        boost::posix_time::milliseconds wait_time(100);
        while (true)
        {
            ++attempt_no;
            try
            {
                vector<string> machine_ip_command = {"ip", "default"};
                return single_value_from_docker_machine_command<string>(
                    machine_ip_command);
            }
            catch (const runtime_error&)
            {
                if (attempt_no > 5)
                {
                    throw;
                }
                else
                {
                    wait_time *= 2;
                }
            }
            boost::this_thread::sleep(wait_time);
        }
    }
    else
    {
        vector<string> inspect_host_command = {
            "inspect", "--format", "{{ .NetworkSettings.IPAddress }}",
            m_container_id};
        return single_value_from_docker_command<string>(inspect_host_command);
    }
}

string openssh_fixture::user() const
{
    return "swish";
}

wstring openssh_fixture::wuser() const
{
    return L"swish";
}

int openssh_fixture::port() const
{
    return m_port;
}

string openssh_fixture::password() const
{
    return "my test password";
}

wstring openssh_fixture::wpassword() const
{
    return L"my test password";
}

int openssh_fixture::ask_docker_for_port() const
{
    vector<string> inspect_host_command = {
        "inspect", "--format",
        "{{ index (index (index .NetworkSettings.Ports \"22/tcp\") "
        "0) \"HostPort\" }}",
        m_container_id};
    return single_value_from_docker_command<int>(inspect_host_command);
}

/**
 * The private half of a key-pair that is expected to authenticate successfully
 * with the fixture server.
 */
path openssh_fixture::private_key_path() const
{
    return SSHD_PRIVATE_KEY_FILE;
}

/**
 * The public half of a key-pair that is expected to authenticate successfully
 * with the fixture server.
 */
path openssh_fixture::public_key_path() const
{
    return SSHD_PUBLIC_KEY_FILE;
}

/**
 * The private half of a key-pair that is expected to fail to authenticate
 * with the fixture server.
 *
 * This must be in the same format as the successful key-pair so that the key
 * mismatches rather than format mismatches are the cause of authentication
 * failure regardless of which combination of keys is passed.
 */
path openssh_fixture::wrong_private_key_path() const
{
    return SSHD_WRONG_PRIVATE_KEY_FILE;
}

/**
 * The public half of a key-pair that is expected to fail to authenticate
 * with the fixture server.
 *
 * This must be in the same format as the successful key-pair so that the
 * key mismatches rather than format mismatches are the cause of
 * authentication
 * failure regardless of which combination of keys is passed.
 */
path openssh_fixture::wrong_public_key_path() const
{
    return SSHD_WRONG_PUBLIC_KEY_FILE;
}
}
} // namespace test::fixtures
