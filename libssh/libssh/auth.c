/* auth.c deals with authentication methods */
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
#include "libssh/ssh2.h"
#include <string.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

static int ask_userauth(SSH_SESSION *session){
    if(session->auth_service_asked)
        return 0;
    else {
        if(ssh_service_request(session,"ssh-userauth"))
            return -1;
        else
            session->auth_service_asked++;
    }
    return 0;
    
}

static void burn(char *ptr){
    if(ptr)
        memset(ptr,'X',strlen(ptr));
}

static void string_burn(STRING *s){
    memset(s->string,'X',string_len(s));
}

static int wait_auth_status(SSH_SESSION *session,int kbdint){
    int err=SSH_AUTH_ERROR;
    int cont=1;
    STRING *can_continue;
    u8 partial=0;
    char *c_cont;
    while(cont){
        if(packet_read(session))
            break;
        if(packet_translate(session))
            break;
        switch(session->in_packet.type){
            case SSH2_MSG_USERAUTH_FAILURE:
                can_continue=buffer_get_ssh_string(session->in_buffer);
                if(!can_continue || buffer_get_u8(session->in_buffer,&partial)!=1 ){
                    ssh_set_error(session,SSH_INVALID_DATA,"invalid SSH_MSG_USERAUTH_FAILURE message");
                    return SSH_AUTH_ERROR;
                }
                c_cont=string_to_char(can_continue);
                if(partial){
                    err=SSH_AUTH_PARTIAL;
                    ssh_set_error(session,SSH_NO_ERROR,"partial success, authentications that can continue : %s",c_cont);
                }
                else {
                    err=SSH_AUTH_DENIED;
                    ssh_set_error(session,SSH_REQUEST_DENIED,"Access denied. authentications that can continue : %s",c_cont);
                }
                free(can_continue);
                free(c_cont);
                cont=0;
                break;
            case SSH2_MSG_USERAUTH_PK_OK:
                /* SSH monkeys have defined the same number for both */
                /* SSH_MSG_USERAUTH_PK_OK and SSH_MSG_USERAUTH_INFO_REQUEST */
                /* which is not really smart; */
          /*case SSH2_MSG_USERAUTH_INFO_REQUEST: */
                if(kbdint){
                    err=SSH_AUTH_INFO;
                    cont=0;
                    break;
                }
                /* continue through success */ 
            case SSH2_MSG_USERAUTH_SUCCESS:
                err=SSH_AUTH_SUCCESS;
                cont=0;
                break;
            case SSH2_MSG_USERAUTH_BANNER:
                {
                    STRING *banner=buffer_get_ssh_string(session->in_buffer);
                    if(!banner){
                        ssh_say(1,"The banner message was invalid. continuing though\n");
                        break;
                    }
                    ssh_say(2,"Received a message banner\n");
                    if(session->banner)
                        free(session->banner); /* erase the older one */
                    session->banner=banner;
                    break;
                }
            default:
                packet_parse(session);
                break;
        }
    }
    return err;
}

/* use the "none" authentication question */

int ssh_userauth_none(SSH_SESSION *session,char *username){
    STRING *user;
    STRING *service;
    STRING *method;
    if(!username)
        if(!(username=session->options->username)){
            if(options_default_username(session->options))
                return SSH_AUTH_ERROR;
            else
                username=session->options->username;
        }
    if(ask_userauth(session))
        return SSH_AUTH_ERROR;
    user=string_from_char(username);
    method=string_from_char("none");
    service=string_from_char("ssh-connection");
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,SSH2_MSG_USERAUTH_REQUEST);
    buffer_add_ssh_string(session->out_buffer,user);
    buffer_add_ssh_string(session->out_buffer,service);
    buffer_add_ssh_string(session->out_buffer,method);
    free(service);
    free(method);
    free(user);
    packet_send(session);
    return wait_auth_status(session,0);
}

int ssh_userauth_offer_pubkey(SSH_SESSION *session, char *username,int type, STRING *publickey){
    STRING *user;
    STRING *service;
    STRING *method;
    STRING *algo;
    int err=SSH_AUTH_ERROR;
    if(!username)
        if(!(username=session->options->username)){
            if(options_default_username(session->options))
                return SSH_AUTH_ERROR;
            else
                username=session->options->username;
        }
    if(ask_userauth(session))
        return SSH_AUTH_ERROR;
    user=string_from_char(username);
    service=string_from_char("ssh-connection");
    method=string_from_char("publickey");
    algo=string_from_char(ssh_type_to_char(type));

    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,SSH2_MSG_USERAUTH_REQUEST);
    buffer_add_ssh_string(session->out_buffer,user);
    buffer_add_ssh_string(session->out_buffer,service);
    buffer_add_ssh_string(session->out_buffer,method);
    buffer_add_u8(session->out_buffer,0);
    buffer_add_ssh_string(session->out_buffer,algo);
    buffer_add_ssh_string(session->out_buffer,publickey);
    packet_send(session);
    err=wait_auth_status(session,0);
    free(user);
    free(method);
    free(service);
    free(algo);
    return err;
}

int ssh_userauth_pubkey(SSH_SESSION *session, char *username, STRING *publickey, PRIVATE_KEY *privatekey){
    STRING *user;
    STRING *service;
    STRING *method;
    STRING *algo;
    STRING *sign;
    int err=SSH_AUTH_ERROR;
    if(!username)
        if(!(username=session->options->username)){
            if(options_default_username(session->options))
                return err;
            else
                username=session->options->username;
        }
    if(ask_userauth(session))
        return err;
    user=string_from_char(username);
    service=string_from_char("ssh-connection");
    method=string_from_char("publickey");
    algo=string_from_char(ssh_type_to_char(privatekey->type));
    
    
    /* we said previously the public key was accepted */
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,SSH2_MSG_USERAUTH_REQUEST);
    buffer_add_ssh_string(session->out_buffer,user);
    buffer_add_ssh_string(session->out_buffer,service);
    buffer_add_ssh_string(session->out_buffer,method);
    buffer_add_u8(session->out_buffer,1);
    buffer_add_ssh_string(session->out_buffer,algo);
    buffer_add_ssh_string(session->out_buffer,publickey);
    sign=ssh_do_sign(session,session->out_buffer,privatekey);
    if(sign){
        buffer_add_ssh_string(session->out_buffer,sign);
        free(sign);
        packet_send(session);
        err=wait_auth_status(session,0);
    }
    free(user);
    free(service);
    free(method);
    free(algo);
    return err;
}

int ssh_userauth_password(SSH_SESSION *session,char *username,char *password){
    STRING *user;
    STRING *service;
    STRING *method;
    STRING *password_s;
    int err;
    if(!username)
        if(!(username=session->options->username)){
            if(options_default_username(session->options))
                return SSH_AUTH_ERROR;
            else
                username=session->options->username;
        }
    if(ask_userauth(session))
        return SSH_AUTH_ERROR;
    user=string_from_char(username);
    service=string_from_char("ssh-connection");
    method=string_from_char("password");
    password_s=string_from_char(password);

    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,SSH2_MSG_USERAUTH_REQUEST);
    buffer_add_ssh_string(session->out_buffer,user);
    buffer_add_ssh_string(session->out_buffer,service);
    buffer_add_ssh_string(session->out_buffer,method);
    buffer_add_u8(session->out_buffer,0);
    buffer_add_ssh_string(session->out_buffer,password_s);
    free(user);
    free(service);
    free(method);
    memset(password_s,0,strlen(password)+4);
    free(password_s);
    packet_send(session);
    err=wait_auth_status(session,0);
    return err;
}

static char *keys_path[]={NULL,"%s/.ssh/identity","%s/.ssh/id_dsa","%s/.ssh/id_rsa",NULL};
static char *pub_keys_path[]={NULL,"%s/.ssh/identity.pub","%s/.ssh/id_dsa.pub","%s/.ssh/id_rsa.pub",NULL};

/* this function initialy was in the client */
/* but the fools are the ones who never change mind */
int ssh_userauth_autopubkey(SSH_SESSION *session){
    int count=1; /* bypass identity */
    int type=0;
    int err;
    STRING *pubkey;
    char *privkeyfile=NULL;
    PRIVATE_KEY *privkey;
    char *id=NULL;
    // always testing none
    err=ssh_userauth_none(session,NULL);
    if(err==SSH_AUTH_ERROR || err==SSH_AUTH_SUCCESS){
        return err;
    }
    if(session->options->identity){
        ssh_say(2,"Trying identity file %s\n",session->options->identity);
        keys_path[0]=session->options->identity;
        /* let's hope alloca exists */
        id=malloc(strlen(session->options->identity)+1 + 4);
        sprintf(id,"%s.pub",session->options->identity);
        pub_keys_path[0]=id;
        count =0;
    }
    while((pubkey=publickey_from_next_file(session,pub_keys_path,keys_path, &privkeyfile,&type,&count))){
        err=ssh_userauth_offer_pubkey(session,NULL,type,pubkey);
        if(err==SSH_AUTH_ERROR){
            if(id){
                pub_keys_path[0]=NULL;
                keys_path[0]=NULL;
                free(id);
            }
            free(pubkey);
            return err;
        } else
        if(err != SSH_AUTH_SUCCESS){
            ssh_say(2,"Public key refused by server\n");
            free(pubkey);
            continue;
        }
        /* pubkey accepted by server ! */
        privkey=privatekey_from_file(session,privkeyfile,type,NULL);
        if(!privkey){
            ssh_say(0,"Reading private key %s failed (bad passphrase ?)\n",privkeyfile);
            free(pubkey);
            continue; /* continue the loop with other pubkey */
        }
        err=ssh_userauth_pubkey(session,NULL,pubkey,privkey);
        if(err==SSH_AUTH_ERROR){
            if(id){
                pub_keys_path[0]=NULL;
                keys_path[0]=NULL;
                free(id);
            }
            free(pubkey);
            private_key_free(privkey);
            return err;
        } else
        if(err != SSH_AUTH_SUCCESS){
            ssh_say(0,"Weird : server accepted our public key but refused the signature\nit might be a bug of libssh\n");
            free(pubkey);
            private_key_free(privkey);
            continue;
        }
        /* auth success */
        ssh_say(1,"Authentication using %s success\n",privkeyfile);
        free(pubkey);
        private_key_free(privkey);
        free(privkeyfile);
        if(id){
            pub_keys_path[0]=NULL;
            keys_path[0]=NULL;
            free(id);
        }
        return SSH_AUTH_SUCCESS;
    }
    ssh_say(1,"Tried every public key, none matched\n");
    ssh_set_error(session,SSH_NO_ERROR,"no public key matched");
    if(id){
        pub_keys_path[0]=NULL;
        keys_path[0]=NULL;
        free(id);
    }

    return SSH_AUTH_DENIED;
}

static struct ssh_kbdint *kbdint_new(){
    struct ssh_kbdint *kbd=malloc(sizeof (struct ssh_kbdint));
    memset(kbd,0,sizeof(*kbd));
    return kbd;
}


static void kbdint_free(struct ssh_kbdint *kbd){
    int i,n=kbd->nprompts;
    if(kbd->name)
        free(kbd->name);
    if(kbd->instruction)
        free(kbd->instruction);
    if(kbd->prompts){
        for(i=0;i<n;++i){
            burn(kbd->prompts[i]);
            free(kbd->prompts[i]);
        }
        free(kbd->prompts);
    }
    if(kbd->answers){
        for(i=0;i<n;++i){
            burn(kbd->answers[i]);
            free(kbd->answers[i]);
        }
        free(kbd->answers);
    }
    if(kbd->echo){
        free(kbd->echo);
    }
    free(kbd);
}
    
static void kbdint_clean(struct ssh_kbdint *kbd){
    int i,n=kbd->nprompts;
    if(kbd->name){
        free(kbd->name);
        kbd->name=NULL;
    }
    if(kbd->instruction){
        free(kbd->instruction);
        kbd->instruction=NULL;
    }
    if(kbd->prompts){
        for(i=0;i<n;++i){
            burn(kbd->prompts[i]);
            free(kbd->prompts[i]);
        }
        free(kbd->prompts);
        kbd->prompts=NULL;
    }
    if(kbd->answers){
        for(i=0;i<n;++i){
            burn(kbd->answers[i]);
            free(kbd->answers[i]);
        }
        free(kbd->answers);
        kbd->answers=NULL;
    }
    if(kbd->echo){
        free(kbd->echo);
        kbd->echo=NULL;
    }
    kbd->nprompts=0;
}

/* this function sends the first packet as explained in section 3.1
 * of the draft */
static int kbdauth_init(SSH_SESSION *session,
        char *user, char *submethods){
    STRING *user_s=string_from_char(user);
    STRING *submethods_s=(submethods ? string_from_char(submethods): string_from_char(""));
    STRING *service=string_from_char("ssh-connection");
    STRING *method=string_from_char("keyboard-interactive");
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,SSH2_MSG_USERAUTH_REQUEST);
    buffer_add_ssh_string(session->out_buffer,user_s);
    buffer_add_ssh_string(session->out_buffer,service);
    buffer_add_ssh_string(session->out_buffer,method);
    buffer_add_u32(session->out_buffer,0); // language tag
    buffer_add_ssh_string(session->out_buffer,submethods_s);
    free(user_s);
    free(service);
    free(method);
    free(submethods_s);
    if(packet_send(session))
        return SSH_AUTH_ERROR;
    return wait_auth_status(session,1);
}

static int kbdauth_info_get(SSH_SESSION *session){
    STRING *name; /* name of the "asking" window showed to client */
    STRING *instruction;
    STRING *tmp;
    u32 nprompts;
    int i;
    name=buffer_get_ssh_string(session->in_buffer);
    instruction=buffer_get_ssh_string(session->in_buffer);
    tmp=buffer_get_ssh_string(session->in_buffer);
    buffer_get_u32(session->in_buffer,&nprompts);
    if(!name || !instruction || !tmp){
        if(name)
            free(name);
        if(instruction)
            free(instruction);
        // tmp must be empty if we got here
        ssh_set_error(session,SSH_FATAL,"Invalid USERAUTH_INFO_REQUEST msg");
        return SSH_AUTH_ERROR;
    }
    if(tmp)
        free(tmp); // no use
    if(!session->kbdint)
        session->kbdint=kbdint_new();
    else
        kbdint_clean(session->kbdint);
    session->kbdint->name=string_to_char(name);
    free(name);
    session->kbdint->instruction=string_to_char(instruction);
    free(instruction);
    nprompts=ntohl(nprompts);
    if(nprompts>KBDINT_MAX_PROMPT){
        ssh_set_error(session,SSH_FATAL,"Too much prompt asked from server: %lu(0x%.8lx)",nprompts,nprompts);
        return SSH_AUTH_ERROR;
    }
    session->kbdint->nprompts=nprompts;
    session->kbdint->prompts=malloc(nprompts*sizeof(char *));
    memset(session->kbdint->prompts,0,nprompts*sizeof(char *));
    session->kbdint->echo=malloc(nprompts);
    memset(session->kbdint->echo,0,nprompts);
    for(i=0;i<nprompts;++i){
        tmp=buffer_get_ssh_string(session->in_buffer);
        buffer_get_u8(session->in_buffer,&session->kbdint->echo[i]);
        if(!tmp){
            ssh_set_error(session,SSH_FATAL,"Short INFO_REQUEST packet");
            return SSH_AUTH_ERROR;
        }
        session->kbdint->prompts[i]=string_to_char(tmp);
        free(tmp);
    }
    return SSH_AUTH_INFO; /* we are not auth. but we parsed the packet */
}

/* sends challenge back to the server */
static int kbdauth_send(SSH_SESSION *session) {
    STRING *answer;
    int i;
    packet_clear_out(session);
    buffer_add_u8(session->out_buffer,SSH2_MSG_USERAUTH_INFO_RESPONSE);
    buffer_add_u32(session->out_buffer,htonl(session->kbdint->nprompts));
    for(i=0;i<session->kbdint->nprompts;++i){
        if(session->kbdint->answers[i])
            answer=string_from_char(session->kbdint->answers[i]);
        else
            answer=string_from_char("");
        buffer_add_ssh_string(session->out_buffer,answer);
        string_burn(answer);
        free(answer);
    }
    if(packet_send(session))
        return SSH_AUTH_ERROR;
    return wait_auth_status(session,1);
}

/* the heart of the whole keyboard interactive login */
int ssh_userauth_kbdint(SSH_SESSION *session,char *user,char *submethods){
    int err;
    if( !session->kbdint){
        /* first time we call. we must ask for a challenge */
        if(!user)
            if(!(user=session->options->username)){
                if(options_default_username(session->options))
                    return SSH_AUTH_ERROR;
                else
                    user=session->options->username;
            }
        if(ask_userauth(session))
            return SSH_AUTH_ERROR;
        err=kbdauth_init(session,user,submethods);
        if(err!=SSH_AUTH_INFO)
            return err; /* error or first try success */
        err=kbdauth_info_get(session);
        if(err==SSH_AUTH_ERROR){
            kbdint_free(session->kbdint);
            session->kbdint=NULL;
        }
        return err;
    }
    /* if we are at this point, it's because session->kbdint exists */
    /* it means the user has set some informations there we need to send *
     * the server. and then we need to ack the status (new questions or ok *
     * pass in */
    err=kbdauth_send(session);
    kbdint_free(session->kbdint);
    session->kbdint=NULL;
    if(err!=SSH_AUTH_INFO)
        return err;
    err=kbdauth_info_get(session);
    if(err==SSH_AUTH_ERROR){
        kbdint_free(session->kbdint);
        session->kbdint=NULL;
    }
    return err;
}

int ssh_userauth_kbdint_getnprompts(SSH_SESSION *session){
    return session->kbdint->nprompts;
}

char *ssh_userauth_kbdint_getname(SSH_SESSION *session){
    return session->kbdint->name;
}

char *ssh_userauth_kbdint_getinstruction(SSH_SESSION *session){
    return session->kbdint->instruction;
}

char *ssh_userauth_kbdint_getprompt(SSH_SESSION *session, int i, 
        char *echo){
    if(i > session->kbdint->nprompts)
        return NULL;
    if(echo)
        *echo=session->kbdint->echo[i];
    return session->kbdint->prompts[i];
}

void ssh_userauth_kbdint_setanswer(SSH_SESSION *session, unsigned int i, char *answer){
    if (i>session->kbdint->nprompts)
        return;
    if(!session->kbdint->answers){
        session->kbdint->answers=malloc(sizeof(char*)*session->kbdint->nprompts);
        memset(session->kbdint->answers,0,sizeof(char *) * session->kbdint->nprompts);
    }
    if(session->kbdint->answers[i]){
        burn(session->kbdint->answers[i]);
        free(session->kbdint->answers[i]);
    }
    session->kbdint->answers[i]=strdup(answer);
}
