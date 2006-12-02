/* crypt.c */
/* it just contains the shit necessary to make blowfish-cbc work ... */
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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/blowfish.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "libssh/priv.h"

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include "libssh/crypto.h"

u32 packet_decrypt_len(SSH_SESSION *session, char *crypted){
    u32 *decrypted;
    if(session->current_crypto)
        packet_decrypt(session,crypted,session->current_crypto->in_cipher->blocksize);
    decrypted=(u32 *)crypted;
    ssh_say(3,"size decrypted : %lx\n",ntohl(*decrypted));
    return ntohl(*decrypted);
}
    
int packet_decrypt(SSH_SESSION *session, void *data,u32 len){
    struct crypto_struct *crypto=session->current_crypto->in_cipher;
    char *out=malloc(len);
    ssh_say(3,"Decrypting %d bytes data\n",len);
    crypto->set_decrypt_key(crypto,session->current_crypto->decryptkey);
    crypto->cbc_decrypt(crypto,data,out,len,session->current_crypto->decryptIV);
    memcpy(data,out,len);
    memset(out,0,len);
    free(out);
    return 0;
}
    
char * packet_encrypt(SSH_SESSION *session,void *data,u32 len){
    struct crypto_struct *crypto;
    HMAC_CTX *ctx;
    char *out;
    int finallen;
    u32 seq=ntohl(session->send_seq);
    if(!session->current_crypto)
        return NULL; /* nothing to do here */
    crypto= session->current_crypto->out_cipher;
    ssh_say(3,"seq num = %d, len = %d\n",session->send_seq,len);
    crypto->set_encrypt_key(crypto,session->current_crypto->encryptkey);
    out=malloc(len);
    ctx=hmac_init(session->current_crypto->encryptMAC,20,HMAC_SHA1);	
    hmac_update(ctx,(unsigned char *)&seq,sizeof(u32));
    hmac_update(ctx,data,len);
    hmac_final(ctx,session->current_crypto->hmacbuf,&finallen);
#ifdef DEBUG_CRYPTO
    ssh_print_hexa("mac :",data,len);
    if(finallen!=20)
        printf("Final len is %d\n",finallen);
    ssh_print_hexa("packet hmac",session->current_crypto->hmacbuf,20);
#endif
    crypto->cbc_encrypt(crypto,data,out,len,session->current_crypto->encryptIV);
    memcpy(data,out,len);
    memset(out,0,len);
    free(out);
    return session->current_crypto->hmacbuf;
}

int packet_hmac_verify(SSH_SESSION *session,BUFFER *buffer,char *mac){
    HMAC_CTX *ctx;
    unsigned char hmacbuf[EVP_MAX_MD_SIZE];
    int len;
    u32 seq=htonl(session->recv_seq);
    ctx=hmac_init(session->current_crypto->decryptMAC,20,HMAC_SHA1);
    hmac_update(ctx,(unsigned char *)&seq,sizeof(u32));
    hmac_update(ctx,buffer_get(buffer),buffer_get_len(buffer));
    hmac_final(ctx,hmacbuf,&len);
#ifdef DEBUG_CRYPTO
    ssh_print_hexa("received mac",mac,len);
    ssh_print_hexa("Computed mac",hmacbuf,len);
    ssh_print_hexa("seq",(unsigned char *)&seq,sizeof(u32));
#endif
    return memcmp(mac,hmacbuf,len);
}
