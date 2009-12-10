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

#include "swish/utils.hpp"
#include "swish/exception.hpp"

#include "test/common_boost/helpers.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include "swish/boost_process.hpp"
#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#pragma warning(push)
#pragma warning(disable:4100) // unreferenced formal parameter
#include <boost/random/uniform_int.hpp>
#pragma warning(pop)
#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>  // mt19937

#include <comet/ptr.h> // com_ptr
#include <comet/interface.h>  // uuidof, comtype

#include <string>
#include <vector>
#include <map>

#include <cstdio>

using swish::utils::Utf8StringToWideString;
using swish::utils::WideStringToUtf8String;
using swish::utils::current_user_a;
using swish::exception::com_exception;

using boost::system::system_error;
using boost::system::system_category;
using boost::filesystem::path;
using boost::filesystem::wpath;
using boost::filesystem::ofstream;
using boost::filesystem::ifstream;
using boost::process::environment;
using boost::process::context;
using boost::process::child;
using boost::process::self;
using boost::process::find_executable_in_path;
using boost::assign::list_of;
using boost::lexical_cast;
using boost::shared_ptr;
using boost::uniform_int;
using boost::variate_generator;
using boost::mt19937;
using boost::filesystem::wpath;

using comet::com_ptr;

using std::string;
using std::wstring;
using std::vector;
using std::map;


namespace comet {

template<> struct comtype<IShellLink>
{
	static const IID& uuid() throw() { return IID_IShellLink; }
	typedef IUnknown base;
};

}

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
		vector<string> full_args = list_of(sshd_path); //("-ddd");
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

	int GenerateRandomPort()
	{
		static mt19937 rndgen;
		static uniform_int<> distribution(10000, 60000);
		static variate_generator<mt19937, uniform_int<> > gen(
			rndgen, distribution);
		rndgen.seed(static_cast<unsigned int>(std::time(0)));
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
			       lexical_cast<string>(port),
			"-o", "Protocol 2",
			"-o", "UsePrivilegeSeparation no",
			"-o", "StrictModes no",
			"-o", "Subsystem sftp " + Cygdriveify(GetSftpPath()).string());
		return options;
	}
}


namespace test {
namespace common_boost {

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
	return m_sshd.wait().exit_status();
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

namespace {

	/**
	 * Windows shortcut header structure from Cygwin's path.cc.
	 */
	struct win_shortcut_hdr
	{
		DWORD size;		/* Header size in bytes.  Must contain 0x4c. */
		GUID magic;		/* GUID of shortcut files. */
		DWORD flags;	/* Content flags.  See above. */

		/* The next fields from attr to icon_no are always set to 0 in Cygwin
		   and U/Win shortcuts. */
		DWORD attr;	/* Target file attributes. */
		FILETIME ctime;	/* These filetime items are never touched by the */
		FILETIME mtime;	/* system, apparently. Values don't matter. */
		FILETIME atime;
		DWORD filesize;	/* Target filesize. */
		DWORD icon_no;	/* Icon number. */

		DWORD run;		/* Values defined in winuser.h. Use SW_NORMAL. */
		DWORD hotkey;	/* Hotkey value. Set to 0.  */
		DWORD dummy[2];	/* Future extension probably. Always 0. */
	};

	static const GUID GUID_shortcut
		= { 0x00021401L, 0, 0, {0xc0, 0, 0, 0, 0, 0, 0, 0x46}};
}

/**
 * Create a symbolic link to given file in the same directory.
 *
 * The the Cygwin-based OpenSSH server, this is done by creating a shortcut
 * with the .lnk extension added.  The shortcut has to have a *very* specific
 * structure to get Cygwin to recognise it.
 */
path OpenSshFixture::create_link(const wpath& file, const wpath& shortcut_name)
const
{
	wpath link_path = file.parent_path() / shortcut_name;
	wpath link_shortcut = link_path.string() + L".lnk";

	path target_path = WideStringToUtf8String(file.string());
	size_t target_path_len = target_path.string().size();
	path posix_target_path = ToRemotePath(target_path);
	size_t posix_target_path_len = posix_target_path.string().size();

	size_t len = sizeof(win_shortcut_hdr) + 2 + target_path_len + 
		2 + posix_target_path_len;
	vector<char> buf(len);

	win_shortcut_hdr* header = reinterpret_cast<win_shortcut_hdr*>(&buf[0]);
	std::memset(header, 0, sizeof(*header));
	header->size = 0x4c;
	std::memcpy(&(header->magic), &GUID_shortcut, sizeof(header->magic));
	header->flags = 0x08 | 0x04; // description | relative path
	header->run = SW_NORMAL;

	char* p = &buf[sizeof(win_shortcut_hdr)];

#pragma warning(push)
#pragma warning(disable:4996)
	// Description
	*reinterpret_cast<unsigned short*>(p) = static_cast<unsigned short>(
		posix_target_path_len);
	posix_target_path.string().copy(p += 2, posix_target_path_len);
	p += posix_target_path_len;

	// Relative path
	*reinterpret_cast<unsigned short*>(p) = static_cast<unsigned short>(
		target_path_len);
	target_path.string().copy(p += 2, target_path_len);
	p += target_path_len;
#pragma warning(pop)

	ofstream s(link_shortcut, std::ios::binary);
	s.write(&buf[0], buf.size());
	s.close();

	// must be readonly for Cygwin to recognise as shortcut
	BOOST_REQUIRE(::SetFileAttributes(
		link_shortcut.file_string().c_str(), 
		::GetFileAttributes(
			link_shortcut.file_string().c_str()) | FILE_ATTRIBUTE_READONLY));

	return WideStringToUtf8String(ToRemotePath(link_path).string());
}

/**
 * Return the real file on the local filesystem that is represented by the
 * given file.
 *
 * For instance, a symlink called 'foo' would resolve to the file that the
 * symlink points to (i.e. the file that foo.lnk points to - not foo.lnk
 * itself).
 */
wpath OpenSshFixture::resolve(const wpath& file) const
{
	wpath shortcut = file.string() + L".lnk";
	if (exists(shortcut))
	{
		ifstream s(shortcut, std::ios::binary);
		
		// Skip to relative path part
		s.seekg(sizeof(win_shortcut_hdr));
		unsigned short len;
		s.read(reinterpret_cast<char*>(&len), 2);
		s.seekg(len, std::ios::cur);
		s.read(reinterpret_cast<char*>(&len), 2);

		// Read and return relative path
		vector<char> buf(MAX_PATH);
		s.read(&buf[0], min(len, buf.size()));
		return Utf8StringToWideString(string(&buf[0]));
	}
	else
		return file;
}

namespace { // private

	const wstring SANDBOX_NAME = L"swish-sandbox";

	wpath NewTempFilePath()
	{
		vector<wchar_t> buffer(MAX_PATH);
		DWORD len = ::GetTempPath(buffer.size(), &buffer[0]);
		BOOST_REQUIRE_LE(len, buffer.size());
		
		wpath directory(wstring(&buffer[0], buffer.size()));
		directory /= SANDBOX_NAME;
		create_directory(directory);

		if (!GetTempFileName(
			directory.directory_string().c_str(), NULL, 0, &buffer[0]))
			throw boost::system::system_error(
				::GetLastError(), boost::system::system_category);
		
		return wpath(wstring(&buffer[0], buffer.size()));
	}

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

/**
 * Create a new empty file in the fixture sandbox with a random name
 * and return the path.
 */
wpath SandboxFixture::NewFileInSandbox()
{
	vector<wchar_t> buffer(MAX_PATH);

	if (!GetTempFileName(
		Sandbox().directory_string().c_str(), NULL, 0, &buffer[0]))
		throw system_error(::GetLastError(), system_category);
	
	wpath p = wpath(wstring(&buffer[0], buffer.size()));
	BOOST_CHECK(exists(p));
	BOOST_CHECK(is_regular_file(p));
	BOOST_CHECK(p.is_complete());
	return p;
}

}} // namespace test::common_boost
