/* keyfiles.c */
/* This part of the library handles private and public key files needed for publickey authentication,*/
/* as well as servers public hashes verifications and certifications. Lot of code here handles openssh */
/* implementations (key files aren't standardized yet). */

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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <openssl/pem.h>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include "libssh/priv.h"
#define MAXLINESIZE 80

static int default_get_password(char *buf, int size,int rwflag, char *descr){
    char *pass;
    char buffer[256];
    int len;
    snprintf(buffer,256,"Please enter passphrase for %s",descr);
#ifdef HAVE_GETPASS
    pass=getpass(buffer);
    snprintf(buf,size,"%s",buffer);
    memset(pass,0,strlen(pass));
#else
    fgets(buf, size, stdin);
    if (strlen(buf) > 0) {
        if (buf[strlen(buf) - 1] == '\n') {
             buf[strlen(buf) - 1] = '\0';
        }
    }
#endif
    len=strlen(buf);
    return len;
}

/* in case the passphrase has been given in parameter */
static int get_password_specified(char *buf,int size, int rwflag, char *password){
    snprintf(buf,size,"%s",password);
    return strlen(buf);
}

/* TODO : implement it to read both DSA and RSA at once */
PRIVATE_KEY  *privatekey_from_file(SSH_SESSION *session,char *filename,int type,char *passphrase){
    FILE *file=fopen(filename,"r");
    PRIVATE_KEY *privkey;
    DSA *dsa=NULL;
    RSA *rsa=NULL;
    if(!file){
        ssh_set_error(session,SSH_REQUEST_DENIED,"Error opening %s : %s",filename,strerror(errno));
        return NULL;
    }
    if(type==TYPE_DSS){
        if(!passphrase){
            if(session && session->options->passphrase_function)
                dsa=PEM_read_DSAPrivateKey(file,NULL, session->options->passphrase_function,"DSA private key");
            else
                dsa=PEM_read_DSAPrivateKey(file,NULL,(void *)default_get_password, "DSA private key");
        }
        else
            dsa=PEM_read_DSAPrivateKey(file,NULL,(void *)get_password_specified,passphrase);
        fclose(file);
        if(!dsa){
            ssh_set_error(session,SSH_FATAL,"parsing private key %s : %s",filename,ERR_error_string(ERR_get_error(),NULL));
        return NULL;
        }
    }
    else if (type==TYPE_RSA){
        if(!passphrase){
            if(session && session->options->passphrase_function)
                rsa=PEM_read_RSAPrivateKey(file,NULL, session->options->passphrase_function,"RSA private key");
            else
                rsa=PEM_read_RSAPrivateKey(file,NULL,(void *)default_get_password, "RSA private key");
        }
        else
            rsa=PEM_read_RSAPrivateKey(file,NULL,(void *)get_password_specified,passphrase);
        fclose(file);
        if(!rsa){
            ssh_set_error(session,SSH_FATAL,"parsing private key %s : %s",filename,ERR_error_string(ERR_get_error(),NULL));
        return NULL;
        }
    } else {
        ssh_set_error(session,SSH_FATAL,"Invalid private key type %d",type);
        return NULL;
    }    
    
    privkey=malloc(sizeof(PRIVATE_KEY));
    privkey->type=type;
    privkey->dsa_priv=dsa;
    privkey->rsa_priv=rsa;
    return privkey;
}

void private_key_free(PRIVATE_KEY *prv){
    if(prv->dsa_priv)
        DSA_free(prv->dsa_priv);
    if(prv->rsa_priv)
        RSA_free(prv->rsa_priv);
    memset(prv,0,sizeof(PRIVATE_KEY));
    free(prv);
}

STRING *publickey_from_file(char *filename,int *_type){
    BUFFER *buffer;
    int type;
    STRING *str;
    char buf[4096]; /* noone will have bigger keys that that */
                        /* where have i head that again ? */
    int fd=open(filename,O_RDONLY);
    int r;
    char *ptr;
    if(fd<0){
        ssh_set_error(NULL,SSH_INVALID_REQUEST,"nonexistent public key file");
        return NULL;
    }
    if(read(fd,buf,8)!=8){
        close(fd);
        ssh_set_error(NULL,SSH_INVALID_REQUEST,"Invalid public key file");
        return NULL;
    }
    buf[7]=0;
    if(!strcmp(buf,"ssh-dss"))
        type=TYPE_DSS;
    else if (!strcmp(buf,"ssh-rsa"))
        type=TYPE_RSA;
    else {
        close(fd);
        ssh_set_error(NULL,SSH_INVALID_REQUEST,"Invalid public key file");
        return NULL;
    }
    r=read(fd,buf,sizeof(buf)-1);
    close(fd);
    if(r<=0){
        ssh_set_error(NULL,SSH_INVALID_REQUEST,"Invalid public key file");
        return NULL;
    }
    buf[r]=0;
    ptr=strchr(buf,' ');
    if(ptr)
        *ptr=0; /* eliminates the garbage at end of file */
    buffer=base64_to_bin(buf);
    if(buffer){
        str=string_new(buffer_get_len(buffer));
        string_fill(str,buffer_get(buffer),buffer_get_len(buffer));
        buffer_free(buffer);
        if(_type)
            *_type=type;
        return str;
    } else {
        ssh_set_error(NULL,SSH_INVALID_REQUEST,"Invalid public key file");
        return NULL; /* invalid file */
    }
}


/* why recursing ? i'll explain. on top, publickey_from_next_file will be executed until NULL returned */
/* we can't return null if one of the possible keys is wrong. we must test them before getting over */
STRING *publickey_from_next_file(SSH_SESSION *session,char **pub_keys_path,char **keys_path,
                            char **privkeyfile,int *type,int *count){
    static char *home=NULL;
    char public[256];
    char private[256];
    char *priv;
    char *pub;
    STRING *pubkey;
    if(!home)
        home=ssh_get_user_home_dir();
    if(home==NULL) {
        ssh_set_error(session,SSH_FATAL,"User home dir impossible to guess");
        return NULL;
    }
    ssh_set_error(session,SSH_NO_ERROR,"no public key matched");
    if((pub=pub_keys_path[*count])==NULL)
        return NULL;
    if((priv=keys_path[*count])==NULL)
        return NULL;
    ++*count;
    /* are them readable ? */
    snprintf(public,256,pub,home);
    ssh_say(2,"Trying to open %s\n",public);
    if(!ssh_file_readaccess_ok(public)){
        ssh_say(2,"Failed\n");
        return publickey_from_next_file(session,pub_keys_path,keys_path,privkeyfile,type,count);
    } 
    snprintf(private,256,priv,home);
    ssh_say(2,"Trying to open %s\n",private);
    if(!ssh_file_readaccess_ok(private)){
        ssh_say(2,"Failed\n");
        return publickey_from_next_file(session,pub_keys_path,keys_path,privkeyfile,type,count);
    }
    ssh_say(2,"Okay both files ok\n");
    /* ok, we are sure both the priv8 and public key files are readable : we return the public one as a string,
        and the private filename in arguments */
    pubkey=publickey_from_file(public,type);
    if(!pubkey){
        ssh_say(2,"Wasn't able to open public key file %s : %s\n",public,ssh_get_error(session));
        return publickey_from_next_file(session,pub_keys_path,keys_path,privkeyfile,type,count);
    }
    *privkeyfile=realloc(*privkeyfile,strlen(private)+1);
    strcpy(*privkeyfile,private);
    return pubkey;
}

#define FOUND_OTHER ( (void *)-1)
#define FILE_NOT_FOUND ((void *)-2)
/* will return a token array containing [host,]ip keytype key */
/* NULL if no match was found, FOUND_OTHER if the match is on an other */
/* type of key (ie dsa if type was rsa) */
static char **ssh_parse_knownhost(char *filename, char *hostname, char *type){
    FILE *file=fopen(filename,"r");
    char buffer[4096];
    char *ptr;
    char **tokens;
    char **ret=NULL;
    if(!file)
        return FILE_NOT_FOUND;
    while(fgets(buffer,sizeof(buffer),file)){
        ptr=strchr(buffer,'\n');
        if(ptr) *ptr=0;
        if((ptr=strchr(buffer,'\r'))) *ptr=0;
        if(!buffer[0])
            continue; /* skip empty lines */
        tokens=space_tokenize(buffer);
        if(!tokens[0] || !tokens[1] || !tokens[2]){
            /* it should have exactly 3 tokens */
            free(tokens[0]);
            free(tokens);
            continue;
        }
        if(tokens[3]){
            /* 3 tokens only, not four */
            free(tokens[0]);
            free(tokens);
            continue;
        }
        ptr=tokens[0];
        while(*ptr==' ')
            ptr++; /* skip the initial spaces */
        /* we allow spaces or ',' to follow the hostname. It's generaly an IP */
        /* we don't care about ip, if the host key match there is no problem with ip */
        if(strncasecmp(ptr,hostname,strlen(hostname))==0){
            if(ptr[strlen(hostname)]==' ' || ptr[strlen(hostname)]=='\0' 
                    || ptr[strlen(hostname)]==','){
                if(strcasecmp(tokens[1],type)==0){
                    fclose(file);
                    return tokens;
                } else {
                    ret=FOUND_OTHER;
                }
            }
        }
        /* not the good one */
        free(tokens[0]);
        free(tokens);
    }
    fclose(file);
    /* we did not find */
    return ret;
}

/* public function to test if the server is known or not */
int ssh_is_server_known(SSH_SESSION *session){
    char *pubkey_64;
    BUFFER *pubkey_buffer;
    STRING *pubkey=session->current_crypto->server_pubkey;
    char **tokens;

    options_default_known_hosts_file(session->options);
    if(!session->options->host){
        ssh_set_error(session,SSH_FATAL,"Can't verify host in known hosts if the hostname isn't known");
        return SSH_SERVER_ERROR;
    }
    tokens=ssh_parse_knownhost(session->options->known_hosts_file,
        session->options->host,session->current_crypto->server_pubkey_type);
    if(tokens==NULL)
        return SSH_SERVER_NOT_KNOWN;
    if(tokens==FOUND_OTHER)
        return SSH_SERVER_FOUND_OTHER;
    if(tokens==FILE_NOT_FOUND){
#if 0
        ssh_set_error(session,SSH_FATAL,"verifying that server is a known host : file %s not found",session->options->known_hosts_file);
        return SSH_SERVER_ERROR;
#else
	return SSH_SERVER_KNOWN_OK;
#endif
    }
    /* ok we found some public key in known hosts file. now un-base64it */
    /* Some time, we may verify the IP address did not change. I honestly think */
    /* it's not an important matter as IP address are known not to be secure */
    /* and the crypto stuff is enough to prove the server's identity */
    pubkey_64=tokens[2];
    pubkey_buffer=base64_to_bin(pubkey_64);
    /* at this point, we may free the tokens */
    free(tokens[0]);
    free(tokens);
    if(!pubkey_buffer){
        ssh_set_error(session,SSH_FATAL,"verifying that server is a known host : base 64 error");
        return SSH_SERVER_ERROR;
    }
    if(buffer_get_len(pubkey_buffer)!=string_len(pubkey)){
        buffer_free(pubkey_buffer);
        return SSH_SERVER_KNOWN_CHANGED;
    }
    /* now test that they are identical */
    if(memcmp(buffer_get(pubkey_buffer),pubkey->string,buffer_get_len(pubkey_buffer))!=0){
        buffer_free(pubkey_buffer);
        return SSH_SERVER_KNOWN_CHANGED;
    }
    buffer_free(pubkey_buffer);
    return SSH_SERVER_KNOWN_OK;
}

int ssh_write_knownhost(SSH_SESSION *session){
    char *pubkey_64;
    STRING *pubkey=session->current_crypto->server_pubkey;
    char buffer[4096];
    FILE *file;
    options_default_known_hosts_file(session->options);
    if(!session->options->host){
        ssh_set_error(session,SSH_FATAL,"Cannot write host in known hosts if the hostname is unknown");
        return -1;
    }
    /* a = append only */
    file=fopen(session->options->known_hosts_file,"a");
    if(!file){
        ssh_set_error(session,SSH_FATAL,"Opening known host file %s for appending : %s",
        session->options->known_hosts_file,strerror(errno));
        return -1;
    }
    pubkey_64=bin_to_base64(pubkey->string,string_len(pubkey));
    snprintf(buffer,sizeof(buffer),"%s %s %s\n",session->options->host,session->current_crypto->server_pubkey_type,pubkey_64);
    free(pubkey_64);
    fwrite(buffer,strlen(buffer),1,file);
    fclose(file);
    return 0;
}
