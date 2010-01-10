/**
    @file

    Fixture for tests that need a backend data provider.

    @if licence

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#pragma once

#include "test/common_boost/ConsumerStub.hpp"  // CConsumerStub
#include "test/common_boost/fixtures.hpp"  // SandboxFixture

#include "swish/shell_folder/Pool.h" // CPool
#include "swish/interfaces/SftpProvider.h"  // ISftpProvider
#include "swish/utils.hpp"  // string conversion functions

#include <comet/ptr.h> // com_ptr
#include <comet/util.h> // auto_coinit

#include <boost/filesystem/path.hpp> // path
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/make_shared.hpp> // make_shared

#include <string>

namespace {

	comet::com_ptr<ISftpProvider> provider_instance(
		const std::string& host, const std::string& user, int port)
	{
		std::wstring whost = swish::utils::Utf8StringToWideString(host);
		std::wstring wuser = swish::utils::Utf8StringToWideString(user);

		// Create Provider instance
		CPool pool;
		return pool.GetSession(whost, wuser, port);
	}

	/**
	 * Helper to ensure CoInitialize/CoUnitialize is called correctly for
	 * static object instance.
	 */
	class static_provider
	{
	public:
		static_provider(
			const std::string& host, const std::string& user, int port)
			: m_provider(provider_instance(host, user, port)) {}

		comet::com_ptr<ISftpProvider> get() const { return m_provider; }

	private:
		comet::auto_coinit coinit;
		comet::com_ptr<ISftpProvider> m_provider;
	};

	static boost::shared_ptr<static_provider> s_provider; ///< static provider

	template<typename ServerType>
	boost::shared_ptr<ServerType> singleton_server()
	{
		static boost::shared_ptr<ServerType> s_server;
		if (!s_server)
		{
			s_server = boost::make_shared<ServerType>();
		}

		return s_server;
	}
}

namespace test {
namespace provider {

/**
 * Abstract base class of mortality policy classes.
 * Provides implementation of property accessors which don't vary between
 * mortality policies.
 */
template<typename ServerType>
class mortality_policy_base
{
public:

	std::string host() const { return server().GetHost(); }

	std::string user() const { return server().GetUser(); }

	int port() const { return server().GetPort(); }

	boost::filesystem::path private_key() const
	{ return server().PrivateKeyPath(); }

	boost::filesystem::path public_key() const
	{ return server().PublicKeyPath(); }

	std::string local_to_remote(const boost::filesystem::path& local_path)
	const
	{
		return server().ToRemotePath(local_path);
	}

	boost::filesystem::wpath local_to_remote(
		const boost::filesystem::wpath& local_path) const
	{
		return server().ToRemotePath(local_path);
	}

private:
	virtual const ServerType& server() const = 0;
};

/**
 * Provider mortality policy that reuses a single instance for many tests.
 *
 * This policy also manages the server lifetime by keeping in alive
 * permanently.  This may cause problems if using an OpenSSH server in
 * debug (-d -d -d) mode as this only allows a single connection before
 * terminating itself.  Use the mortal_provider policy in this case.
 *
 * This policy is not in the spirit of unit testing but it makes them run
 * so much faster that it's worth the risk.
 */
template<typename ServerType>
class immortal_provider : public mortality_policy_base<ServerType>
{
public:

	/**
	 * Return a pointer to the static provider instance.
	 *
	 * Created on demand on the first request.
	 */
	comet::com_ptr<ISftpProvider> provider() const
	{
		if (!s_provider)
		{
			s_provider = boost::make_shared<static_provider>(
				host(), user(), port());
		}

		return s_provider->get();
	}
	
private:
	const ServerType& server() const
	{ return *singleton_server<ServerType>(); }
};

/**
 * Provider mortality policy that creates a new instance for each test.
 *
 * As well as managing the provider lifetime, this policy stop and restarts
 * the SSH server on each test.  This ensures that the Provider cache can't
 * give us an existing instance instead of a new one.
 *
 * Use this mortality policy when testing provider setup and authentication.
 * These tests will not work with instances that have already been used.
 *
 */
template<typename ServerType>
class mortal_provider : public mortality_policy_base<ServerType>
{
public:

	/**
	 * Return a pointer to a new provider instance.
	 */
	comet::com_ptr<ISftpProvider> provider()
	{
		if (!m_provider)
		{
			m_provider = provider_instance(
				m_server.GetHost(), m_server.GetUser(), m_server.GetPort());
		}

		return m_provider;
	}

private:
	ServerType m_server;
	comet::auto_coinit m_coinit;
	comet::com_ptr<ISftpProvider> m_provider;

	const ServerType& server() const { return m_server; }
};

template<typename MortalityPolicy>
class ProviderFixtureT : public test::common_boost::SandboxFixture
{
public:

	/**
	 * Get a CProvider instance connected to the fixture SSH server.
	 */
	comet::com_ptr<ISftpProvider> ProviderFixtureT::Provider()
	{
		return m_policy.provider();
	}

	/**
	 * Get a dummy consumer to use in calls to provider.
	 */
	comet::com_ptr<ISftpConsumer> ProviderFixtureT::Consumer()
	{
		comet::com_ptr<test::common_boost::CConsumerStub> consumer = 
			test::common_boost::CConsumerStub::CreateCoObject();
		consumer->SetKeyPaths(m_policy.private_key(), m_policy.public_key());
		return consumer;
	}

	std::string GetUser() { return m_policy.user(); }
	std::string GetHost() { return m_policy.host(); }
	int GetPort() { return m_policy.port(); }

	std::string ToRemotePath(const boost::filesystem::path& local_path)
	{
		return m_policy.local_to_remote(local_path);
	}

	boost::filesystem::wpath ToRemotePath(
		const boost::filesystem::wpath& local_path)
	{
		return m_policy.local_to_remote(local_path);
	}

private:
	MortalityPolicy m_policy;
};

#ifdef _DEBUG
typedef ProviderFixtureT<
	immortal_provider<test::common_boost::OpenSshFixture> > ProviderFixture;
#else
typedef ProviderFixtureT<
	mortal_provider<test::common_boost::OpenSshFixture> > ProviderFixture;
#endif

typedef ProviderFixtureT<
	mortal_provider<test::common_boost::OpenSshFixture> >
MortalProviderFixture;

}} // namespace test::provider
