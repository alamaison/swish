/**
    @file

    Management functions for host entries saved in the registry.

    @if license

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

#include "host_management.hpp"

#include "swish/atl.hpp"
#include "swish/debug.hpp"

#include <algorithm>

#include <boost/lambda/lambda.hpp>
#include <boost/numeric/conversion/cast.hpp>  // numeric_cast
#pragma warning(push)
#pragma warning(disable:4180) // qualifier applied to func type has no meaning
#include <boost/bind.hpp>
#pragma warning(pop)

using ATL::CRegKey;
using boost::numeric_cast;
using std::wstring;
using std::vector;

namespace { // private

	static const wstring CONNECTIONS_REGISTRY_KEY_NAME = 
		L"Software\\Swish\\Connections";
	static const wchar_t* HOST_VALUE_NAME = L"Host";
	static const wchar_t* PORT_VALUE_NAME = L"Port";
	static const wchar_t* USER_VALUE_NAME = L"User";
	static const wchar_t* PATH_VALUE_NAME = L"Path";

	static const int MAX_REGISTRY_LEN = 2048;

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
	 * @throws  com_error: E_FAIL if the registry key does not exist
	 *          and E_UNEXPECTED if the registry is corrupted.
	 */
	CHostItem GetConnectionDetailsFromRegistry(wstring label)
	{
		CRegKey registry;
		LSTATUS rc;

		// Open HKCU\Software\Swish\Connections\<label> registry key
		wstring key = CONNECTIONS_REGISTRY_KEY_NAME + L"\\" + label;
		rc = registry.Open(HKEY_CURRENT_USER, key.c_str());
		ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_FAIL);

		// Host
		vector<wchar_t> host(MAX_HOSTNAME_LENZ);
		if (host.size() > 0)
		{
			ULONG cch = numeric_cast<ULONG>(host.size());
			rc = registry.QueryStringValue(HOST_VALUE_NAME, &host[0], &cch);
			ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_UNEXPECTED);
		}

		// Port
		DWORD port;
		rc = registry.QueryDWORDValue(PORT_VALUE_NAME, port);
		ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_UNEXPECTED);
		ATLASSERT(port >= MIN_PORT && port <= MAX_PORT);

		// User
		vector<wchar_t> user(MAX_USERNAME_LENZ);
		if (user.size() > 0)
		{
			ULONG cch = numeric_cast<ULONG>(user.size());
			rc = registry.QueryStringValue(USER_VALUE_NAME, &user[0], &cch);
			ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_UNEXPECTED);
		}

		// Path
		vector<wchar_t> path(MAX_PATH_LENZ);
		if (path.size() > 0)
		{
			ULONG cch = numeric_cast<ULONG>(path.size());
			rc = registry.QueryStringValue(PATH_VALUE_NAME, &path[0], &cch);
			ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_UNEXPECTED);
		}

		ATLENSURE(host.size() > 0 && user.size() > 0 && path.size() > 0);

		return CHostItem(
			&user[0], &host[0], &path[0], numeric_cast<USHORT>(port), 
			&label[0]);
	}
}

namespace swish {
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
vector<CHostItem> LoadConnectionsFromRegistry()
{
	CRegKey registry;
	LSTATUS rc;
	vector<CHostItem> connections;

	rc = registry.Open(
		HKEY_CURRENT_USER, CONNECTIONS_REGISTRY_KEY_NAME.c_str());

	if (rc == ERROR_SUCCESS) // Legal to fail here - may be first ever connection
	{
		int iSubKey = 0;
		do {
			wchar_t label[MAX_LABEL_LENZ]; 
			DWORD cchLabel = MAX_LABEL_LENZ;
			rc = registry.EnumKey(iSubKey, label, &cchLabel);
			if (rc == ERROR_SUCCESS)
			{
				connections.push_back(
					GetConnectionDetailsFromRegistry(label));
			}
			iSubKey++;
			// rc may be an error for corrupted registry entries such 
			// as a label being too big.  We continue looping regardless.
		} while (rc != ERROR_NO_MORE_ITEMS);

		ATLASSERT_REPORT(rc == ERROR_NO_MORE_ITEMS, rc);
	}

	return connections; // May be empty
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
	CRegKey registry;
	LSTATUS rc;

	// Create HKCU\Software\Swish\Connections\<label> registry key
	wstring key = CONNECTIONS_REGISTRY_KEY_NAME + L"\\" + label;
	rc = registry.Create(HKEY_CURRENT_USER, key.c_str());
	ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_FAIL);

	rc = registry.SetStringValue(HOST_VALUE_NAME, host.c_str());
	ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_FAIL);

	rc = registry.SetDWORDValue(PORT_VALUE_NAME, port);
	ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_FAIL);

	rc = registry.SetStringValue(USER_VALUE_NAME, username.c_str());
	ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_FAIL);

	rc = registry.SetStringValue(PATH_VALUE_NAME, path.c_str());
	ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_FAIL);
}

/**
 * Remove a host entry from the Swish connections registry key by label.
 */
void RemoveConnectionFromRegistry(wstring label)
{
	CRegKey registry;
	LSTATUS rc;

	rc = registry.Open(
		HKEY_CURRENT_USER, CONNECTIONS_REGISTRY_KEY_NAME.c_str());
	ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_FAIL);
	
	rc = registry.RecurseDeleteKey(label.c_str());
	ATLENSURE_REPORT_THROW(rc == ERROR_SUCCESS, rc, E_FAIL);
}

/**
 * Returns whether a host entry with the given label exists in the registry.
 */
bool ConnectionExists(wstring label)
{
	if (label.size() < 1)
		return false;

	vector<CHostItem> connections = LoadConnectionsFromRegistry();

	return find_if(connections.begin(), connections.end(), 
		bind(&CHostItem::GetLabel, _1) == label.c_str()) != connections.end();

}

}} // namespace swish::host_management
