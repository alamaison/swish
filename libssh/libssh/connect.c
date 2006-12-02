/* connect.c */
/* it handles connections to ssh servers */
/*
Copyright 2003 Aris Adamantiadis

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

#include "libssh/priv.h"

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/time.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <fcntl.h>
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#define FIRST_CHANNEL 42
static void sock_set_nonblocking(int sock) {
#if defined(F_SETFL) && defined(O_NONBLOCK)
    fcntl(sock,F_SETFL,O_NONBLOCK);
#endif
}
static void sock_set_blocking(int sock){
#ifdef F_SETFL
    fcntl(sock,F_SETFL,0);
#endif
}

int ssh_net_init(void) {
#ifdef __WIN32__
   static int called = 0;
   WSADATA wsaData;

   if (called) {
       return(0);
   }

   if (WSAStartup(MAKEWORD(2, 0), &wsaData)!=0) {
    return(-1);
   }
   if (wsaData.wVersion!=MAKEWORD(2, 0)) {
    /* Cleanup Winsock stuff */
    WSACleanup();
    return(-1);
   }

   called = 1;
#endif
   return(0);
}

/* connect_host connects to an IPv4 (or IPv6) host */
/* specified by its IP address or hostname. */
/* output is the file descriptor, <0 if failed. */

int ssh_connect_host(const char *host, const char *bind_addr, int port,long timeout, long usec){
    struct sockaddr_in sa;
    struct sockaddr_in bindsa;
    struct hostent *hp=NULL;
    static int count=0; /* for reentrencity */
    int s;

    ssh_net_init();

    while(++count>1)
        --count;


    sa.sin_family = AF_INET;

#ifdef HAVE_INET_ATON
   if (!inet_aton(host,&sa.sin_addr)) {
#else
#ifdef HAVE_INET_ADDR
   if ( (sa.sin_addr.s_addr=inet_addr(host) )==-1) {
#else
   {
#endif
#endif
       hp = gethostbyname(host);

       if(!hp) {
           --count;
#ifdef HAVE_HSTRERROR
             ssh_set_error(NULL,SSH_FATAL,"Failed to resolve hostname %s (%s)",host,hstrerror(h_errno));
#else
             ssh_set_error(NULL,SSH_FATAL,"Failed to resolve hostname %s (%s)",host,strerror(h_errno));
#endif
             return -1;
        }

        memcpy(&sa.sin_addr.s_addr,hp->h_addr_list[0],hp->h_length);
        sa.sin_family=hp->h_addrtype;
    }
    sa.sin_port=htons((unsigned short)port);
    --count;

    if(bind_addr){
        ssh_say(2,"resolving %s\n",bind_addr);
        hp=NULL;
        while(++count>1)
            --count;
#ifdef HAVE_GETHOSTBYADDR
        hp=gethostbyaddr(bind_addr,4,AF_INET);
#endif
#ifdef HAVE_GETHOSTBYNAME
        if(!hp)
            hp=gethostbyname(bind_addr);
#endif
        if(!hp){
            --count;
#ifdef HAVE_HSTRERROR
            ssh_set_error(NULL,SSH_FATAL,"Failed to resolve bind address %s (%s)",bind_addr,hstrerror(h_errno));
#else
            ssh_set_error(NULL,SSH_FATAL,"Failed to resolve bind address %s (%s)",bind_addr,strerror(h_errno));
#endif
            return -1;
        }
    }
    memset(&bindsa,0,sizeof(bindsa));
    /* create socket */
    s=socket(sa.sin_family,SOCK_STREAM,0);
    if(s<0){
        if(bind_addr)
            --count;
        ssh_set_error(NULL,SSH_FATAL,"socket : %s",strerror(errno));
        return s;
    }

    if(bind_addr){
        memcpy(&bindsa.sin_addr,hp->h_addr,hp->h_length);
        bindsa.sin_family=hp->h_addrtype;
        --count;
        if(bind(s,(struct sockaddr *)&bindsa,sizeof(bindsa))<0){
            ssh_set_error(NULL,SSH_FATAL,"Binding local address : %s",strerror(errno));
            close(s);
            return -1;
        }
    }
    if(timeout){
        struct timeval to;
        fd_set set;
        int ret=0;
        int len=sizeof(ret);
        to.tv_sec=timeout;
        to.tv_usec=usec;
        sock_set_nonblocking(s);
        connect(s,(struct sockaddr* )&sa,sizeof(sa));
        FD_ZERO(&set);
        FD_SET(s,&set);
        ret=select(s+1,NULL,&set,NULL,&to);
        if(ret==0){
            /* timeout */
            ssh_set_error(NULL,SSH_FATAL,"Timeout while connecting to %s:%d",host,port);
            close(s);
            return -1;
        }
        if(ret<0){
            ssh_set_error(NULL,SSH_FATAL,"Select error : %s",strerror(errno));
            close(s);
            return -1;
        }
        /* get connect(2) return code. zero means no error */
        getsockopt(s,SOL_SOCKET,SO_ERROR,&ret,&len);
        if (ret!=0){
            ssh_set_error(NULL,SSH_FATAL,"Connecting : %s",strerror(ret));
            close(s);
            return -1;
        }
        /* s is connected ? */
        ssh_say(3,"socket connected with timeout\n");
        sock_set_blocking(s);
        return s;
    }
    if(connect(s,(struct sockaddr *)&sa,sizeof(sa))< 0){
        close(s);
        ssh_set_error(NULL,SSH_FATAL,"connect: %s",strerror(errno));
        return -1;
    }
    return s;
}

/* connection_new() returns a newly allocated SSH_SESSION structure pointer */
SSH_SESSION *ssh_session_new() {
    SSH_SESSION *conn=malloc(sizeof (SSH_SESSION));
    memset(conn,0,sizeof(SSH_SESSION));
    conn->next_crypto=crypto_new();
    conn->maxchannel=FIRST_CHANNEL;
    return conn;
}


/* returns 1 if bytes are available on the stream, 0 instead */
int ssh_fd_poll(SSH_SESSION *session){
#ifdef HAVE_POLL
    struct pollfd fdset;
#else
    struct timeval sometime;
    fd_set descriptor;
#endif
    if(session->datatoread)
        return(session->datatoread);
#ifdef HAVE_POLL
    fdset.fd=session->fd;
    fdset.events=POLLHUP|POLLIN|POLLPRI;
    fdset.revents=0;
    if(poll(&fdset,1,0)==0)
        return 0;
    if(fdset.revents & (POLLHUP|POLLIN|POLLPRI))
        return (session->datatoread=1);
    return 0;
#elif defined(HAVE_SELECT)
    
    /* Set to return immediately (no blocking) */
    sometime.tv_sec = 0;
    sometime.tv_usec = 0;

    /* Set up descriptor */
    FD_ZERO(&descriptor);
    FD_SET(session->fd, &descriptor);

    /* Make the call, and listen for errors */
    if (select(session->fd + 1, &descriptor, NULL, NULL, &sometime) < 0) {
    	ssh_set_error(NULL,SSH_FATAL, "select: %s", strerror(errno));
    	return -1;
    }
    session->datatoread=FD_ISSET(session->fd,&descriptor);
    return session->datatoread;
#else
#error This system does not have poll() or select(), so ssh_fd_poll() will not work correctly
	return 0;
#endif
}

/* this function is a complete wrapper for the select syscall. it does more than wrapping ... */
int ssh_select(CHANNEL **channels, CHANNEL **outchannels, int maxfd, fd_set *readfds, struct timeval *timeout){
	struct timeval zerotime;
	fd_set localset, localset2;
	int rep = 0;
	int i, j;
	int set;
	int getc_ret;

	zerotime.tv_sec = 0;
	zerotime.tv_usec = 1;

	/* first, poll the maxfd file descriptors from the user with a zero-second timeout. they have the bigger priority */
	if (maxfd > 0) {
		memcpy(&localset, readfds, sizeof(*readfds));
		rep = select(maxfd, &localset, NULL, NULL, &zerotime);
		// catch the eventual errors
		if (rep == -1) {
			FD_ZERO(&localset);
			if (FD_ISSET(fileno(stdin), readfds)) {
				getc_ret = getc(stdin);
				if (getc_ret != EOF) {
					ungetc(getc_ret, stdin);
					FD_SET(fileno(stdin), &localset);
				}
			}
		}
	}

	/* polls every channel. */
	j = 0;
	for (i = 0; channels[i]; i++) {
		if (channel_poll(channels[i], 0) > 0) {
			outchannels[j] = channels[i];
			j++;
		} else if (channel_poll(channels[i], 1) > 0) {
			outchannels[j] = channels[i];
			j++;
		}
	}
	outchannels[j] = NULL;

	/* look into the localset for active fd */
	set = 0;
	for (i = 0; i < maxfd; i++) {
		if (FD_ISSET(i, &localset)) {
			set = 1;
			break;
		}
	}

	/* j!=0 means a channel has data */
	if ((j != 0) || (set != 0)) {
		if (maxfd > 0) {
			memcpy(readfds, &localset, sizeof(fd_set));
		}
		return 0;
	}

	/* at this point, not any channel had any data ready for reading, nor any fd had data for reading */
	memcpy(&localset, readfds, sizeof(fd_set));
	for (i = 0; channels[i]; i++) {
		if (channels[i]->session->alive) {
			FD_SET(channels[i]->session->fd, &localset);
			if (channels[i]->session->fd > (maxfd - 1)) {
				maxfd = channels[i]->session->fd + 1;
			}
		}
	}

	rep = select(maxfd, &localset, NULL, NULL, timeout);
	if (rep == -1 && errno == EINTR) {
		return SSH_EINTR; /* interrupted by a signal */
	}

	if (rep == -1) {
		/* was the error due to a libssh's Channel or from a closed descriptor from the user ? user closed descriptors have been
		caught in the first select and not closed since that moment. that case shouldn't occur at all */
		FD_ZERO(&localset);
		if (FD_ISSET(fileno(stdin), readfds)) {
			getc_ret = getc(stdin);
			if (getc_ret != EOF) {
				ungetc(getc_ret, stdin);
				FD_SET(fileno(stdin), &localset);
			}
		}
	}

	/* set the data_to_read flag on each session */
	for (i = 0; channels[i]; i++) {
		if (FD_ISSET(channels[i]->session->fd, &localset)) {
			channels[i]->session->datatoread = 1;
		}
	}

	/* now, test each channel */
	j = 0;
	for (i = 0; channels[i]; i++) {
		if (FD_ISSET(channels[i]->session->fd, &localset)) {
			if ((channel_poll(channels[i], 0) > 0) || (channel_poll(channels[i], 1) > 0)) {
				outchannels[j] = channels[i];
				j++;
			}
		}
	}

	outchannels[j] = NULL;
	FD_ZERO(&localset2);
	for (i = 0; i < maxfd; i++) {
		if (FD_ISSET(i, readfds) && FD_ISSET(i, &localset)) {
			FD_SET(i, &localset2);
		}
	}

	memcpy(readfds, &localset2, sizeof(fd_set));
	return 0;
}
