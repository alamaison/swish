/* sshd.c */
/* yet another ssh daemon (Yawn!) */
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

#include <libssh/libssh.h>
#include <libssh/server.h>
#include "libssh/priv.h"
#include <unistd.h>
#include <stdio.h>
int main(int argc, char **argv){
#ifdef WITH_SERVER
    SSH_OPTIONS *opts=ssh_getopt(&argc,argv);
    SSH_SESSION *server = getserver(opts);
    if(!server){
      printf("pwned : %s\n",ssh_get_error(NULL));
      exit(-1);
    }
    server->clientbanner = ssh_get_banner(server);
    if(!server->clientbanner){
        printf("%s\n",ssh_get_error(NULL));
        return -1;
    }
    server_set_kex(server);
    send_kex(server,1);
    if (ssh_get_kex(server,1)){
        printf("%s \n",ssh_get_error(NULL));
        return -1;
    }
    list_kex(&server->client_kex);

    printf("Key exchange complete.\n");

    while(1);
#endif
    return 0;
}
