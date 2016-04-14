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

#include "com_stream_fixture.hpp"

#include "swish/connection/authenticated_session.hpp"

#include <ssh/filesystem.hpp>
#include <ssh/stream.hpp> // fstream

#include <comet/ptr.h>    // com_ptr
#include <comet/stream.h> // adapt_stream_pointer

#include <memory>

using swish::connection::authenticated_session;

using ssh::filesystem::fstream;
using ssh::filesystem::path;

using comet::adapt_stream_pointer;
using comet::com_ptr;

using std::ios_base;
using std::make_shared;

namespace test
{
namespace fixtures
{

com_stream_fixture::com_stream_fixture() : m_path(new_file_in_sandbox())
{
}

com_ptr<IStream> com_stream_fixture::get_stream(ios_base::openmode flags)
{
    // TODO: This should not create the stream directly but should use
    // SftpDirectory.  This can happen when SftpDirectory is merged with
    // the provider project
    return adapt_stream_pointer(
        make_shared<fstream>(filesystem(), test_file().wstring(), flags),
        test_file().wstring());
}

path com_stream_fixture::test_file()
{
    return m_path;
}
}
} // namespace test::fixtures
