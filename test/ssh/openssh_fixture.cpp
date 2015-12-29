/**
    @file

    Fixture that runs local OpenSSH server for testing.

    @if license

    Copyright (C) 2009, 2010, 2011, 2012
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

#include "openssh_fixture.hpp"

#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/process/context.hpp> // context
#include <boost/process/environment.hpp> // environment
#include <boost/process/operations.hpp> // find_executable_in_path
#include <boost/process/self.hpp> // self
#pragma warning(push)
#pragma warning(disable:4100) // unreferenced formal parameter
#include <boost/random/uniform_int.hpp>
#pragma warning(pop)
#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>  // mt19937
#include <boost/test/unit_test.hpp>

#include <sstream>
#include <string>
#include <vector>
#include <map>

using boost::filesystem::path;
using boost::process::environment;
using boost::process::context;
using boost::process::child;
using boost::process::self;
using boost::process::find_executable_in_path;
using boost::assign::list_of;
using boost::uniform_int;
using boost::variate_generator;
using boost::mt19937;

using std::string;
using std::wstring;
using std::vector;
using std::map;

namespace { // private

    const string SSHD_LISTEN_ADDRESS = "localhost";
    const string SSHD_EXE_NAME = "sshd.exe";
    const string SFTP_SUBSYSTEM = "sftp-server";
    const string SSHD_DIR_ENVIRONMENT_VAR = "OPENSSH_DIR";
    const string SSHD_CONFIG_DIR = "sshd-etc";
    const string SSHD_CONFIG_FILE = "/dev/null";
    const string SSHD_HOST_KEY_FILE = "fixture_hostkey";
    const string SSHD_PRIVATE_KEY_FILE = "fixture_dsakey";
    const string SSHD_PUBLIC_KEY_FILE = "fixture_dsakey.pub";
    const string SSHD_WRONG_PRIVATE_KEY_FILE = "fixture_wrong_dsakey";
    const string SSHD_WRONG_PUBLIC_KEY_FILE = "fixture_wrong_dsakey.pub";

    ssh::filesystem::path cygdrive_prefix()
    {
        return "/cygdrive/";
    }

    /**
     * Locale-independent port number to port string conversion.
     */
    std::string port_to_string(long port)
    {
        std::ostringstream stream;
        stream.imbue(std::locale::classic()); // force locale-independence
        stream << port;
        if (!stream)
            BOOST_THROW_EXCEPTION(
                std::logic_error("Unable to convert port number to string"));

        return stream.str();
    }

    string current_user()
    {
        // Calculate required size of output buffer
        DWORD len = 0;
        if (::GetUserNameA(NULL, &len))
            return string();

        DWORD err = ::GetLastError();
        if (err != ERROR_INSUFFICIENT_BUFFER)
        {
            BOOST_THROW_EXCEPTION(
                boost::system::system_error(
                    err, boost::system::get_system_category()));
        }

        // Repeat call with a buffer of required size
        if (len > 0)
        {
            std::vector<char> buffer(len);
            if (::GetUserNameA(&buffer[0], &len))
            {
                return string(&buffer[0], len - 1);
            }
            else
            {
                BOOST_THROW_EXCEPTION(
                    boost::system::system_error(
                    ::GetLastError(), boost::system::get_system_category()));
            }
        }
        else
        {
            return string();
        }
    }

    /**
     * Return the path of the currently running executable.
     */
    path GetModulePath()
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
                    CP_UTF8, 0, &wide_buffer[0], len,
                    &buffer[0], static_cast<UINT>(buffer.size()), NULL, NULL);
                return string(&buffer[0], len);
            }
        }

        return "";
    }

    /**
     * Try to find OpenSSH (sshd) directory path in an environment variable.
     */
    path GetSshdDirFromEnvironment()
    {
        map<string, string>::const_iterator pos;
        environment env = self::get_environment();
        pos = env.find(SSHD_DIR_ENVIRONMENT_VAR);
        if (pos != env.end())
            return pos->second;

        return path();
    }

    /**
     * Find OpenSSH (sshd); either in an environment variable or on the path.
     */
    path GetSshdPath()
    {
        path sshd_dir = GetSshdDirFromEnvironment();
        if (!sshd_dir.empty())
            return sshd_dir / SSHD_EXE_NAME;
        else
            return find_executable_in_path(SSHD_EXE_NAME);
    }

    /**
     * Find OpenSSH SFTP subsystem (sftp-server).
     * Either in an environment variable or on the path in the same directory
     * as sshd.
     */
    path GetSftpPath()
    {
        path sshd_dir = GetSshdDirFromEnvironment();
        if (!sshd_dir.empty())
            return sshd_dir / SFTP_SUBSYSTEM;
        else
            return find_executable_in_path(SFTP_SUBSYSTEM);
    }

    /**
     * Invoke the sshd program with the given list of arguments.
     */
    child StartSshd(vector<string> args)
    {
        string sshd_path = GetSshdPath().string();

        context ctx;
        ctx.env = self::get_environment();

        // sshd insists on an absolute path but what it actually looks at isn't
        // what it is invoked as, rather the first argument passed to is.
        // By default Boost.Filesystem uses just the exe filename for that so
        // we force it to use the full path here.
        ctx.process_name = sshd_path;

        /* Uncomment if needed
        ctx.stdout_behavior = boost::process::inherit_stream();
        ctx.stderr_behavior = boost::process::redirect_stream_to_stdout();
        */
        return create_child(sshd_path, args, ctx);
    }

    path ConfigDir()
    {
        return GetModulePath().parent_path() / SSHD_CONFIG_DIR;
    }

    int GenerateRandomPort()
    {
        static mt19937 rndgen(static_cast<boost::uint32_t>(std::time(0)));
        static uniform_int<> distribution(10000, 65535);
        static variate_generator<mt19937, uniform_int<> > gen(
            rndgen, distribution);
        return gen();
    }

    /**
     * Turn a path, rooted at a Windows drive letter, into a /cygdrive path.
     *
     * For example:
     *   C:\\Users\\username\\file becomes /cygdrive/c/Users/username/file
     */
    ssh::filesystem::path Cygdriveify(path windows_path)
    {
        wstring drive(windows_path.root_name().wstring(), 0, 1);
        ssh::filesystem::path remote_path = cygdrive_prefix() / drive;
        BOOST_FOREACH(const path& segment, windows_path.relative_path())
        {
            remote_path /= segment.wstring();
        }
        return remote_path;
    }

    vector<string> GetSshdOptions(int port)
    {
        path host_key_file = ConfigDir() / SSHD_HOST_KEY_FILE;
        path auth_key_file = ConfigDir() / SSHD_PUBLIC_KEY_FILE;
        vector<string> options = (list_of(string("-D")),
            "-f", SSHD_CONFIG_FILE,
            "-h", Cygdriveify(host_key_file).string(),
            "-o", "AuthorizedKeysFile \"" +
                  Cygdriveify(auth_key_file).string() + "\"",
            "-o", "ListenAddress " + SSHD_LISTEN_ADDRESS + ":" +
                   port_to_string(port),
            "-o", "Protocol 2",
            "-o", "UsePrivilegeSeparation no",
            "-o", "StrictModes no",
            "-o", "Subsystem sftp " + Cygdriveify(GetSftpPath()).string());
        return options;
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

namespace {
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

    BOOST_GLOBAL_FIXTURE(global_fixture);
}

namespace test {
namespace ssh {

openssh_fixture::openssh_fixture() :
    m_port(GenerateRandomPort()),
    m_sshd(StartSshd(GetSshdOptions(m_port)))
{
}

openssh_fixture::~openssh_fixture()
{
    try
    {
        stop_server();
    }
    catch (...) {}
}

int openssh_fixture::stop_server()
{
    m_sshd.terminate();
    return m_sshd.wait();
}

string openssh_fixture::host() const
{
    return SSHD_LISTEN_ADDRESS;
}

string openssh_fixture::user() const
{
    return current_user();
}

int openssh_fixture::port() const
{
    return m_port;
}

/**
 * The private half of a key-pair that is expected to authenticate successfully
 * with the fixture server.
 */
path openssh_fixture::private_key_path() const
{
    return ConfigDir() / SSHD_PRIVATE_KEY_FILE;
}

/**
 * The public half of a key-pair that is expected to authenticate successfully
 * with the fixture server.
 */
path openssh_fixture::public_key_path() const
{
    return ConfigDir() / SSHD_PUBLIC_KEY_FILE;
}

/**
 * The private half of a key-pair that is expected to fail to authenticate
 * with the fixture server.
 *
 * This must be in the same format as the successful key-pair so that the
 * key mismatches rather than format mismatches are the cause of authentication
 * failure regardless of which combination of keys is passed.
 */
path openssh_fixture::wrong_private_key_path() const
{
    return ConfigDir() / SSHD_WRONG_PRIVATE_KEY_FILE;
}

/**
 * The public half of a key-pair that is expected to fail to authenticate
 * with the fixture server.
 *
 * This must be in the same format as the successful key-pair so that the
 * key mismatches rather than format mismatches are the cause of authentication
 * failure regardless of which combination of keys is passed.
 */
path openssh_fixture::wrong_public_key_path() const
{
    return ConfigDir() / SSHD_WRONG_PUBLIC_KEY_FILE;
}

/**
 * Transform a local (Windows) path into a form usuable on the
 * command-line of the fixture SSH server.
 */
::ssh::filesystem::path openssh_fixture::to_remote_path(path local_path) const
{
    return Cygdriveify(local_path);
}

}} // namespace test::ssh
