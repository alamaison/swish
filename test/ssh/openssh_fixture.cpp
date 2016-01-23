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

#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/io/detail/quoted_manip.hpp>
#include <boost/optional.hpp>
#include <boost/process/context.hpp>
#include <boost/process/environment.hpp>
#include <boost/process/operations.hpp> // find_executable_in_path
#include <boost/process/pistream.hpp>
#include <boost/process/self.hpp>
#include <boost/process/stream_behavior.hpp>
#include <boost/test/unit_test.hpp>

#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using ssh::filesystem::path;

using boost::assign::list_of;
using boost::io::quoted;
using boost::optional;
using boost::process::behavior::pipe;
using boost::process::child;
using boost::process::context;
using boost::process::environment;
using boost::process::find_executable_in_path;
using boost::process::pistream;
using boost::process::self;
using boost::process::stderr_id;
using boost::process::stdout_id;

using std::map;
using std::ostream_iterator;
using std::ostringstream;
using std::runtime_error;
using std::string;
using std::vector;
using std::wstring;

namespace
{ // private

const string SSHD_CONFIG_DIR = "sshd-etc";
const string SSHD_PRIVATE_KEY_FILE = "fixture_dsakey";
const string SSHD_PUBLIC_KEY_FILE = "fixture_dsakey.pub";
const string SSHD_WRONG_PRIVATE_KEY_FILE = "fixture_wrong_dsakey";
const string SSHD_WRONG_PUBLIC_KEY_FILE = "fixture_wrong_dsakey.pub";

/**
 * Return the path of the currently running executable.
 */
boost::filesystem::path GetModulePath()
{
    vector<wchar_t> wide_buffer(MAX_PATH);
    if (wide_buffer.size() > 0)
    {
        unsigned long len = ::GetModuleFileName(
            NULL, &wide_buffer[0], static_cast<UINT>(wide_buffer.size()));

        vector<char> buffer(MAX_PATH * 2);
        if (buffer.size() > 0)
        {
            len = ::WideCharToMultiByte(
                CP_UTF8, 0, &wide_buffer[0], len, &buffer[0],
                static_cast<UINT>(buffer.size()), NULL, NULL);
            return string(&buffer[0], len);
        }
    }

    return "";
}

boost::filesystem::path ConfigDir()
{
    return GetModulePath().parent_path() / SSHD_CONFIG_DIR;
}

template <typename ArgSequence>
string error_message_from_stderr(const string& command,
                                 const ArgSequence& arguments, child& process)
{
    pistream command_stderr(process.get_handle(stderr_id));

    ostringstream message;
    message << quoted(command) << " ";
    copy(arguments.begin(), arguments.end(),
         ostream_iterator<string>(message, " "));
    message << " failed: ";
    message << command_stderr.rdbuf() << std::flush;

    return message.str();
}

template <typename Out, typename ArgSequence>
Out single_value_from_executable(const path& executable,
                                 const ArgSequence& arguments)
{
    context ctx;
    ctx.env = self::get_environment();
    ctx.streams[stdout_id] = pipe();
    ctx.streams[stderr_id] = pipe();

    child process = create_child(executable, arguments, ctx);

    pistream command_stdout(process.get_handle(stdout_id));
    Out out;
    command_stdout >> out;

    int status = process.wait();
    // TODO: Check if process exited, with WIFEXITED - may need to upgrade
    // Boost.Process to the 2012 version to get mitigate.hpp, which handles this
    // portably.
    if (status == 0)
    {
        return out;
    }
    else
    {
        BOOST_THROW_EXCEPTION(runtime_error(error_message_from_stderr(
            executable.string(), arguments, process)));
    }
}

template <typename Out, typename ArgSequence>
Out single_value_from_command(const string& command,
                              const ArgSequence& arguments)
{
    path command_executable = find_executable_in_path(command);

    return single_value_from_executable<Out>(command_executable, arguments);
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
    const string docker_machine_name_variable = "DOCKER_MACHINE_NAME";

    boost::process::environment environment = self::get_environment();
    if (environment.count(docker_machine_name_variable) == 1)
    {
        return environment[docker_machine_name_variable];
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
    ~global_fixture()
    {
        // We call this here as a bit of a hack to stop memory-leak
        // detection incorrectly detecting OpenSSL global data as a
        // leak
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
namespace ssh
{

openssh_fixture::openssh_fixture()
{
    vector<string> docker_command =
        (list_of(string("run")), "--detach", "-P", "swish_test_sshd");
    m_container_id = single_value_from_docker_command<string>(docker_command);
    m_host = ask_docker_for_host();
    m_port = ask_docker_for_port();
}

openssh_fixture::~openssh_fixture()
{
    try
    {
        vector<string> stop_command = (list_of(string("stop")), m_container_id);
        run_docker_command(stop_command);
    }
    catch (...)
    {
    }
}

string openssh_fixture::host() const
{
    return m_host;
}

string openssh_fixture::ask_docker_for_host() const
{
    optional<string> active_docker_machine = docker_machine_name();
    if (active_docker_machine)
    {
        vector<string> machine_ip_command = (list_of(string("ip")), "default");
        return single_value_from_docker_machine_command<string>(
            machine_ip_command);
    }
    else
    {
        vector<string> inspect_host_command =
            (list_of(string("inspect")), "--format",
             "{{ .NetworkSettings.IPAddress }}", m_container_id);
        return single_value_from_docker_command<string>(inspect_host_command);
    }
}

string openssh_fixture::user() const
{
    return "swish";
}

int openssh_fixture::port() const
{
    return m_port;
}

int openssh_fixture::ask_docker_for_port() const
{
    vector<string> inspect_host_command =
        (list_of(string("inspect")), "--format",
         "{{ index (index (index .NetworkSettings.Ports \"22/tcp\") "
         "0) \"HostPort\" }}",
         m_container_id);
    return single_value_from_docker_command<int>(inspect_host_command);
}

path openssh_fixture::sandbox() const
{
    return "sandbox";
}

path openssh_fixture::absolute_sandbox() const
{
    return "/home/swish/sandbox";
}

/**
 * The private half of a key-pair that is expected to authenticate successfully
 * with the fixture server.
 */
boost::filesystem::path openssh_fixture::private_key_path() const
{
    return ConfigDir() / SSHD_PRIVATE_KEY_FILE;
}

/**
 * The public half of a key-pair that is expected to authenticate successfully
 * with the fixture server.
 */
boost::filesystem::path openssh_fixture::public_key_path() const
{
    return ConfigDir() / SSHD_PUBLIC_KEY_FILE;
}

/**
 * The private half of a key-pair that is expected to fail to authenticate
 * with the fixture server.
 *
 * This must be in the same format as the successful key-pair so that the key
 * mismatches rather than format mismatches are the cause of authentication
 * failure regardless of which combination of keys is passed.
 */
boost::filesystem::path openssh_fixture::wrong_private_key_path() const
{
    return ConfigDir() / SSHD_WRONG_PRIVATE_KEY_FILE;
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
boost::filesystem::path openssh_fixture::wrong_public_key_path() const
{
    return ConfigDir() / SSHD_WRONG_PUBLIC_KEY_FILE;
}
}
} // namespace test::ssh
