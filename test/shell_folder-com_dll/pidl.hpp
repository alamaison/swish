/**
    @file

    Custom PIDL functions for use only by tests.

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

#include "test/common/testlimits.h"

#define STRICT_TYPED_ITEMIDS ///< Better type safety for PIDLs (must be 
                             ///< before <shtypes.h> or <shlobj.h>).
#include <shlobj.h>  // For PIDLs and PIDL-handling functions

namespace test {
namespace swish {
namespace com_dll {
/**
 * PIDL-making helpers so that tests have no dependencies other than
 * the COM interfaces to CHostFolder and CRemoteFolder in swish.h.
 *
 * @note
 * These functions omit most error-checking so must only be used for tests.
 */
namespace pidl {

	PITEMID_CHILD MakeHostPidl(
		PCWSTR user, PCWSTR host, PCWSTR path, 
		USHORT port=SFTP_DEFAULT_PORT, PCWSTR label=L"");

	PITEMID_CHILD MakeRemotePidl(
		PCWSTR filename, bool fIsFolder=false, PCWSTR owner=L"", 
		PCWSTR group=L"", ULONG uUid=0, ULONG uGid=0, bool fIsLink=false,
		DWORD dwPermissions=0, ULONGLONG uSize=0,
		DATE dateModified=0, DATE dateAccessed=0);

}}}} // namespace test::swish::com_dll::pidl