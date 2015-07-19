/**
    @file

    Management functions for host entries saved in the registry.

    @if license

    Copyright (C) 2009, 2015  Alexander Lamaison <swish@lammy.co.uk>

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

#include "host_management.hpp"

#include "swish/debug.hpp"
#include "swish/host_folder/host_pidl.hpp" // create_host_itemid,
                                           // host_itemid_view

#include <comet/regkey.h>

#include <algorithm>
#include <stdexcept>

using swish::host_folder::create_host_itemid;
using swish::host_folder::host_itemid_view;

using washer::shell::pidl::cpidl_t;

using comet::regkey;

using std::runtime_error;
using std::transform;
using std::wstring;
using std::vector;

namespace { // private

    static const wstring CONNECTIONS_REGISTRY_KEY_NAME =
        L"Software\\Swish\\Connections";
    static const wchar_t* HOST_VALUE_NAME = L"Host";
    static const wchar_t* PORT_VALUE_NAME = L"Port";
    static const wchar_t* USER_VALUE_NAME = L"User";
    static const wchar_t* PATH_VALUE_NAME = L"Path";

    regkey get_connection_from_registry(const wstring& label)
    {
        regkey swish_connections = regkey(HKEY_CURRENT_USER).open(
            CONNECTIONS_REGISTRY_KEY_NAME);

        return swish_connections.open(label);
    }

    /**
     * Get a single connection from the registry as a PIDL.
     *
     * @pre The @c Software\\Swish\\Connections registry key exists.
     * @pre The connection is present as a subkey of the
     *      @c Software\\Swish\\Connections registry key whose name is given
     *      by @p label.
     *
     * @param label  Friendly name of the connection to load.
     *
     * @returns  A host PIDL holding the connection details.
     * @throws  com_error if the connection does not exist in the
     *                    or is corrupted.
     */
    cpidl_t GetConnectionDetailsFromRegistry(wstring label)
    {
        regkey swish_connections = regkey(HKEY_CURRENT_USER).open(
            CONNECTIONS_REGISTRY_KEY_NAME);

        regkey connection = swish_connections.open(label);

        wstring host = connection[HOST_VALUE_NAME];
        int port = connection[PORT_VALUE_NAME];
        wstring user = connection[USER_VALUE_NAME];
        wstring path = connection[PATH_VALUE_NAME];

        return create_host_itemid(host, user, path, port, label);
    }
}

namespace swish {
namespace host_folder {
namespace host_management {

/**
 * Load all the connections stored in the registry into PIDLs.
 *
 * It's possible that there aren't any connections in
 * the @c Software\\Swish\\Connections key of the registry, in which case
 * the vector is left empty.
 *
 * @returns  Vector of PIDLs containing the details of all the SFTP
 *           stored in the registry.
 * @throws  com_error if something unexpected happens such as corrupt
 *          registry structure.
 */
vector<cpidl_t> LoadConnectionsFromRegistry()
{
    vector<cpidl_t> connection_pidls;

    regkey connections = regkey(HKEY_CURRENT_USER).open(
        CONNECTIONS_REGISTRY_KEY_NAME);

    if (connections) // Legal to fail here - may be first ever connection
    {
        regkey::subkeys_type connection_collection =
            connections.enumerate().subkeys();

        transform(
            connection_collection.begin(), connection_collection.end(),
            back_inserter(connection_pidls), GetConnectionDetailsFromRegistry);
    }

    return connection_pidls;
}

/**
 * Add a host entry to the Swish connection key with the given details.
 *
 * If the connections key does not already exits (because no hosts have
 * been added yet) the key is created and the host added to it.
 */
void AddConnectionToRegistry(
    wstring label, wstring host, int port, wstring username, wstring path)
{
    DWORD key_disposition;
    regkey connection = regkey(HKEY_CURRENT_USER).create(
        CONNECTIONS_REGISTRY_KEY_NAME + L"\\" + label,
        REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, 0, &key_disposition);
    if (key_disposition == REG_OPENED_EXISTING_KEY)
    {
        BOOST_THROW_EXCEPTION(
            runtime_error("connection already exists in registry"));
    }
    else
    {
        connection[HOST_VALUE_NAME] = host;
        connection[PORT_VALUE_NAME] = port;
        connection[USER_VALUE_NAME] = username;
        connection[PATH_VALUE_NAME] = path;
    }
}

namespace {

    // TODO: move into Comet

    void delete_subkey_recursively(
        const regkey& key, const wstring& subkey_name);

    void delete_all_subkeys_recursively(const regkey& key)
    {
        regkey::subkeys_type subkeys = key.enumerate().subkeys();
        for(regkey::subkeys_type::iterator it=subkeys.begin();
            it != subkeys.end(); ++it)
        {
            delete_subkey_recursively(key, *it);
        }
    }

    void delete_subkey_recursively(
        const regkey& key, const wstring& subkey_name)
    {
        delete_all_subkeys_recursively(key.open(subkey_name));
        key.delete_subkey(subkey_name);
    }
}

/**
 * Remove a host entry from the Swish connections registry key by label.
 */
void RemoveConnectionFromRegistry(wstring label)
{
    regkey connections = regkey(HKEY_CURRENT_USER).open(
        CONNECTIONS_REGISTRY_KEY_NAME);

    delete_subkey_recursively(connections, label);
}

void RenameConnectionInRegistry(const wstring& from_label, const wstring& to_label)
{
    regkey connection = get_connection_from_registry(from_label);

    wstring host = connection[HOST_VALUE_NAME];
    int port = connection[PORT_VALUE_NAME];
    wstring user = connection[USER_VALUE_NAME];
    wstring path = connection[PATH_VALUE_NAME];

    AddConnectionToRegistry(to_label, host, port, user, path);
    RemoveConnectionFromRegistry(from_label);
}

namespace {

    class label_matches
    {
    public:

        label_matches(const wstring& label) : m_label(label) {}

        bool operator()(const cpidl_t& connection)
        {
            return host_itemid_view(connection).label() == m_label;
        }

    private:
        wstring m_label;
    };
}

/**
 * Returns whether a host entry with the given label exists in the registry.
 */
bool ConnectionExists(wstring label)
{
    if (label.size() < 1)
        return false;

    vector<cpidl_t> connections = LoadConnectionsFromRegistry();

    return find_if(
        connections.begin(), connections.end(), label_matches(label))
        != connections.end();
}

}}} // namespace swish::host_folder::host_management
