/* keys handle the public key related functions */
/* decoding a public key (both rsa and dsa), decoding a signature (rsa and dsa), veryfying them */

/*
Copyright 2003,04 Aris Adamantiadis

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
#include <string.h>
#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include "libssh/priv.h"


/* Public key decoding functions */

char *ssh_type_to_char(int type){
    switch(type){
        case TYPE_DSS:
            return "ssh-dss";
        case TYPE_RSA:
        case TYPE_RSA1:
            return "ssh-rsa";
        default:
            return NULL;
    }
}

PUBLIC_KEY *publickey_make_dss(BUFFER *buffer){
    STRING *p,*q,*g,*pubkey;
    PUBLIC_KEY *key=malloc(sizeof(PUBLIC_KEY));
    key->type=TYPE_DSS;
    key->type_c="ssh-dss";
    p=buffer_get_ssh_string(buffer);
    q=buffer_get_ssh_string(buffer);
    g=buffer_get_ssh_string(buffer);
    pubkey=buffer_get_ssh_string(buffer);
    buffer_free(buffer); /* we don't need it anymore */
    if(!p || !q || !g || !pubkey){
        ssh_set_error(NULL,SSH_FATAL,"Invalid DSA public key");
        if(p)
            free(p);
        if(q)
            free(q);
        if(g)
            free(g);
        if(pubkey)
            free(pubkey);
        free(key);
        return NULL;
    }
    key->dsa_pub=DSA_new();
    key->dsa_pub->p=make_string_bn(p);
    key->dsa_pub->q=make_string_bn(q);
    key->dsa_pub->g=make_string_bn(g);
    key->dsa_pub->pub_key=make_string_bn(pubkey);
    free(p);
    free(q);
    free(g);
    free(pubkey);
    return key;
}

PUBLIC_KEY *publickey_make_rsa(BUFFER *buffer){
    STRING *e,*n;
    PUBLIC_KEY *key=malloc(sizeof(PUBLIC_KEY));
    key->type=TYPE_RSA;
    key->type_c="ssh-rsa";
    e=buffer_get_ssh_string(buffer);
    n=buffer_get_ssh_string(buffer);
    buffer_free(buffer); /* we don't need it anymore */
    if(!e || !n){
        ssh_set_error(NULL,SSH_FATAL,"Invalid RSA public key");
        if(e)
            free(e);
        if(n)
            free(n);
        free(key);
        return NULL;
    }
    key->rsa_pub=RSA_new();
    key->rsa_pub->e=make_string_bn(e);
    key->rsa_pub->n=make_string_bn(n);
#ifdef DEBUG_CRYPTO
    ssh_print_bignum("e",key->rsa_pub->e);
    ssh_print_bignum("n",key->rsa_pub->n);
#endif
    free(e);
    free(n);
    return key;
}

void publickey_free(PUBLIC_KEY *key){
    if(!key)
        return;
    switch(key->type){
        case TYPE_DSS:
            DSA_free(key->dsa_pub);
            break;
        case TYPE_RSA:
        case TYPE_RSA1:
            RSA_free(key->rsa_pub);
            break;
        default:
            break;
    }
    free(key);
}

PUBLIC_KEY *publickey_from_string(STRING *pubkey_s){
    BUFFER *tmpbuf=buffer_new();
    STRING *type_s;
    char *type;

    buffer_add_data(tmpbuf,pubkey_s->string,string_len(pubkey_s));
    type_s=buffer_get_ssh_string(tmpbuf);
    if(!type_s){
        buffer_free(tmpbuf);
        ssh_set_error(NULL,SSH_FATAL,"Invalid public key format");
        return NULL;
    }
    type=string_to_char(type_s);
    free(type_s);
    if(!strcmp(type,"ssh-dss")){
        free(type);
        return publickey_make_dss(tmpbuf);
    }
    if(!strcmp(type,"ssh-rsa")){
        free(type);
        return publickey_make_rsa(tmpbuf);
    }
    ssh_set_error(NULL,SSH_FATAL,"unknown public key protocol %s",type);
    buffer_free(tmpbuf);
    free(type);
    return NULL;
}

/* Signature decoding functions */

STRING *signature_to_string(SIGNATURE *sign){
    STRING *str;
    STRING *rs,*r,*s;
    unsigned char buffer[40];
    BUFFER *tmpbuf=buffer_new();
    STRING *tmp;
    tmp=string_from_char(ssh_type_to_char(sign->type));
    buffer_add_ssh_string(tmpbuf,tmp);
    free(tmp);
    switch(sign->type){
        case TYPE_DSS:
            r=make_bignum_string(sign->dsa_sign->r);
            s=make_bignum_string(sign->dsa_sign->s);
            rs=string_new(40);
            memset(buffer,0,40);
            memcpy(buffer,r->string+string_len(r)-20,20);
            memcpy(buffer+ 20, s->string + string_len(s) - 20, 20);
            string_fill(rs,buffer,40);
            free(r);
            free(s);
            buffer_add_ssh_string(tmpbuf,rs);
            free(rs);
            break;
        case TYPE_RSA:
        case TYPE_RSA1:
            buffer_add_ssh_string(tmpbuf,sign->rsa_sign);
            break;
    }
    str=string_new(buffer_get_len(tmpbuf));
    string_fill(str,buffer_get(tmpbuf),buffer_get_len(tmpbuf));
    buffer_free(tmpbuf);
    return str;
}

/* TODO : split this function in two so it becomes smaller */
SIGNATURE *signature_from_string(STRING *signature,PUBLIC_KEY *pubkey,int needed_type){
    DSA_SIG *sig;
    SIGNATURE *sign=malloc(sizeof(SIGNATURE));
    BUFFER *tmpbuf=buffer_new();
    STRING *rs;
    STRING *r,*s,*type_s,*e;
    int len,rsalen;
    char *type;
    buffer_add_data(tmpbuf,signature->string,string_len(signature));
    type_s=buffer_get_ssh_string(tmpbuf);
    if(!type_s){
        ssh_set_error(NULL,SSH_FATAL,"Invalid signature packet");
        buffer_free(tmpbuf);
        return NULL;
    }
    type=string_to_char(type_s);
    free(type_s);
    switch(needed_type){
        case TYPE_DSS:
            if(strcmp(type,"ssh-dss")){
                ssh_set_error(NULL,SSH_FATAL,"Invalid signature type : %s",type);
                buffer_free(tmpbuf);
                free(type);
                return NULL;
            }
            break;
        case TYPE_RSA:
            if(strcmp(type,"ssh-rsa")){
                ssh_set_error(NULL,SSH_FATAL,"Invalid signature type : %s",type);
                buffer_free(tmpbuf);
                free(type);
                return NULL;
            }
            break;
        default:
            ssh_set_error(NULL,SSH_FATAL,"Invalid signature type : %s",type);
            free(type);
            buffer_free(tmpbuf);
            return NULL;
    }
    free(type);
    switch(needed_type){
        case TYPE_DSS:
            rs=buffer_get_ssh_string(tmpbuf);
            buffer_free(tmpbuf);
            if(!rs || string_len(rs)!=40){ /* 40 is the dual signature blob len. */
                if(rs)
                    free(rs);
                return NULL;
            }
            /* we make use of strings because we have all-made functions to convert them to bignums */
            r=string_new(20);
            s=string_new(20);
            string_fill(r,rs->string,20);
            string_fill(s,rs->string+20,20);
            free(rs);
            sig=DSA_SIG_new();
            sig->r=make_string_bn(r); /* is that really portable ? Openssh's hack isn't better */
            sig->s=make_string_bn(s);
#ifdef DEBUG_CRYPTO
            ssh_print_bignum("r",sig->r);
            ssh_print_bignum("s",sig->s);
#endif
            free(r);
            free(s);
            sign->type=TYPE_DSS;
            sign->dsa_sign=sig;
            return sign;
        case TYPE_RSA:
            e=buffer_get_ssh_string(tmpbuf);
            buffer_free(tmpbuf);
            if(!e){
                free(e);
            return NULL;
            }
            len=string_len(e);
            rsalen=RSA_size(pubkey->rsa_pub);
            if(len>rsalen){
                free(e);
                free(sign);
                ssh_set_error(NULL,SSH_FATAL,"signature too big ! %d instead of %d",len,rsalen);
                return NULL;
            }
            if(len<rsalen)
                ssh_say(0,"Len %d < %d\n",len,rsalen);
            sign->type=TYPE_RSA;
            sign->rsa_sign=e;
#ifdef DEBUG_CRYPTO
            ssh_say(0,"Len : %d\n",len);
            ssh_print_hexa("rsa signature",e->string,len);
#endif
            return sign;
        default:
            return NULL;
    }
}

void signature_free(SIGNATURE *sign){
    if(!sign)
        return;
    switch(sign->type){
        case TYPE_DSS:
            DSA_SIG_free(sign->dsa_sign);
            break;
        case TYPE_RSA:
        case TYPE_RSA1:
            free(sign->rsa_sign);
            break;
        default:
            ssh_say(1,"freeing a signature with no type !\n");
    }
    free(sign);
}

/* maybe the missing function from libcrypto */
/* i think now, maybe it's a bad idea to name it has it should have be named in libcrypto */
static STRING *RSA_do_sign(void *payload,int len,RSA *privkey){
    STRING *sign;
    void *buffer=malloc(RSA_size(privkey));
    unsigned int size;
    int err;
    err=RSA_sign(NID_sha1,payload,len,buffer,&size,privkey);
    if(!err){
        free(buffer);
        return NULL;
    }
    sign=string_new(size);
    string_fill(sign,buffer,size);
    free(buffer);
    return sign;
}

STRING *ssh_do_sign(SSH_SESSION *session,BUFFER *sigbuf, PRIVATE_KEY *privatekey){
    SHACTX *ctx;
    STRING *session_str=string_new(SHA_DIGEST_LEN);
    char hash[SHA_DIGEST_LEN];
    SIGNATURE *sign;
    STRING *signature;
    string_fill(session_str,session->current_crypto->session_id,SHA_DIGEST_LENGTH);
    ctx=sha1_init();
    sha1_update(ctx,session_str,string_len(session_str)+4);
    sha1_update(ctx,buffer_get(sigbuf),buffer_get_len(sigbuf));
    sha1_final(hash,ctx);
    free(session_str);
    sign=malloc(sizeof(SIGNATURE));
    switch(privatekey->type){
        case TYPE_DSS:
            sign->dsa_sign=DSA_do_sign(hash,SHA_DIGEST_LENGTH,privatekey->dsa_priv);
            sign->rsa_sign=NULL;
            break;
        case TYPE_RSA:
            sign->rsa_sign=RSA_do_sign(hash,SHA_DIGEST_LENGTH,privatekey->rsa_priv);
            sign->dsa_sign=NULL;
            break;
    }
    sign->type=privatekey->type;
    if(!sign->dsa_sign && !sign->rsa_sign){
        ssh_set_error(session,SSH_FATAL,"Signing : openssl error");
        signature_free(sign);
        return NULL;
    }
    signature=signature_to_string(sign);
    signature_free(sign);
    return signature;
}
