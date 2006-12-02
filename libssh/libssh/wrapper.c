/* wrapper.c */
/* wrapping functions for crypto functions. */
/* why a wrapper ? let's say you want to port libssh from libcrypto of openssl to libfoo */
/* you are going to spend hours to remove every references to SHA1_Update() to libfoo_sha1_update */
/* after the work is finished, you're going to have only this file to modify */
/* it's not needed to say that your modifications are welcome */

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
#include "libssh/crypto.h"
#include <string.h>
#ifdef OPENSSL_CRYPTO
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>
#ifdef HAVE_OPENSSL_AES_H
#define HAS_AES
#include <openssl/aes.h>
#endif
#ifdef HAVE_OPENSSL_BLOWFISH_H
#define HAS_BLOWFISH
#include <openssl/blowfish.h>
#endif
#if (OPENSSL_VERSION_NUMBER<0x009070000)
#define OLD_CRYPTO
#endif

SHACTX *sha1_init(){
    SHACTX *c=malloc(sizeof(SHACTX));
    SHA1_Init(c);
    return c;
}
void sha1_update(SHACTX *c, const void *data, unsigned long len){
    SHA1_Update(c,data,len);
}
void sha1_final(unsigned char *md,SHACTX *c){
    SHA1_Final(md,c);
    free(c);
}
void sha1(unsigned char *digest,int len,unsigned char *hash){
    SHA1(digest,len,hash);
}

MD5CTX *md5_init(){
    MD5CTX *c=malloc(sizeof(MD5CTX));
    MD5_Init(c);
    return c;
}
void md5_update(MD5CTX *c, const void *data, unsigned long len){
    MD5_Update(c,data,len);
}
void md5_final(unsigned char *md,MD5CTX *c){
    MD5_Final(md,c);
    free(c);
}

HMACCTX *hmac_init(const void *key, int len,int type){
    HMAC_CTX *ctx;
    ctx=malloc(sizeof(HMAC_CTX));
#ifndef OLD_CRYPTO
    HMAC_CTX_init(ctx); // openssl 0.9.7 requires it.
#endif
    switch(type){
        case HMAC_SHA1:
            HMAC_Init(ctx,key,len,EVP_sha1());
            break;
        case HMAC_MD5:
            HMAC_Init(ctx,key,len,EVP_md5());
            break;
        default:
            free(ctx);
            ctx=NULL;
        }
    return ctx;
}
void hmac_update(HMACCTX *ctx,const void *data, unsigned long len){
    HMAC_Update(ctx,data,len);
}
void hmac_final(HMACCTX *ctx,unsigned char *hashmacbuf,int *len){
   HMAC_Final(ctx,hashmacbuf,len);
#ifndef OLD_CRYPTO
   HMAC_CTX_cleanup(ctx);
#else
   HMAC_cleanup(ctx);
#endif
   free(ctx);
}

static void alloc_key(struct crypto_struct *cipher){
    cipher->key=malloc(cipher->keylen);
}

#ifdef HAS_BLOWFISH
/* the wrapper functions for blowfish */
static void blowfish_set_key(struct crypto_struct *cipher, void *key){
    if(!cipher->key){
        alloc_key(cipher);
        BF_set_key(cipher->key,16,key);
    }
}

static void blowfish_encrypt(struct crypto_struct *cipher, void *in, void *out,unsigned long len,void *IV){
    BF_cbc_encrypt(in,out,len,cipher->key,IV,BF_ENCRYPT);
}

static void blowfish_decrypt(struct crypto_struct *cipher, void *in, void *out,unsigned long len,void *IV){
    BF_cbc_encrypt(in,out,len,cipher->key,IV,BF_DECRYPT);
}
#endif
#ifdef HAS_AES
static void aes_set_encrypt_key(struct crypto_struct *cipher, void *key){
    if(!cipher->key){
        alloc_key(cipher);
        AES_set_encrypt_key(key,cipher->keysize,cipher->key);
    }
}
static void aes_set_decrypt_key(struct crypto_struct *cipher, void *key){
    if(!cipher->key){
        alloc_key(cipher);
        AES_set_decrypt_key(key,cipher->keysize,cipher->key);
    }
}
static void aes_encrypt(struct crypto_struct *cipher, void *in, void *out, unsigned long len, void *IV){
    AES_cbc_encrypt(in,out,len,cipher->key,IV,AES_ENCRYPT);
}
static void aes_decrypt(struct crypto_struct *cipher, void *in, void *out, unsigned long len, void *IV){
    AES_cbc_encrypt(in,out,len,cipher->key,IV,AES_DECRYPT);
}
#endif
/* the table of supported ciphers */
static struct crypto_struct ssh_ciphertab[]={
#ifdef HAS_BLOWFISH
    { "blowfish-cbc", 8 ,sizeof (BF_KEY),NULL,128,blowfish_set_key,blowfish_set_key,blowfish_encrypt, blowfish_decrypt},
#endif
#ifdef HAS_AES
    { "aes128-cbc",16,sizeof(AES_KEY),NULL,128,aes_set_encrypt_key,aes_set_decrypt_key,aes_encrypt,aes_decrypt},
    { "aes192-cbc",16,sizeof(AES_KEY),NULL,192,aes_set_encrypt_key,aes_set_decrypt_key,aes_encrypt,aes_decrypt},
    { "aes256-cbc",16,sizeof(AES_KEY),NULL,256,aes_set_encrypt_key,aes_set_decrypt_key,aes_encrypt,aes_decrypt},
#endif
    { NULL,0,0,NULL,0,NULL,NULL,NULL}
};
#endif /* OPENSSL_CRYPTO */

/* it allocates a new cipher structure based on its offset into the global table */
struct crypto_struct *cipher_new(int offset){
    struct crypto_struct *cipher=malloc(sizeof(struct crypto_struct));
    /* note the memcpy will copy the pointers : so, you shouldn't free them */
    memcpy(cipher,&ssh_ciphertab[offset],sizeof(*cipher));
    return cipher;
}

void cipher_free(struct crypto_struct *cipher){
    if(cipher->key){
        // destroy the key
        memset(cipher->key,0,cipher->keylen);
        free(cipher->key);
    }
    free(cipher);
}

CRYPTO *crypto_new(){
    CRYPTO *crypto=malloc(sizeof (CRYPTO));
    memset(crypto,0,sizeof(*crypto));
    return crypto;
}

void crypto_free(CRYPTO *crypto){
    if(crypto->server_pubkey)
        free(crypto->server_pubkey);
    if(crypto->in_cipher)
        cipher_free(crypto->in_cipher);
    if(crypto->out_cipher)
        cipher_free(crypto->out_cipher);
    if(crypto->e)
        bignum_free(crypto->e);
    if(crypto->f)
        bignum_free(crypto->f);
    if(crypto->x)
        bignum_free(crypto->x);
    if(crypto->k)
        bignum_free(crypto->k);
    /* lot of other things */
    /* i'm lost in my own code. good work */
    memset(crypto,0,sizeof(*crypto));
    free(crypto);
}

int crypt_set_algorithms(SSH_SESSION *session){
    /* we must scan the kex entries to find crypto algorithms and set their appropriate structure */
    int i=0;
    /* out */
    char *wanted=session->client_kex.methods[KEX_CRYPT_C_S];
    while(ssh_ciphertab[i].name && strcmp(wanted,ssh_ciphertab[i].name))
        i++;
    if(!ssh_ciphertab[i].name){
        ssh_set_error((session->connected?session:NULL),SSH_FATAL,"Crypt_set_algorithms : no crypto algorithm function found for %s",wanted);
        return -1;
    }
    ssh_say(2,"Set output algorithm %s\n",wanted);
    session->next_crypto->out_cipher=cipher_new(i);
    i=0;
    /* in */
    wanted=session->client_kex.methods[KEX_CRYPT_S_C];
    while(ssh_ciphertab[i].name && strcmp(wanted,ssh_ciphertab[i].name))
        i++;
    if(!ssh_ciphertab[i].name){
        ssh_set_error((session->connected?session:NULL),SSH_FATAL,"Crypt_set_algorithms : no crypto algorithm function found for %s",wanted);
        return -1;
    }
    ssh_say(2,"Set input algorithm %s\n",wanted);
    session->next_crypto->in_cipher=cipher_new(i);
    
    /* compression */
    if(strstr(session->client_kex.methods[KEX_COMP_C_S],"zlib"))
        session->next_crypto->do_compress_out=1;
    if(strstr(session->client_kex.methods[KEX_COMP_S_C],"zlib"))
        session->next_crypto->do_compress_in=1;
    return 0;
}
