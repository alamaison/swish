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

#include "ProviderFixture.hpp"

#include "test/common_boost/MockConsumer.hpp"
#include "test/common_boost/fixtures.hpp"  // SandboxFixture, OpenSshFixture

#include "swish/connection/connection_spec.hpp"
#include "swish/connection/session_manager.hpp"
#include "swish/provider/Provider.hpp" // CProvider
#include "swish/utils.hpp" // Utf8StringToWideString

#include <comet/ptr.h> // com_ptr

#include <boost/shared_ptr.hpp>

#include <string>

using swish::connection::connection_spec;
using swish::connection::session_manager;
using swish::connection::session_reservation;
using swish::provider::sftp_provider;
using swish::provider::CProvider;
using swish::utils::Utf8StringToWideString;

using comet::com_ptr;

using boost::shared_ptr;

namespace test {

shared_ptr<sftp_provider> ProviderFixture::Provider()
{
    session_reservation ticket(
        session_manager().reserve_session(
            connection_spec(
                Utf8StringToWideString(GetHost()),
                Utf8StringToWideString(GetUser()), GetPort()),
            Consumer(),
            "Running tests"));

    return boost::shared_ptr<CProvider>(new CProvider(ticket));
}

/**
 * Get a dummy consumer to use in calls to provider.
 */
com_ptr<test::MockConsumer> ProviderFixture::Consumer()
{
    com_ptr<test::MockConsumer> consumer = new test::MockConsumer();
    consumer->set_pubkey_behaviour(test::MockConsumer::CustomKeys);
    consumer->set_key_files(
        PrivateKeyPath().string(), PublicKeyPath().string());
    return consumer;
}

} // namespace test
