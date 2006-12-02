/* server.c */

/* No. It doesn't work yet. It's just hard to have 2 separated trees, one for releases 
 * and one for development */
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

/* from times to times, you need to serve your friends */
/* and, perhaps, ssh connections. */

#include "libssh/priv.h"

#ifdef WITH_SERVER

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include <errno.h>
#include <string.h>
#include "libssh/libssh.h"
#include "libssh/server.h"

int bind_socket() {
    struct sockaddr_in myaddr;
    int opt = 1;
    int s;

    ssh_net_init();

    s = socket(PF_INET, SOCK_STREAM, 0);
    memset(&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(2222);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(s, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
	ssh_set_error(NULL, SSH_FATAL, "%s", strerror(errno));
	return -1;
    }
    /* ok, bound */
    return s;
}

int listen_socket(int socket) {
    int i = listen(socket, 1);
    if (i < 0)
	ssh_set_error(NULL, SSH_FATAL, "listening on %d : %s",
		      strerror(errno));
    return i;
}

int accept_socket(int socket) {
    int i = accept(socket, NULL, NULL);
    if (i < 0)
	ssh_set_error(NULL, SSH_FATAL, "accepting client on socket %d : %s",
		      strerror(errno));
    return i;
}


SSH_SESSION *getserver(SSH_OPTIONS * options) {
    int socket;
    int fd;
    SSH_SESSION *session;
    socket = bind_socket();
    if (socket < 0)
        return NULL;
    if (listen_socket(socket) < 0)
        return NULL;
    fd = accept_socket(socket);
    close(socket);
    if (fd < 0) {
        return NULL;
    }
    session = malloc(sizeof(SSH_SESSION));
    memset(session, 0, sizeof(SSH_SESSION));
    session->fd = fd;
    session->options = options;
    ssh_send_banner(session);
    return session;
}

extern char *supported_methods[];
int server_set_kex(SSH_SESSION * session) {
    KEX *server = &session->server_kex;
    SSH_OPTIONS *options = session->options;
    int i;
    char *wanted;
    if (!options) {
        ssh_set_error((session->connected ? session : NULL), SSH_FATAL,
		      "Options structure is null(client's bug)");
	return -1;
    }
    memset(server,0,sizeof(KEX));
    /* the program might ask for a specific cookie to be sent. useful for server
       debugging */
    if (options->wanted_cookie)
        memcpy(server->cookie, options->wanted_cookie, 16);
    else
        ssh_get_random(server->cookie, 16);
    server->methods = malloc(10 * sizeof(char **));
    for (i = 0; i < 10; i++) {
	if (!(wanted = options->wanted_methods[i]))
	    wanted = supported_methods[i];
	server->methods[i] = wanted;
    printf("server->methods[%d]=%s\n",i,wanted);
	if (!server->methods[i]) {
	    ssh_set_error((session->connected ? session : NULL), SSH_FATAL, 
	    	"kex error : did not find algo");
	    return -1;
	}
    }
    return 0;

}

#endif /* HAVE_SERVER */
