#pragma once

#include "test/common_boost/helpers.hpp" // wstring output

#include <boost/lexical_cast.hpp> // lexical_cast

#include <swish/utils.hpp> // environment_variable

#include <string>

namespace test {

namespace detail {

	inline std::wstring from_env(const std::wstring& variable_name)
	{
		std::wstring var = swish::utils::environment_variable(variable_name);
		BOOST_REQUIRE_MESSAGE(
			!var.empty(),
			L"Environment variable '" + variable_name + L"' must exist");
		return var;
	}

}

class remote_test_config
{
public:
	remote_test_config()
		: m_host(detail::from_env(L"TEST_HOST_NAME")),
		m_user(detail::from_env(L"TEST_USER_NAME")),
		m_password(detail::from_env(L"TEST_PASSWORD")),
		m_port(boost::lexical_cast<USHORT>(detail::from_env(L"TEST_HOST_PORT")))
	{}

	/**
	 * Get the host name of the machine to connect to for remote testing.
	 *
	 * The host name is retrieved from the TEST_HOST_NAME environment variable.
	 * If this variable is not set, a CPPUNIT exception is thrown.
	 * 
	 * In order to be useful, the host name should exist and the machine 
	 * should be accessible via SSH.
	 * 
	 * @pre the host name should have at least 3 characters
	 * @pre the host name should have less than 255 characters
	 *
	 * @return the host name
	 */
	std::wstring GetHost() const
	{
		return m_host;
	}
		 
	/**
	 * Get the user name of the SSH account to connect to on the remote machine.
	 *
	 * The user name is retrieved from the TEST_USER_NAME environment variable.
	 * If this variable is not set, a CPPUNIT exception is thrown.
	 * 
	 * In order to be useful, the user name should correspond to a valid SSH
	 * account on the testing machine.
	 * 
	 * @pre the user name should have at least 3 characters
	 * @pre the user name should have less than 64 characters
	 *
	 * @return the user name
	 */
	std::wstring GetUser() const
	{
		return m_user;
	}

	/**
	 * Get the port to connect to on the remote testing machine.
	 *
	 * The port is retrieved from the TEST_HOST_PORT environment variable.
	 * If this variable is not set, the default SSH port 22 is returned.
	 * 
	 * In order to be useful, the machine should be accessible via SSH on
	 * this port.
	 * 
	 * @post the port should be between 0 and 65535 inclusive
	 *
	 * @return the port number
	 */
	USHORT GetPort() const
	{
		return static_cast<USHORT>(m_port);
	}

	/**
	 * Get the password to use to connect to the SSH account on the remote machine.
	 *
	 * The password is retrieved from the TEST_PASSWORD environment variable.
	 * If this variable is not set, a CPPUNIT exception is thrown.
	 * 
	 * In order to be useful, the password should be valid for the SSH
	 * account on the testing machine.
	 * 
	 * @return the password
	 */
	std::wstring GetPassword() const
	{
		return m_password;
	}

private:
	std::wstring m_host;
	std::wstring m_user;
	std::wstring m_password;
	USHORT m_port;
};

}