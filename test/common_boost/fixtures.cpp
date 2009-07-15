/**
    @file

    Fixtures common to several test cases that use Boost.Test.

    @if licence

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <boost/filesystem.hpp>
#include "swish/boost_process.hpp"
#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>

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
using boost::lexical_cast;

using std::string;
using std::vector;
using std::map;

namespace { // private

	const int SSHD_PORT = 30000;
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
		vector<string> full_args = list_of(sshd_path);
		copy(args.begin(), args.end(), back_inserter(full_args));

		context ctx; 
		ctx.environment = self::get_environment();
		/* Uncomment if needed
		ctx.stdout_behavior = boost::process::inherit_stream();
		ctx.stderr_behavior = boost::process::redirect_stream_to_stdout();
		*/
		return launch(sshd_path, full_args, ctx);
	}

	path ConfigDir()
	{
		return GetModulePath().parent_path() / SSHD_CONFIG_DIR;
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

	vector<string> GetSshdOptions()
	{
		path host_key_file = ConfigDir() / SSHD_HOST_KEY_FILE;
		path auth_key_file = ConfigDir() / SSHD_PUBLIC_KEY_FILE;
		vector<string> options = (list_of(string("-D")),
			"-f", SSHD_CONFIG_FILE,
			"-h", Cygdriveify(host_key_file).string(),
			"-o", "AuthorizedKeysFile \"" +
			      Cygdriveify(auth_key_file).string() + "\"",
			"-o", "ListenAddress " + SSHD_LISTEN_ADDRESS + ":" +
			       lexical_cast<string>(SSHD_PORT),
			"-o", "Protocol 2",
			"-o", "UsePrivilegeSeparation no",
			"-o", "StrictModes no",
			"-o", "Subsystem sftp " + Cygdriveify(GetSftpPath()).string());
		return options;
	}
}


namespace test {
namespace common_boost {

OpenSshFixture::OpenSshFixture() : m_sshd(StartSshd(GetSshdOptions()))
{
}

OpenSshFixture::~OpenSshFixture()
{
	BOOST_WARN_NO_THROW(StopServer());
}

int OpenSshFixture::StopServer()
{
	m_sshd.terminate();
	return m_sshd.wait().exit_status();
}

string OpenSshFixture::GetHost() const
{
	return SSHD_LISTEN_ADDRESS;
}

int OpenSshFixture::GetPort() const
{
	return SSHD_PORT;
}

path OpenSshFixture::GetPrivateKey() const
{
	return ConfigDir() / SSHD_PRIVATE_KEY_FILE;
}

path OpenSshFixture::GetPublicKey() const
{
	return ConfigDir() / SSHD_PUBLIC_KEY_FILE;
}

}} // namespace test::common_boost
