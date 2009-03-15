/*  Container for both sides of the SFTP connection, producer and consumer.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
*/

#ifndef CONNECTION_H
#define CONNECTION_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SftpProvider.h" // ISftpProvider/ISftpConsumer interfaces

#include <atl.hpp>      // Common ATL setup

class CConnection
{
public:
	CConnection();
	~CConnection();

	ATL::CComPtr<ISftpProvider> spProvider;
	ATL::CComPtr<ISftpConsumer> spConsumer;
};

#endif // CONNECTION_H