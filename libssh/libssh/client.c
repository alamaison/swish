/* client.c file */
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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include "libssh/ssh2.h"
static void ssh_cleanup(SSH_SESSION *session);
#define set_status(opt,status) do {\
        if (opt->connect_status_function) \
            opt->connect_status_function(opt->connect_status_arg, status); \
    } while (0)
/* simply gets a banner from a socket */

char *ssh_get_banner(SSH_SESSION *session){
    char buffer[128];
    int i = 0;
    ssize_t read_ret;

    while (i < 127) {
        if((read_ret = recv(session->fd, &buffer[i], 1, 0))<=0){
            ssh_set_error(session,SSH_CONNECTION_LOST,"Remote host closed connection");
            return NULL;
        }
        if (buffer[i] == '\015')
            buffer[i] = 0;
        if (buffer[i] == '\012') {
            buffer[i] = 0;
            return strdup(buffer);
        }
    i++;
    }
    ssh_set_error(NULL,SSH_FATAL,"Too large banner");
    return NULL;
}

/* ssh_send_banner sends a SSH banner to the server */
/* TODO select a banner compatible with server version */
/* switch SSH1/1.5/2 */
/* and quit when the server is SSH1 only */

void ssh_send_banner(SSH_SESSION *session){
    char *banner=CLIENTBANNER ;
    char buffer[128];
    if(session->options->clientbanner)
        banner=session->options->clientbanner;
    session->clientbanner=strdup(banner);
    snprintf(buffer,128,"%s\r\n",session->clientbanner);
    send(session->fd,buffer,strlen(buffer),0);
}


int dh_handshake(SSH_SESSION *session){
    STRING *e,*f,*pubkey,*signature;
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,SSH2_MSG_KEXDH_INIT);
    dh_generate_x(session);
    dh_generate_e(session);
    e=dh_get_e(session);
    buffer_add_ssh_string(session->out_buffer,e);
    packet_send(session);
    free(e);
    if(packet_wait(session,SSH2_MSG_KEXDH_REPLY,1))
        return -1;
    pubkey=buffer_get_ssh_string(session->in_buffer);
    if(!pubkey){
        ssh_set_error(NULL,SSH_FATAL,"No public key in packet");
        return -1;
    }
    dh_import_pubkey(session,pubkey);
    f=buffer_get_ssh_string(session->in_buffer);
    if(!f){
        ssh_set_error(NULL,SSH_FATAL,"No F number in packet");
        return -1;
    }
    dh_import_f(session,f);
    free(f);
    if(!(signature=buffer_get_ssh_string(session->in_buffer))){
        ssh_set_error(NULL,SSH_FATAL,"No signature in packet");
        return -1;
    }

    dh_build_k(session);
    packet_wait(session,SSH2_MSG_NEWKEYS,1);
    ssh_say(2,"Got SSH_MSG_NEWKEYS\n");
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,SSH2_MSG_NEWKEYS);
    packet_send(session);
    ssh_say(2,"SSH_MSG_NEWKEYS sent\n");
    make_sessionid(session);
    /* set the cryptographic functions for the next crypto (it is needed for generate_session_keys for key lenghts) */
    if(crypt_set_algorithms(session))
        return -1;
    generate_session_keys(session);
    /* verify the host's signature. XXX do it sooner */
    if(signature_verify(session,signature)){
        free(signature);
        return -1;
    }
    free(signature);	/* forget it for now ... */
    /* once we got SSH2_MSG_NEWKEYS we can switch next_crypto and current_crypto */
    if(session->current_crypto)
        crypto_free(session->current_crypto);
    /* XXX later, include a function to change keys */
    session->current_crypto=session->next_crypto;
    session->next_crypto=crypto_new();
    return 0;
}

int ssh_service_request(SSH_SESSION *session,char *service){
    STRING *service_s;
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,SSH2_MSG_SERVICE_REQUEST);
    service_s=string_from_char(service);
    buffer_add_ssh_string(session->out_buffer,service_s);
    free(service_s);
    packet_send(session);
    ssh_say(3,"Sent SSH_MSG_SERVICE_REQUEST (service %s)\n",service);
    if(packet_wait(session,SSH2_MSG_SERVICE_ACCEPT,1)){
        ssh_set_error(session,SSH_INVALID_DATA,"did not receive SERVICE_ACCEPT");
        return -1;
    }
    ssh_say(3,"Received SSH_MSG_SERVICE_ACCEPT (service %s)\n",service);
    return 0;
}

SSH_SESSION *ssh_connect(SSH_OPTIONS *options){
  SSH_SESSION *session;
  int fd;
  if(!options){
      ssh_set_error(NULL,SSH_FATAL,"Null argument given to ssh_connect !");
      return NULL;
  }
  ssh_crypto_init();
  if(options->fd==-1 && !options->host){
      ssh_set_error(NULL,SSH_FATAL,"Hostname required");
      return NULL;
  } 
  if(options->fd != -1)
      fd=options->fd;
  else
      fd=ssh_connect_host(options->host,options->bindaddr,options->port,
          options->timeout,options->timeout_usec);    
  if(fd<0) {
      ssh_set_error(NULL,SSH_FATAL,"ssh_connect_host failed");
      return NULL;
  }
  set_status(options,0.2);
  session=ssh_session_new();
  session->fd=fd;
  session->alive=1;
      session->options=options;
  if(!(session->serverbanner=ssh_get_banner(session))){
      ssh_cleanup(session);
      ssh_set_error(NULL,SSH_FATAL,"ssh_get_banner failed");
      return NULL;
  }
  set_status(options,0.4);
  ssh_say(2,"banner : %s\n",session->serverbanner);
  ssh_send_banner(session);
  set_status(options,0.5);
  if(ssh_get_kex(session,0)){
      ssh_disconnect(session);
      ssh_set_error(NULL,SSH_FATAL,"ssh_get_kex failed");
      return NULL;
  }
  set_status(options,0.6);
  list_kex(&session->server_kex);
  if(set_kex(session)){
      ssh_disconnect(session);
      ssh_set_error(NULL,SSH_FATAL,"set_kex failed");
      return NULL;
  }
  send_kex(session,0);
  set_status(options,0.8);
  if(dh_handshake(session)){
      ssh_disconnect(session);
      ssh_set_error(NULL,SSH_FATAL,"dh_handshake failed");
      return NULL;
  }
  set_status(options,1.0);
  session->connected=1;
  return session;
}

static void ssh_cleanup(SSH_SESSION *session){
    int i;
    if(session->serverbanner)
        free(session->serverbanner);
    if(session->clientbanner)
        free(session->clientbanner);
    if(session->in_buffer)
        buffer_free(session->in_buffer);
    if(session->out_buffer)
        buffer_free(session->out_buffer);
    if(session->banner)
        free(session->banner);
    if(session->options)
        options_free(session->options);
    if(session->current_crypto)
        crypto_free(session->current_crypto);
    if(session->next_crypto)
        crypto_free(session->next_crypto);

    // delete all channels
    while(session->channels)
        channel_free(session->channels);
    if(session->client_kex.methods)
        for(i=0;i<10;i++)
            if(session->client_kex.methods[i])
                free(session->client_kex.methods[i]);
    if(session->server_kex.methods)
        for(i=0;i<10;++i)
            if(session->server_kex.methods[i])
                free(session->server_kex.methods[i]);
    free(session->client_kex.methods);
    free(session->server_kex.methods);
    memset(session,'X',sizeof(SSH_SESSION)); /* burn connection, it could hangs sensitive datas */
    free(session);
}

char *ssh_get_issue_banner(SSH_SESSION *session){
    if(!session->banner)
        return NULL;
    return string_to_char(session->banner);
}

void ssh_disconnect(SSH_SESSION *session){
    STRING *str;
    if(session->fd!= -1) {
        packet_clear_out(session);
        buffer_add_u8(session->out_buffer,SSH2_MSG_DISCONNECT);
        buffer_add_u32(session->out_buffer,htonl(SSH2_DISCONNECT_BY_APPLICATION));
        str=string_from_char("Bye Bye");
        buffer_add_ssh_string(session->out_buffer,str);
        free(str);
        packet_send(session);
        close(session->fd);
        session->fd=-1;
    }
    session->alive=0;
    ssh_cleanup(session);
}

const char *ssh_copyright(){
    return LIBSSH_VERSION " (c) 2003-2004 Aris Adamantiadis (aris@0xbadc0de.be)"
                " Distributed under the LGPL, please refer to COPYING file for informations"
                " about your rights" ;
}
