// Copyright 2009, 2010, 2011, 2013, 2016 Alexander Lamaison

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

#include "provider_fixture.hpp"

#include "test/common_boost/MockConsumer.hpp"

#include "swish/connection/connection_spec.hpp"
#include "swish/connection/session_manager.hpp"
#include "swish/provider/Provider.hpp" // CProvider

#include <comet/ptr.h> // com_ptr

#include <boost/shared_ptr.hpp>

#include <string>

using swish::connection::connection_spec;
using swish::connection::session_manager;
using swish::connection::session_reservation;
using swish::provider::sftp_provider;
using swish::provider::CProvider;

using comet::com_ptr;

using boost::shared_ptr;

namespace test
{
namespace fixtures
{
shared_ptr<sftp_provider> provider_fixture::Provider()
{
    session_reservation ticket(session_manager().reserve_session(
        connection_spec(whost(), wuser(), port()), Consumer(),
        "Running tests"));

    return boost::shared_ptr<CProvider>(new CProvider(ticket));
}

com_ptr<test::MockConsumer> provider_fixture::Consumer()
{
    com_ptr<test::MockConsumer> consumer = new test::MockConsumer();
    consumer->set_pubkey_behaviour(test::MockConsumer::CustomKeys);
    consumer->set_key_files(private_key_path().string(),
                            public_key_path().string());
    return consumer;
}
}
} // namespace test::fixture
