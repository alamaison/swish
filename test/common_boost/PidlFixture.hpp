/**
    @file

    Fixture for tests that manipulate remote files via PIDLs.

    @if license

    Copyright (C) 2011, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_TEST_PIDL_FIXTURE_HPP
#define SWISH_TEST_PIDL_FIXTURE_HPP
#pragma once

#include "test/common_boost/fixtures.hpp" // ComFixture
#include "test/common_boost/ProviderFixture.hpp" // ProviderFixture

#include <ssh/path.hpp>

#include <washer/shell/pidl.hpp> // apidl_t, cpidl_t

#include <comet/ptr.h> // com_ptr

#include <vector>

#include <ObjIdl.h> // IDataObject

namespace test {

class PidlFixture : public ProviderFixture, public ComFixture
{
public:

    /**
     * Return an absolute PIDL to a remote directory.
     *
     * We cheat by returning a PIDL to a HostFolder item with the
     * shortcut path set to the remote directory.
     */
    washer::shell::pidl::apidl_t directory_pidl(
        const ssh::filesystem::path& directory);

    /**
     * Return an absolute PIDL to the sandbox on the remote end.
     *
     * This is, of course, the local sandbox but the PIDL points to
     * it via Swish rather than via the local filesystem.
     */
    washer::shell::pidl::apidl_t sandbox_pidl();

    /**
     * Return pidls for all the items in the sandbox directory.
     */
    std::vector<washer::shell::pidl::cpidl_t> pidls_in_sandbox();

    /**
     * Make a DataObject to all the items in the sandbox, via the SFTP
     * connection.
     */
    comet::com_ptr<IDataObject> data_object_from_sandbox();
};

}

#endif
