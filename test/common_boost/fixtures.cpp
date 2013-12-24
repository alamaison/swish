/**
    @file

    Fixtures common to several test cases that use Boost.Test.

    @if license

    Copyright (C) 2009, 2011, 2012, 2013
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

#include "fixtures.hpp"

#include "swish/port_conversion.hpp" // port_to_string
#include "swish/utils.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp> // wofstream
#include <boost/assign/list_of.hpp>
#include <boost/numeric/conversion/cast.hpp>  // numeric_cast
#include <boost/process/context.hpp> // context
#include <boost/process/environment.hpp> // environment
#include <boost/process/operations.hpp> // find_executable_in_path
#include <boost/process/self.hpp> // self
#include <boost/shared_ptr.hpp>
#pragma warning(push)
#pragma warning(disable:4100) // unreferenced formal parameter
#include <boost/random/uniform_int.hpp>
#pragma warning(pop)
#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>  // mt19937

#include <string>
#include <vector>
#include <map>

#include <cstdio>

using swish::port_to_string;
using swish::utils::Utf8StringToWideString;
using swish::utils::current_user_a;

using boost::system::get_system_category;
using boost::system::system_error;
using boost::filesystem::wofstream;
using boost::filesystem::path;
using boost::filesystem::wpath;
using boost::process::environment;
using boost::process::context;
using boost::process::child;
using boost::process::self;
using boost::process::find_executable_in_path;
using boost::assign::list_of;
using boost::numeric_cast;
using boost::shared_ptr;
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

    const path CYGDRIVE_PREFIX = "/cygdrive/";

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
    path Cygdriveify(path windowsPath)
    {
        string drive(windowsPath.root_name(), 0, 1);
        return CYGDRIVE_PREFIX / drive / windowsPath.relative_path();
    }

    wpath Cygdriveify(wpath windowsPath)
    {
        wstring drive(windowsPath.root_name(), 0, 1);
        return wpath(Utf8StringToWideString(
            CYGDRIVE_PREFIX.directory_string())) / drive / 
            windowsPath.relative_path();
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


namespace test {

OpenSshFixture::OpenSshFixture() : 
    m_port(GenerateRandomPort()),
    m_sshd(StartSshd(GetSshdOptions(m_port)))
{
}

OpenSshFixture::~OpenSshFixture()
{
    try
    {
        StopServer();
    }
    catch (...) {}
}

int OpenSshFixture::StopServer()
{
    m_sshd.terminate();
    return m_sshd.wait();
}

string OpenSshFixture::GetHost() const
{
    return SSHD_LISTEN_ADDRESS;
}

string OpenSshFixture::GetUser() const
{
    return current_user_a();
}

int OpenSshFixture::GetPort() const
{
    return m_port;
}

path OpenSshFixture::PrivateKeyPath() const
{
    return ConfigDir() / SSHD_PRIVATE_KEY_FILE;
}

path OpenSshFixture::PublicKeyPath() const
{
    return ConfigDir() / SSHD_PUBLIC_KEY_FILE;
}

/**
 * Transform a local (Windows) path into a form usuable on the
 * command-line of the fixture SSH server.
 */
string OpenSshFixture::ToRemotePath(path local_path) const
{
    return Cygdriveify(local_path).string();
}

wpath OpenSshFixture::ToRemotePath(wpath local_path) const
{
    return Cygdriveify(local_path).string();
}


namespace { // private

    const wstring SANDBOX_NAME = L"swish-sandbox";

    /**
     * Return the path to the sandbox directory.
     */
    wpath SandboxDirectory()
    {
        shared_ptr<wchar_t> name(
            _wtempnam(NULL, SANDBOX_NAME.c_str()), free);
        BOOST_REQUIRE(name);
        
        return wpath(name.get());
    }
}

SandboxFixture::SandboxFixture() : m_sandbox(SandboxDirectory())
{
    create_directory(m_sandbox);
}

SandboxFixture::~SandboxFixture()
{
    try
    {
        remove_all(m_sandbox);
    }
    catch (...) {}
}

wpath SandboxFixture::Sandbox()
{
    return m_sandbox;
}

wpath SandboxFixture::NewFileInSandbox(wstring name)
{
    wpath p = Sandbox() / name;
    BOOST_REQUIRE(!exists(p));
    wofstream file(p);
    BOOST_CHECK(exists(p));
    return p;
}

/**
 * Create a new empty file in the fixture sandbox with a random name
 * and return the path.
 */
wpath SandboxFixture::NewFileInSandbox()
{
    vector<wchar_t> buffer(MAX_PATH);

    if (!GetTempFileName(
        Sandbox().directory_string().c_str(), NULL, 0, &buffer[0]))
        throw system_error(::GetLastError(), get_system_category());
    
    wpath p = wpath(wstring(&buffer[0], buffer.size()));
    BOOST_CHECK(exists(p));
    BOOST_CHECK(is_regular_file(p));
    BOOST_CHECK(p.is_complete());
    return p;
}

wpath SandboxFixture::NewDirectoryInSandbox()
{
    // This is a bit of a hack but it's simple and works: create a new file,
    // delete it, reuse the filename to make a folder.  It's not worth
    // investigating the proper way to get a new random directory name.

    wpath file = NewFileInSandbox();
    remove(file);
    create_directory(file);
    BOOST_CHECK(is_directory(file));
    return file;
}

wpath SandboxFixture::NewDirectoryInSandbox(wstring name)
{
    wpath p = Sandbox() / name;
    BOOST_REQUIRE(!exists(p));
    create_directory(p);
    return p;
}

} // namespace test
