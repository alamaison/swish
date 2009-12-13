/**
    @file

	Pool of reusuable SFTP connections.

    @if licence

    Copyright (C) 2007, 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "SftpProvider.h" // ISftpProvider/ISftpConsumer interfaces

#include <comet/ptr.h>  // com_ptr

#include <string>

class CPool
{
public:

	comet::com_ptr<ISftpProvider> GetSession(
		const comet::com_ptr<ISftpConsumer>& consumer,
		const std::wstring& host, const std::wstring& user, int port);
};
