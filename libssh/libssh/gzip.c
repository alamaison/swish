/* gzip.c */
/* include hooks for compression of packets */
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
#ifdef HAVE_LIBZ
#undef NO_GZIP
#else
#define NO_GZIP
#endif

#ifndef NO_GZIP
#include <zlib.h>
#include <string.h>
#define BLOCKSIZE 4092

static z_stream *initcompress(SSH_SESSION *session,int level){
    z_stream *stream=malloc(sizeof(z_stream));
    int status;
    memset(stream,0,sizeof(z_stream));
    status=deflateInit(stream,level);
    if (status!=0)
        ssh_set_error((session->connected?session:NULL),SSH_FATAL,"status %d inititalising zlib deflate",status);
    return stream;
}

BUFFER *gzip_compress(SSH_SESSION *session,BUFFER *source,int level){
    BUFFER *dest;
    static unsigned char out_buf[BLOCKSIZE];
    void *in_ptr=buffer_get(source);
    unsigned long in_size=buffer_get_len(source);
    unsigned long len;
    int status;
    z_stream *zout=session->current_crypto->compress_out_ctx;
    if(!zout)
        zout=session->current_crypto->compress_out_ctx=initcompress(session,level);
    dest=buffer_new();
    zout->next_out=out_buf;
    zout->next_in=in_ptr;
    zout->avail_in=in_size;
    do {
        zout->avail_out=BLOCKSIZE;
        status=deflate(zout,Z_PARTIAL_FLUSH);
        if(status !=0){
            ssh_set_error((session->connected?session:NULL),SSH_FATAL,"status %d deflating zlib packet",status);
            return NULL;
        }
        len=BLOCKSIZE-zout->avail_out;
        buffer_add_data(dest,out_buf,len);
        zout->next_out=out_buf;
    } while (zout->avail_out == 0);

    return dest;
}

int compress_buffer(SSH_SESSION *session,BUFFER *buf){
    BUFFER *dest=gzip_compress(session,buf,9);
    if(!dest)
        return -1;
    buffer_reinit(buf);
    buffer_add_data(buf,buffer_get(dest),buffer_get_len(dest));
    buffer_free(dest);
    return 0;
}

/* decompression */

static z_stream *initdecompress(SSH_SESSION *session){
    z_stream *stream=malloc(sizeof(z_stream));
    int status;
    memset(stream,0,sizeof(z_stream));
    status=inflateInit(stream);
    if (status!=0){
        ssh_set_error((session->connected?session:NULL),SSH_FATAL,"Status = %d initiating inflate context !",status);
        free(stream);
        stream=NULL;
    }
    return stream;
}

BUFFER *gzip_decompress(SSH_SESSION *session,BUFFER *source){
    BUFFER *dest;
    static unsigned char out_buf[BLOCKSIZE];
    void *in_ptr=buffer_get_rest(source);
    unsigned long in_size=buffer_get_rest_len(source);
    unsigned long len;
    int status;
    z_stream *zin=session->current_crypto->compress_in_ctx;
    if(!zin)
        zin=session->current_crypto->compress_in_ctx=initdecompress(session);
    dest=buffer_new();
    zin->next_out=out_buf;
    zin->next_in=in_ptr;
    zin->avail_in=in_size;
    do {
        zin->avail_out=BLOCKSIZE;
        status=inflate(zin,Z_PARTIAL_FLUSH);
        if(status !=Z_OK){
            ssh_set_error((session->connected?session:NULL),SSH_FATAL,"status %d inflating zlib packet",status);
            buffer_free(dest);
            return NULL;
        }
        len=BLOCKSIZE-zin->avail_out;
        buffer_add_data(dest,out_buf,len);
        zin->next_out=out_buf;
    } while (zin->avail_out == 0);

    return dest;
}

int decompress_buffer(SSH_SESSION *session,BUFFER *buf){
    BUFFER *dest=gzip_decompress(session,buf);
    buffer_reinit(buf);
    if(!dest){
        return -1; /* failed */
    }
    buffer_reinit(buf);
    buffer_add_data(buf,buffer_get(dest),buffer_get_len(dest));
    buffer_free(dest);
    return 0;
}

#endif /* NO_GZIP */
