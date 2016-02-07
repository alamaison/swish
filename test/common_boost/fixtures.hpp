// Copyright 2009, 2012, 2013, 2014, 2016 Alexander Lamaison

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

#pragma once

#include <boost/test/unit_test.hpp>
#include <boost/system/system_error.hpp> // For system_error

#include <Winsock2.h> // For WSAStartup/Cleanup
#include <objbase.h>  // For CoInitialize/Uninitialize

#include <string>

namespace test
{

/**
 * Fixture that initialises and uninitialises COM.
 */
class ComFixture
{
public:
    ComFixture()
    {
        HRESULT hr = ::CoInitialize(NULL);
        BOOST_WARN_MESSAGE(SUCCEEDED(hr), "::CoInitialize failed");
    }

    virtual ~ComFixture()
    {
        ::CoUninitialize();
    }
};

/**
 * Fixture that initialises and uninitialises Winsock.
 */
class WinsockFixture
{
public:
    WinsockFixture()
    {
        WSADATA wsadata;
        int err = ::WSAStartup(MAKEWORD(2, 2), &wsadata);
        if (err)
            throw boost::system::system_error(
                err, boost::system::get_system_category());
    }

    virtual ~WinsockFixture()
    {
        ::WSACleanup();
    }
};

} // namespace test
