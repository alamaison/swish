/*
    Server connection and associated operations (header)

    Copyright (C) 2006  Alexander Lamaison <awl03 (at) doc.ic.ac.uk>

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

#ifndef SERVER_H_
#define SERVER_H_

#include "swish.h"

class Server
{
	char hash[MD5_DIGEST_LEN];
	int known;
	SSH_SESSION *session;

public:
	Server(SSH_OPTIONS *options);

	SSH_SESSION* getSession() { return session; }
	int getKnownStatus() { return known; }
	char* getHash() { return hash; }
	void setHash(char* hash);

	int authenticatePassword();
	int authenticatePubKey();
	int authenticateKeyboardInteractive();
};

#endif /*SERVER_H_*/

/* $Id$ */
