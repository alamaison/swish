/* dh.c */
/* this file contains usefull stuff for Diffie helman algorithm against SSH 2 */
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

/* Let us resume the dh protocol. */
/* Each side computes a private prime number, x at client side, y at server side. */
/* g and n are two numbers common to every ssh software. */
/* client's public key (e) is calculated by doing */
/* e = g^x mod p */
/* client sents e to the server . */
/* the server computes his own public key, f */
/* f = g^y mod p */
/* it sents it to the client */
/* the common key K is calculated by the client by doing */
/* k = f^x mod p */
/* the server does the same with the client public key e */
/* k' = e^y mod p */
/* if everything went correctly, k and k' are equal */

#include "libssh/priv.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
#include "libssh/crypto.h"    
static unsigned char p_value[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2,
        0x21, 0x68, 0xC2, 0x34, 0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
        0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74, 0x02, 0x0B, 0xBE, 0xA6,
        0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
        0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D,
        0xF2, 0x5F, 0x14, 0x37, 0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
        0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6, 0xF4, 0x4C, 0x42, 0xE9,
        0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
        0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11,
        0x7C, 0x4B, 0x1F, 0xE6, 0x49, 0x28, 0x66, 0x51, 0xEC, 0xE6, 0x53, 0x81,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define P_LEN 128	/* Size in bytes of the p number */

static unsigned long g_int = 2 ;	/* G is defined as 2 by the ssh2 standards */
static bignum g;
static bignum p;

/* maybe it might be enhanced .... */
/* XXX Do it. */		
void ssh_get_random(void *where, int len){
	static int rndfd = -1;
	unsigned char *data = where;
	int i;

	if(rndfd < 0) {
		rndfd=open("/dev/urandom",O_RDONLY);
	}

	if(rndfd >= 0){
		read(rndfd,where,len);
		return;
	}

	for (i = 0; i < len; i++) {
		srand(getpid() + time(NULL) + rand() + len + i);
		data[i] = (int) (256.0*rand()/(RAND_MAX+1.0));
	}
}

/* it inits the values g and p which are used for DH key agreement */
void ssh_crypto_init(){
    static int init=0;
    if(!init){
        g=bignum_new();
        bignum_set_word(g,g_int);
        p=bignum_new();
        bignum_bin2bn(p_value,P_LEN,p);
        init++;
    }
}

/* prints the bignum on stderr */
void ssh_print_bignum(char *which,bignum num){
    char *hex;
    fprintf(stderr,"%s value: ",which);
    hex=bignum_bn2hex(num);
    fprintf(stderr,"%s\n",hex);
    free(hex);	
}

void ssh_print_hexa(char *descr,unsigned char *what, int len){
	int i;
	printf("%s : ",descr);
	for(i=0;i<len-1;i++)
		printf("%.2hhx:",what[i]);
	printf("%.2hhx\n",what[i]);
}

void dh_generate_x(SSH_SESSION *session){
    session->next_crypto->x=bignum_new();
    bignum_rand(session->next_crypto->x,128,0,-1);
	/* not harder than this */
#ifdef DEBUG_CRYPTO
	ssh_print_bignum("x",session->next_crypto->x);
#endif
}

void dh_generate_e(SSH_SESSION *session){
    bignum_CTX ctx=bignum_ctx_new();
    session->next_crypto->e=bignum_new();
    bignum_mod_exp(session->next_crypto->e,g,session->next_crypto->x,p,ctx);
#ifdef DEBUG_CRYPTO
    ssh_print_bignum("e",session->next_crypto->e);
#endif
    bignum_ctx_free(ctx);
}


STRING *make_bignum_string(bignum num){
    STRING *ptr;
    int pad=0;
    int len=bignum_num_bytes(num);
    int bits=bignum_num_bits(num);
    int finallen;
    /* remember if the fist bit is set, it is considered as a negative number. so 0's must be appended */
    if(!(bits%8) && bignum_is_bit_set(num,bits-1))
        pad++;
    ssh_say(3,"%d bits, %d bytes, %d padding\n",bits,len,pad);
    ptr=malloc(4 + len + pad);
    ptr->size=htonl(len+pad);
    if(pad)
        ptr->string[0]=0;
    finallen=bignum_bn2bin(num,ptr->string+pad);
    return ptr;
}

bignum make_string_bn(STRING *string){
	int len=ntohl(string->size);
	ssh_say(3,"Importing a %d bits,%d bytes object ...\n",len*8,len);
	return bignum_bin2bn(string->string,len,NULL);
}

STRING *dh_get_e(SSH_SESSION *session){
	return make_bignum_string(session->next_crypto->e);
}

void dh_import_pubkey(SSH_SESSION *session,STRING *pubkey_string){
    session->next_crypto->server_pubkey=pubkey_string;
}

void dh_import_f(SSH_SESSION *session,STRING *f_string){
    session->next_crypto->f=make_string_bn(f_string);
#ifdef DEBUG_CRYPTO
    ssh_print_bignum("f",session->next_crypto->f);
#endif
}

void dh_build_k(SSH_SESSION *session){
    bignum_CTX ctx=bignum_ctx_new();
    session->next_crypto->k=bignum_new();
    bignum_mod_exp(session->next_crypto->k,session->next_crypto->f,session->next_crypto->x,p,ctx);
#ifdef DEBUG_CRYPTO
    ssh_print_bignum("shared secret key",session->next_crypto->k);
#endif
    bignum_ctx_free(ctx);
}

static void sha_add(STRING *str,SHACTX *ctx){
    sha1_update(ctx,str,string_len(str)+4);
}

void make_sessionid(SSH_SESSION *session){
    SHACTX *ctx;
    STRING *num,*str;
    int len;
    ctx=sha1_init();

    str=string_from_char(session->clientbanner);
    sha_add(str,ctx);
    free(str);

    str=string_from_char(session->serverbanner);
    sha_add(str,ctx);
    free(str);

    buffer_add_u32(session->in_hashbuf,0);
    buffer_add_u8(session->in_hashbuf,0);
    buffer_add_u32(session->out_hashbuf,0);
    buffer_add_u8(session->out_hashbuf,0);

    len=ntohl(buffer_get_len(session->out_hashbuf));
    sha1_update(ctx,&len,4);

    sha1_update(ctx,buffer_get(session->out_hashbuf),buffer_get_len(session->out_hashbuf));
    buffer_free(session->out_hashbuf);
    session->out_hashbuf=NULL;

    len=ntohl(buffer_get_len(session->in_hashbuf));
    sha1_update(ctx,&len,4);

    sha1_update(ctx,buffer_get(session->in_hashbuf),buffer_get_len(session->in_hashbuf));
    buffer_free(session->in_hashbuf);
    session->in_hashbuf=NULL;
    sha1_update(ctx,session->next_crypto->server_pubkey,len=(string_len(session->next_crypto->server_pubkey)+4));
    num=make_bignum_string(session->next_crypto->e);
    sha1_update(ctx,num,len=(string_len(num)+4));
    free(num);
    num=make_bignum_string(session->next_crypto->f);
    sha1_update(ctx,num,len=(string_len(num)+4));
    free(num);
    num=make_bignum_string(session->next_crypto->k);
    sha1_update(ctx,num,len=(string_len(num)+4));
    free(num);
    sha1_final(session->next_crypto->session_id,ctx);

#ifdef DEBUG_CRYPTO
		printf("Session hash : ");
		ssh_print_hexa("session id",session->next_crypto->session_id,SHA_DIGEST_LENGTH);
#endif
}

void hashbufout_add_cookie(SSH_SESSION *session){
    session->out_hashbuf=buffer_new();
    buffer_add_u8(session->out_hashbuf,20);
    buffer_add_data(session->out_hashbuf,session->client_kex.cookie,16);
}


void hashbufin_add_cookie(SSH_SESSION *session,unsigned char *cookie){
    session->in_hashbuf=buffer_new();
    buffer_add_u8(session->in_hashbuf,20);
    buffer_add_data(session->in_hashbuf,cookie,16);
}

static void generate_one_key(STRING *k,char session_id[SHA_DIGEST_LENGTH],char output[SHA_DIGEST_LENGTH],char letter){
    SHACTX *ctx=sha1_init();
    sha1_update(ctx,k,string_len(k)+4);
    sha1_update(ctx,session_id,SHA_DIGEST_LENGTH);
    sha1_update(ctx,&letter,1);
    sha1_update(ctx,session_id,SHA_DIGEST_LENGTH);
    sha1_final(output,ctx);
}

void generate_session_keys(SSH_SESSION *session){
    STRING *k_string;
    SHACTX *ctx;
    k_string=make_bignum_string(session->next_crypto->k);

    /* IV */
    generate_one_key(k_string,session->next_crypto->session_id,session->next_crypto->encryptIV,'A');
    generate_one_key(k_string,session->next_crypto->session_id,session->next_crypto->decryptIV,'B');

    generate_one_key(k_string,session->next_crypto->session_id,session->next_crypto->encryptkey,'C');

    /* some ciphers need more than 20 bytes of input key */
    if(session->next_crypto->out_cipher->keylen > SHA_DIGEST_LENGTH*8){
        ctx=sha1_init();
        sha1_update(ctx,k_string,string_len(k_string)+4);
        sha1_update(ctx,session->next_crypto->session_id,SHA_DIGEST_LENGTH);
        sha1_update(ctx,session->next_crypto->encryptkey,SHA_DIGEST_LENGTH);
        sha1_final(session->next_crypto->encryptkey+SHA_DIGEST_LEN,ctx);
    }

    generate_one_key(k_string,session->next_crypto->session_id,session->next_crypto->decryptkey,'D');

    if(session->next_crypto->in_cipher->keylen > SHA_DIGEST_LENGTH*8){
        ctx=sha1_init();
        sha1_update(ctx,k_string,string_len(k_string)+4);
        sha1_update(ctx,session->next_crypto->session_id,SHA_DIGEST_LENGTH);
        sha1_update(ctx,session->next_crypto->decryptkey,SHA_DIGEST_LENGTH);
        sha1_final(session->next_crypto->decryptkey+SHA_DIGEST_LEN,ctx);
    }

    generate_one_key(k_string,session->next_crypto->session_id,session->next_crypto->encryptMAC,'E');
    generate_one_key(k_string,session->next_crypto->session_id,session->next_crypto->decryptMAC,'F');

#ifdef DEBUG_CRYPTO
    ssh_print_hexa("client->server IV",session->next_crypto->encryptIV,SHA_DIGEST_LENGTH);
    ssh_print_hexa("server->client IV",session->next_crypto->decryptIV,SHA_DIGEST_LENGTH);
    ssh_print_hexa("encryption key",session->next_crypto->encryptkey,16);
    ssh_print_hexa("decryption key",session->next_crypto->decryptkey,16);
    ssh_print_hexa("Encryption MAC",session->next_crypto->encryptMAC,SHA_DIGEST_LENGTH);
    ssh_print_hexa("Decryption MAC",session->next_crypto->decryptMAC,20);
#endif
    free(k_string);
}

int ssh_get_pubkey_hash(SSH_SESSION *session,char hash[MD5_DIGEST_LEN]){
    STRING *pubkey=session->current_crypto->server_pubkey;
    MD5CTX *ctx;
    int len=string_len(pubkey);

    ctx=md5_init();
    md5_update(ctx,pubkey->string,len);
    md5_final(hash,ctx);
    return MD5_DIGEST_LEN;
}

int pubkey_get_hash(SSH_SESSION *session, char hash[MD5_DIGEST_LEN]){
    return ssh_get_pubkey_hash(session,hash);
}

STRING *ssh_get_pubkey(SSH_SESSION *session){
    return string_copy(session->current_crypto->server_pubkey);
}

/* XXX i doubt it is still needed, or may need some fix */
static int match(char *group,char *object){
    char *ptr,*saved;
    char *end;
    ptr=strdup(group);
    saved=ptr;
    while(1){
        end=strchr(ptr,',');
        if(end)
            *end=0;
        if(!strcmp(ptr,object)){
            free(saved);
            return 0;
        }
        if(end)
            ptr=end+1;
        else{
            free(saved);
            return -1;
        }
    }
    /* not reached */
    return 1;
}

int sig_verify(PUBLIC_KEY *pubkey, SIGNATURE *signature, char *digest){
    int valid=0;
    char hash[SHA_DIGEST_LENGTH];
    sha1(digest,SHA_DIGEST_LENGTH,hash);
    switch(pubkey->type){
        case TYPE_DSS:
            valid=DSA_do_verify(hash,SHA_DIGEST_LENGTH,signature->dsa_sign,
                pubkey->dsa_pub);
            if(valid==1)
                return 0;
            if(valid==-1){
                ssh_set_error(NULL,SSH_INVALID_DATA,"DSA error : %s",ERR_error_string(ERR_get_error(),NULL));
                return -1;
            }
            ssh_set_error(NULL,SSH_NO_ERROR,"Invalid DSA signature");
            return -1;
        case TYPE_RSA:
        case TYPE_RSA1:
        valid=RSA_verify(NID_sha1,hash,SHA_DIGEST_LENGTH,
            signature->rsa_sign->string,string_len(signature->rsa_sign),pubkey->rsa_pub);
            if(valid==1)
                return 0;
            if(valid==-1){
                ssh_set_error(NULL,SSH_INVALID_DATA,"RSA error : %s",ERR_error_string(ERR_get_error(),NULL));
                return -1;
            }
            ssh_set_error(NULL,SSH_NO_ERROR,"Invalid RSA signature");
            return -1;
       default:
            ssh_set_error(NULL,SSH_INVALID_DATA,"Unknown public key type");
            return -1;
   }
return -1;
}

    
int signature_verify(SSH_SESSION *session,STRING *signature){
    PUBLIC_KEY *pubkey;
    SIGNATURE *sign;
    int err;
    if(session->options->dont_verify_hostkey){
        ssh_say(1,"Host key wasn't verified\n");
        return 0;
    }
    pubkey=publickey_from_string(session->next_crypto->server_pubkey);
    if(!pubkey)
        return -1;
    if(session->options->wanted_methods[KEX_HOSTKEY]){
         if(match(session->options->wanted_methods[KEX_HOSTKEY],pubkey->type_c)){
             ssh_set_error((session->connected?session:NULL),SSH_FATAL,"Public key from server (%s) doesn't match user preference (%s)",
             pubkey->type,session->options->wanted_methods[KEX_HOSTKEY]);
             publickey_free(pubkey);
             return -1;
         }
    }
    sign=signature_from_string(signature,pubkey,pubkey->type);
    if(!sign){
        ssh_set_error((session->connected?session:NULL),SSH_INVALID_DATA,"Invalid signature blob");
        publickey_free(pubkey);
        return -1;
    }
    ssh_say(1,"Going to verify a %s type signature\n",pubkey->type_c);
    err=sig_verify(pubkey,sign,session->next_crypto->session_id);
    signature_free(sign);
    session->next_crypto->server_pubkey_type=pubkey->type_c;
    publickey_free(pubkey);
    return err;
}
