/*
Copyright 2004 Aris Adamantiadis

This file is part of the SSH Library

The SSH Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version.

The SSH Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the SSH Library; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA. */

#ifndef SERVER_H
#define SERVER_H
/* the client banner doesn't say hey! look i'm a client ! */
#include "libssh/libssh.h"
#define SERVERBANNER CLIENTBANNER

int bind_socket();
int listen_socket(int s);
int accept_socket(int s);
SSH_SESSION *getserver(SSH_OPTIONS * options);
int server_set_kex(SSH_SESSION * session);

#endif
