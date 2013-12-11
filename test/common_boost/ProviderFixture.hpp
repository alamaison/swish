/**
    @file

    Fixture for tests that need a backend data provider.

    @if license

    Copyright (C) 2009, 2010, 2011, 2013
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

#pragma once

#include "test/common_boost/MockConsumer.hpp"
#include "test/common_boost/fixtures.hpp"  // SandboxFixture, OpenSshFixture

#include "swish/provider/sftp_provider.hpp"

#include <comet/ptr.h> // com_ptr

#include <boost/filesystem/path.hpp> // path
#include <boost/shared_ptr.hpp>

#include <string>

namespace swish
{
    namespace connection {
        
        class authenticated_session;

    }
}

namespace test {

class ProviderFixture :
    public test::SandboxFixture, public test::OpenSshFixture
{
public:

    ProviderFixture();
    ~ProviderFixture();

    /**
     * Get a CProvider instance connected to the fixture SSH server.
     */
    boost::shared_ptr<swish::provider::sftp_provider> Provider();

    /**
     * Get a dummy consumer to use in calls to provider.
     */
    comet::com_ptr<test::MockConsumer> Consumer();

private:
    swish::connection::authenticated_session* m_session;
};

} // namespace test
