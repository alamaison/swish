/* scp.c contains the needed function to work with file transfer protocol over ssh*/
/* don't look further if you believe this is just FTP over some tunnel. It IS different */
/* This file contains code written by Nick Zitzmann */
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
#include <string.h>
#include <fcntl.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include "libssh/ssh2.h"
#include "libssh/sftp.h"
#ifndef NO_SFTP
/* here how it works : sftp commands are channeled by the ssh sftp subsystem. */
/* every packet are sent/read using a SFTP_PACKET type structure. */
/* into these packets, most of the server answers are messages having an ID and */
/* having a message specific part. it is described by SFTP_MESSAGE */
/* when reading a message, the sftp system puts it into the queue, so the process having asked for it */
/* can fetch it, while continuing to read for other messages (it is inspecified in which order messages may */
/* be sent back to the client */


/* functions */
static void sftp_packet_free(SFTP_PACKET *packet);
void sftp_enqueue(SFTP_SESSION *session, SFTP_MESSAGE *msg);
static void sftp_message_free(SFTP_MESSAGE *msg);

SFTP_SESSION *sftp_new(SSH_SESSION *session){
    SFTP_SESSION *sftp=malloc(sizeof(SFTP_SESSION));
    memset(sftp,0,sizeof(SFTP_SESSION));
    sftp->session=session;
    sftp->channel=open_session_channel(session,131000,32000);
    if(!sftp->channel){
        free(sftp);
        return NULL;
    }
    if(channel_request_sftp(sftp->channel)){
        sftp_free(sftp);
        return NULL;
    }
    return sftp;
}

void sftp_free(SFTP_SESSION *sftp){
    struct request_queue *ptr;
    channel_send_eof(sftp->channel);
    /* let libssh handle the channel closing from the server reply */
    ptr=sftp->queue;
    while(ptr){
        struct request_queue *old;
        sftp_message_free(ptr->message);
        old=ptr->next;
        free(ptr);
        ptr=old;
    }
    memset(sftp,0,sizeof(*sftp));
    free(sftp);
}

int sftp_packet_write(SFTP_SESSION *sftp,u8 type, BUFFER *payload){
    u32 size;
    buffer_add_data_begin(payload,&type,sizeof(u8));
    size=htonl(buffer_get_len(payload));
    buffer_add_data_begin(payload,&size,sizeof(u32));
    size=channel_write(sftp->channel,buffer_get(payload),buffer_get_len(payload));
    if(size != buffer_get_len(payload)){
        ssh_say(1,"had to write %d bytes, wrote only %d\n",buffer_get_len(payload),size);
    }
    return size;
}

SFTP_PACKET *sftp_packet_read(SFTP_SESSION *sftp){
    SFTP_PACKET *packet=malloc(sizeof(SFTP_PACKET));
    u32 size;
    packet->sftp=sftp;
    packet->payload=buffer_new();
    if(channel_read(sftp->channel,packet->payload,4,0)<=0){
        buffer_free(packet->payload);
        free(packet);
        return NULL;
    }
    buffer_get_u32(packet->payload,&size);
    size=ntohl(size);
    if(channel_read(sftp->channel,packet->payload,1,0)<=0){
        buffer_free(packet->payload);
        free(packet);
        return NULL;
    }
    buffer_get_u8(packet->payload,&packet->type);
    if(size>1)
        if(channel_read(sftp->channel,packet->payload,size-1,0)<=0){
            buffer_free(packet->payload);
            free(packet);
            return NULL;
        }
    return packet;
}

static SFTP_MESSAGE *sftp_message_new(){
    SFTP_MESSAGE *msg=malloc(sizeof(SFTP_MESSAGE));
    memset(msg,0,sizeof(*msg));
    msg->payload=buffer_new();
    return msg;
}

static void sftp_message_free(SFTP_MESSAGE *msg){
    if(msg->payload)
        buffer_free(msg->payload);
    free(msg);
}

SFTP_MESSAGE *sftp_get_message(SFTP_PACKET *packet){
    SFTP_MESSAGE *msg=sftp_message_new();
    msg->sftp=packet->sftp;
    msg->packet_type=packet->type;
    if((packet->type!=SSH_FXP_STATUS)&&(packet->type!=SSH_FXP_HANDLE) &&
        (packet->type != SSH_FXP_DATA) && (packet->type != SSH_FXP_ATTRS)
    && (packet->type != SSH_FXP_NAME)){
        ssh_set_error(packet->sftp->session,SSH_INVALID_DATA,"get_message : unknown packet type %d\n",packet->type);
        sftp_message_free(msg);
        return NULL;
    }
    if(buffer_get_u32(packet->payload,&msg->id)!=sizeof(u32)){
        ssh_set_error(packet->sftp->session,SSH_INVALID_DATA,"invalid packet %d : no ID",packet->type);
        sftp_message_free(msg);
        return NULL;
    }
    ssh_say(2,"packet with id %d type %d\n",msg->id,msg->packet_type);
    buffer_add_data(msg->payload,buffer_get_rest(packet->payload),buffer_get_rest_len(packet->payload));
    return msg;
}

int sftp_read_and_dispatch(SFTP_SESSION *session){
    SFTP_PACKET *packet;
    SFTP_MESSAGE *message=NULL;
    packet=sftp_packet_read(session);
    if(!packet)
        return -1; /* something nasty happened reading the packet */
    message=sftp_get_message(packet);
    sftp_packet_free(packet);
    if(!message)
        return -1;
    sftp_enqueue(session,message);
    return 0;
}

static void sftp_packet_free(SFTP_PACKET *packet){
    if(packet->payload)
        buffer_free(packet->payload);
    free(packet);
}

int sftp_init(SFTP_SESSION *sftp){
    SFTP_PACKET *packet;
    BUFFER *buffer=buffer_new();
    STRING *ext_name_s=NULL, *ext_data_s=NULL;
    char *ext_name,*ext_data;
    u32 version=htonl(LIBSFTP_VERSION);
    buffer_add_u32(buffer,version);
    sftp_packet_write(sftp,SSH_FXP_INIT,buffer);
    buffer_free(buffer);
    packet=sftp_packet_read(sftp);
    if(!packet)
        return -1;
    if(packet->type != SSH_FXP_VERSION){
        ssh_set_error(sftp->session,SSH_INVALID_DATA,"Received a %d messages instead of SSH_FXP_VERSION",packet->type);
        sftp_packet_free(packet);
        return -1;
    }
    buffer_get_u32(packet->payload,&version);
    version=ntohl(version);
    if(!(ext_name_s=buffer_get_ssh_string(packet->payload))||!(ext_data_s=buffer_get_ssh_string(packet->payload)))
        ssh_say(2,"sftp server version %d\n",version);
    else{
        ext_name=string_to_char(ext_name_s);
        ext_data=string_to_char(ext_data_s);
        ssh_say(2,"sftp server version %d (%s,%s)\n",version,ext_name,ext_data);
        free(ext_name);
        free(ext_data);
    }
    if(ext_name_s)
        free(ext_name_s);
    if(ext_data_s)
        free(ext_data_s);
    sftp_packet_free(packet);
    sftp->server_version=version;
    return 0;
}

REQUEST_QUEUE *request_queue_new(SFTP_MESSAGE *msg){
    REQUEST_QUEUE *queue=malloc(sizeof(REQUEST_QUEUE));
    memset(queue,0,sizeof(REQUEST_QUEUE));
    queue->message=msg;
    return queue;
}

void request_queue_free(REQUEST_QUEUE *queue){
    memset(queue,0,sizeof(*queue));
    free(queue);
}

void sftp_enqueue(SFTP_SESSION *session, SFTP_MESSAGE *msg){
    REQUEST_QUEUE *queue=request_queue_new(msg);
    REQUEST_QUEUE *ptr;
    ssh_say(2,"queued msg type %d id %d\n",msg->id,msg->packet_type);
    if(!session->queue)
        session->queue=queue;
    else {
        ptr=session->queue;
        while(ptr->next){
            ptr=ptr->next; /* find end of linked list */
        }
        ptr->next=queue; /* add it on bottom */
    }
}

/* pulls of a message from the queue based on the ID. returns null if no message has been found */
SFTP_MESSAGE *sftp_dequeue(SFTP_SESSION *session, u32 id){
    REQUEST_QUEUE *queue,*prev=NULL;
    SFTP_MESSAGE *msg;
    if(session->queue==NULL){
        return NULL;
    }
    queue=session->queue;
    while(queue){
        if(queue->message->id==id){
            /* remove from queue */
            if(prev==NULL){
                session->queue=queue->next;
            } else {
                prev->next=queue->next;
            }
            msg=queue->message;
            request_queue_free(queue);
            ssh_say(2,"dequeued msg id %d type %d\n",msg->id,msg->packet_type);
            return msg;
        }
        prev=queue;
        queue=queue->next;
    }
    return NULL;
}

/* assigns a new sftp ID for new requests and assures there is no collision between them. */
u32 sftp_get_new_id(SFTP_SESSION *session){
    return ++session->id_counter;
}

STATUS_MESSAGE *parse_status_msg(SFTP_MESSAGE *msg){
    STATUS_MESSAGE *status;
    if(msg->packet_type != SSH_FXP_STATUS){
        ssh_set_error(msg->sftp->session, SSH_INVALID_DATA,"Not a ssh_fxp_status message passed in !");
        return NULL;
    }
    status=malloc(sizeof(STATUS_MESSAGE));
    memset(status,0,sizeof(*status));
    status->id=msg->id;
    if( (buffer_get_u32(msg->payload,&status->status)!= 4)
     || !(status->error=buffer_get_ssh_string(msg->payload)) ||
     !(status->lang=buffer_get_ssh_string(msg->payload))){
         if(status->error)
             free(status->error);
         /* status->lang never get allocated if something failed */
         free(status);
         ssh_set_error(msg->sftp->session,SSH_INVALID_DATA,"invalid SSH_FXP_STATUS message");
         return NULL;
    }
    status->status=ntohl(status->status);
    status->errormsg=string_to_char(status->error);
    status->langmsg=string_to_char(status->lang);
    return status;
}

void status_msg_free(STATUS_MESSAGE *status){
    if(status->errormsg)
        free(status->errormsg);
    if(status->error)
        free(status->error);
    if(status->langmsg)
        free(status->langmsg);
    if(status->lang)
        free(status->lang);
    free(status);
}

SFTP_FILE *parse_handle_msg(SFTP_MESSAGE *msg){
    SFTP_FILE *file;
    if(msg->packet_type != SSH_FXP_HANDLE){
        ssh_set_error(msg->sftp->session,SSH_INVALID_DATA,"Not a ssh_fxp_handle message passed in !");
        return NULL;
    }
    file=malloc(sizeof(SFTP_FILE));
    memset(file,0,sizeof(*file));
    file->sftp=msg->sftp;
    file->handle=buffer_get_ssh_string(msg->payload);
    file->offset=0;
    file->eof=0;
    if(!file->handle){
        ssh_set_error(msg->sftp->session,SSH_INVALID_DATA,"Invalid SSH_FXP_HANDLE message");
        free(file);
        return NULL;
    }
    return file;
}

SFTP_DIR *sftp_opendir(SFTP_SESSION *sftp, char *path){
    SFTP_DIR *dir=NULL;
    SFTP_FILE *file;
    STATUS_MESSAGE *status;
    SFTP_MESSAGE *msg=NULL;
    STRING *path_s;
    BUFFER *payload=buffer_new();
    u32 id=sftp_get_new_id(sftp);
    buffer_add_u32(payload,id);
    path_s=string_from_char(path);
    buffer_add_ssh_string(payload,path_s);
    free(path_s);
    sftp_packet_write(sftp,SSH_FXP_OPENDIR,payload);
    buffer_free(payload);
    while(!msg){
        if(sftp_read_and_dispatch(sftp))
            /* something nasty has happened */
            return NULL;
        msg=sftp_dequeue(sftp,id);
    }
    switch (msg->packet_type){
        case SSH_FXP_STATUS:
            status=parse_status_msg(msg);
            sftp_message_free(msg);
            if(!status)
                return NULL;
            ssh_set_error(sftp->session,SSH_REQUEST_DENIED,"sftp server : %s",status->errormsg);
            status_msg_free(status);
            return NULL;
        case SSH_FXP_HANDLE:
            file=parse_handle_msg(msg);
            sftp_message_free(msg);
            if(file){
                dir=malloc(sizeof(SFTP_DIR));
                memset(dir,0,sizeof(*dir));
                dir->sftp=sftp;
                dir->name=strdup(path);
                dir->handle=file->handle;
                free(file);
            }
            return dir;
        default:
            ssh_set_error(sftp->session,SSH_INVALID_DATA,"Received message %d during opendir!",msg->packet_type);
            sftp_message_free(msg);
    }
    return NULL;
}

/* parse the attributes from a payload from some messages */
/* i coded it on baselines from the protocol version 4. */
/* please excuse me for the inaccuracy of the code. it isn't my fault, it's sftp draft's one */
/* this code is dead anyway ... */
/* version 4 specific code */
SFTP_ATTRIBUTES *sftp_parse_attr_4(SFTP_SESSION *sftp,BUFFER *buf,int expectnames){
    u32 flags=0;
    SFTP_ATTRIBUTES *attr=malloc(sizeof(SFTP_ATTRIBUTES));
    STRING *owner=NULL;
    STRING *group=NULL;
    int ok=0;
    memset(attr,0,sizeof(*attr));
    /* it isn't really a loop, but i use it because it's like a try..catch.. construction in C */
    do {
        if(buffer_get_u32(buf,&flags)!=4)
            break;
        flags=ntohl(flags);
        attr->flags=flags;
        if(flags & SSH_FILEXFER_ATTR_SIZE){
            if(buffer_get_u64(buf,&attr->size)!=8)
                break;
            attr->size=ntohll(attr->size);
        }
        if(flags & SSH_FILEXFER_ATTR_OWNERGROUP){
            if(!(owner=buffer_get_ssh_string(buf)))
                break;
            if(!(group=buffer_get_ssh_string(buf)))
                break;
        }
        if(flags & SSH_FILEXFER_ATTR_PERMISSIONS){
            if(buffer_get_u32(buf,&attr->permissions)!=4)
                break;
            attr->permissions=ntohl(attr->permissions);
        }
        if(flags & SSH_FILEXFER_ATTR_ACCESSTIME){
            if(buffer_get_u64(buf,&attr->atime64)!=8)
                break;
            attr->atime64=ntohll(attr->atime64);
        }
        if(flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES){
            if(buffer_get_u32(buf,&attr->atime_nseconds)!=4)
                break;
            attr->atime_nseconds=ntohl(attr->atime_nseconds);
        }
        if(flags & SSH_FILEXFER_ATTR_CREATETIME){
            if(buffer_get_u64(buf,&attr->createtime)!=8)
                break;
            attr->createtime=ntohll(attr->createtime);
        }
        if(flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES){
            if(buffer_get_u32(buf,&attr->createtime_nseconds)!=4)
                break;
            attr->createtime_nseconds=ntohl(attr->createtime_nseconds);
        }
        if(flags & SSH_FILEXFER_ATTR_MODIFYTIME){
            if(buffer_get_u64(buf,&attr->mtime64)!=8)
                break;
            attr->mtime64=ntohll(attr->mtime64);
        }
        if(flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES){
            if(buffer_get_u32(buf,&attr->mtime_nseconds)!=4)
                break;
            attr->mtime_nseconds=ntohl(attr->mtime_nseconds);
        }
        if(flags & SSH_FILEXFER_ATTR_ACL){
            if(!(attr->acl=buffer_get_ssh_string(buf)))
                break;
        }
        if (flags & SSH_FILEXFER_ATTR_EXTENDED){
            if(buffer_get_u32(buf,&attr->extended_count)!=4)
                break;
            attr->extended_count=ntohl(attr->extended_count);
            while(attr->extended_count && (attr->extended_type=buffer_get_ssh_string(buf))
                    && (attr->extended_data=buffer_get_ssh_string(buf))){
                        attr->extended_count--;
            }
            if(attr->extended_count)
                break;
        }
        ok=1;
    } while (0);
    if(!ok){
        /* break issued somewhere */
        if(owner)
            free(owner);
        if(group)
            free(group);
        if(attr->acl)
            free(attr->acl);
        if(attr->extended_type)
            free(attr->extended_type);
        if(attr->extended_data)
            free(attr->extended_data);
        free(attr);
        ssh_set_error(sftp->session,SSH_INVALID_DATA,"Invalid ATTR structure");
        return NULL;
    }
    /* everything went smoothly */
    if(owner){
        attr->owner=string_to_char(owner);
        free(owner);
    }
    if(group){
        attr->group=string_to_char(group);
        free(group);
    }
    return attr;
}

/* Version 3 code. it is the only one really supported (the draft for the 4 misses clarifications) */
/* maybe a paste of the draft is better than the code */
/*
        uint32   flags
        uint64   size           present only if flag SSH_FILEXFER_ATTR_SIZE
        uint32   uid            present only if flag SSH_FILEXFER_ATTR_UIDGID
        uint32   gid            present only if flag SSH_FILEXFER_ATTR_UIDGID
        uint32   permissions    present only if flag SSH_FILEXFER_ATTR_PERMISSIONS
        uint32   atime          present only if flag SSH_FILEXFER_ACMODTIME
        uint32   mtime          present only if flag SSH_FILEXFER_ACMODTIME
        uint32   extended_count present only if flag SSH_FILEXFER_ATTR_EXTENDED
        string   extended_type
        string   extended_data
        ...      more extended data (extended_type - extended_data pairs),
                   so that number of pairs equals extended_count              */
SFTP_ATTRIBUTES *sftp_parse_attr_3(SFTP_SESSION *sftp,BUFFER *buf,int expectname){
    u32 flags=0;
    STRING *name;
    STRING *longname;
    SFTP_ATTRIBUTES *attr=malloc(sizeof(SFTP_ATTRIBUTES));
    int ok=0;
    memset(attr,0,sizeof(*attr));
    /* it isn't really a loop, but i use it because it's like a try..catch.. construction in C */
    do {
        if(expectname){
            if(!(name=buffer_get_ssh_string(buf)))
                break;
            attr->name=string_to_char(name);
            free(name);
            ssh_say(2,"name : %s\n",attr->name);
            if(!(longname=buffer_get_ssh_string(buf)))
                break;
            attr->longname=string_to_char(longname);
            free(longname);
        }
        if(buffer_get_u32(buf,&flags)!=sizeof(u32))
            break;
        flags=ntohl(flags);
        attr->flags=flags;
        ssh_say(2,"flags : %.8lx\n",flags);
        if(flags & SSH_FILEXFER_ATTR_SIZE){
            if(buffer_get_u64(buf,&attr->size)!=sizeof(u64))
                break;
            attr->size=ntohll(attr->size);
            ssh_say(2,"size : %lld\n",attr->size);
        }
        if(flags & SSH_FILEXFER_ATTR_UIDGID){
            if(buffer_get_u32(buf,&attr->uid)!=sizeof(u32))
                break;
            if(buffer_get_u32(buf,&attr->gid)!=sizeof(u32))
                break;
            attr->uid=ntohl(attr->uid);
            attr->gid=ntohl(attr->gid);
        }
        if(flags & SSH_FILEXFER_ATTR_PERMISSIONS){
            if(buffer_get_u32(buf,&attr->permissions)!=sizeof(u32))
                break;
            attr->permissions=ntohl(attr->permissions);
        }
        if(flags & SSH_FILEXFER_ATTR_ACMODTIME){
            if(buffer_get_u32(buf,&attr->atime)!=sizeof(u32))
                break;
            attr->atime=ntohl(attr->atime);
            if(buffer_get_u32(buf,&attr->mtime)!=sizeof(u32))
                break;
            attr->mtime=ntohl(attr->mtime);
        }
        if (flags & SSH_FILEXFER_ATTR_EXTENDED){
            if(buffer_get_u32(buf,&attr->extended_count)!=sizeof(u32))
                break;
            attr->extended_count=ntohl(attr->extended_count);
            while(attr->extended_count && (attr->extended_type=buffer_get_ssh_string(buf))
                    && (attr->extended_data=buffer_get_ssh_string(buf))){
                        attr->extended_count--;
            }
            if(attr->extended_count)
                break;
        }
        ok=1;
    } while (0);
    if(!ok){
        /* break issued somewhere */
        if(attr->name)
            free(attr->name);
        if(attr->extended_type)
            free(attr->extended_type);
        if(attr->extended_data)
            free(attr->extended_data);
        free(attr);
        ssh_set_error(sftp->session,SSH_INVALID_DATA,"Invalid ATTR structure");
        return NULL;
    }
    /* everything went smoothly */
    return attr;
}

void buffer_add_attributes(BUFFER *buffer, SFTP_ATTRIBUTES *attr){
	u32 flags=(attr?attr->flags:0);
	flags &= (SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_UIDGID | SSH_FILEXFER_ATTR_PERMISSIONS | SSH_FILEXFER_ATTR_ACMODTIME);
	buffer_add_u32(buffer,htonl(flags));
	if(attr){
		if (flags & SSH_FILEXFER_ATTR_SIZE)
		{
			buffer_add_u64(buffer, htonll(attr->size));
		}
		if(flags & SSH_FILEXFER_ATTR_UIDGID){
			buffer_add_u32(buffer,htonl(attr->uid));
			buffer_add_u32(buffer,htonl(attr->gid));
		}
		if(flags & SSH_FILEXFER_ATTR_PERMISSIONS){
			buffer_add_u32(buffer,htonl(attr->permissions));
		}
		if (flags & SSH_FILEXFER_ATTR_ACMODTIME)
		{
			buffer_add_u32(buffer, htonl(attr->atime));
			buffer_add_u32(buffer, htonl(attr->mtime));
		}
	}
}

    
SFTP_ATTRIBUTES *sftp_parse_attr(SFTP_SESSION *session, BUFFER *buf,int expectname){
    switch(session->server_version){
        case 4:
            return sftp_parse_attr_4(session,buf,expectname);
        case 3:
            return sftp_parse_attr_3(session,buf,expectname);
        default:
            ssh_set_error(session->session,SSH_INVALID_DATA,"Version %d unsupported by client",session->server_version);
            return NULL;
    }
    return NULL;
}

int sftp_server_version(SFTP_SESSION *sftp){
    return sftp->server_version;
}

SFTP_ATTRIBUTES *sftp_readdir(SFTP_SESSION *sftp, SFTP_DIR *dir){
    BUFFER *payload;
    u32 id;
    SFTP_MESSAGE *msg=NULL;
    STATUS_MESSAGE *status;
    SFTP_ATTRIBUTES *attr;
    if(!dir->buffer){
        payload=buffer_new();
        id=sftp_get_new_id(sftp);
        buffer_add_u32(payload,id);
        buffer_add_ssh_string(payload,dir->handle);
        sftp_packet_write(sftp,SSH_FXP_READDIR,payload);
        buffer_free(payload);
        ssh_say(2,"sent a ssh_fxp_readdir with id %d\n",id);
        while(!msg){
            if(sftp_read_and_dispatch(sftp))
                /* something nasty has happened */
                return NULL;
            msg=sftp_dequeue(sftp,id);
        }
        switch (msg->packet_type){
            case SSH_FXP_STATUS:
                status=parse_status_msg(msg);
                sftp_message_free(msg);
                if(!status)
                    return NULL;
                if(status->status==SSH_FX_EOF){
                    dir->eof=1;
                    status_msg_free(status);
                    return NULL;
                }
                ssh_set_error(sftp->session,SSH_INVALID_DATA,"Unknown error status : %d",status->status);
                status_msg_free(status);
                return NULL;
            case SSH_FXP_NAME:
                buffer_get_u32(msg->payload,&dir->count);
                dir->count=ntohl(dir->count);
                dir->buffer=msg->payload;
                msg->payload=NULL;
                sftp_message_free(msg);
                break;
            default:
                ssh_set_error(sftp->session,SSH_INVALID_DATA,"unsupported message back %d",msg->packet_type);
                sftp_message_free(msg);
                return NULL;
        }
    }
    /* now dir->buffer contains a buffer and dir->count != 0 */
    if(dir->count==0){
        ssh_set_error(sftp->session,SSH_INVALID_DATA,"Count of files sent by the server is zero, which is invalid, or libsftp bug");
        return NULL;
    }
    ssh_say(2,"Count is %d\n",dir->count);
    attr=sftp_parse_attr(sftp,dir->buffer,1);
    dir->count--;
    if(dir->count==0){
        buffer_free(dir->buffer);
        dir->buffer=NULL;
    }
    return attr;
}

int sftp_dir_eof(SFTP_DIR *dir){
    return (dir->eof);
}

void sftp_attributes_free(SFTP_ATTRIBUTES *file){
    if(file->name)
        free(file->name);
    if(file->longname)
        free(file->longname);
    if(file->acl)
        free(file->acl);
    if(file->extended_data)
        free(file->extended_data);
    if(file->extended_type)
        free(file->extended_type);
    if(file->group)
        free(file->group);
    if(file->owner)
        free(file->owner);
    free(file);
}

static int sftp_handle_close(SFTP_SESSION *sftp, STRING *handle){
    SFTP_MESSAGE *msg=NULL;
    STATUS_MESSAGE *status;
    int id=sftp_get_new_id(sftp);
    int err=0;
    BUFFER *buffer=buffer_new();
    buffer_add_u32(buffer,id);
    buffer_add_ssh_string(buffer,handle);
    sftp_packet_write(sftp,SSH_FXP_CLOSE,buffer);
    buffer_free(buffer);
    while(!msg){
        if(sftp_read_and_dispatch(sftp))
        /* something nasty has happened */
            return -1;
        msg=sftp_dequeue(sftp,id);
    }
    switch (msg->packet_type){
        case SSH_FXP_STATUS:
            status=parse_status_msg(msg);
            sftp_message_free(msg);
            if(!status)
                return -1;
            if(status->status != SSH_FX_OK){
                ssh_set_error(sftp->session,SSH_REQUEST_DENIED,"sftp server : %s",status->errormsg);
                err=-1;
            }
            status_msg_free(status);
            return err;
        default:
            ssh_set_error(sftp->session,SSH_INVALID_DATA,"Received message %d during sftp_handle_close!",msg->packet_type);
            sftp_message_free(msg);
    }
    return -1;
}

int sftp_file_close(SFTP_FILE *file){
    int err=0;
    if(file->name)
        free(file->name);
    if(file->handle){
        err=sftp_handle_close(file->sftp,file->handle);
        free(file->handle);
    }
    free(file);
    return err;
}

int sftp_dir_close(SFTP_DIR *dir){
    int err=0;
    if(dir->name)
        free(dir->name);
    if(dir->handle){
        err=sftp_handle_close(dir->sftp,dir->handle);
        free(dir->handle);
    }
    if(dir->buffer)
        buffer_free(dir->buffer);
    free(dir);
    return err;
}

SFTP_FILE *sftp_open(SFTP_SESSION *sftp, char *file, int access, SFTP_ATTRIBUTES *attr){
    SFTP_FILE *handle;
    SFTP_MESSAGE *msg=NULL;
    STATUS_MESSAGE *status;
    u32 flags=0;
    u32 id=sftp_get_new_id(sftp);
    BUFFER *buffer=buffer_new();
    STRING *filename;
    if(access & O_RDONLY)
        flags|=SSH_FXF_READ;
    if(access & O_WRONLY)
        flags |= SSH_FXF_WRITE;
    if(access & O_RDWR)
        flags|=(SSH_FXF_WRITE | SSH_FXF_READ);
    if(access & O_CREAT)
        flags |=SSH_FXF_CREAT;
    if(access & O_TRUNC)
        flags |=SSH_FXF_TRUNC;
    if(access & O_EXCL)
        flags |= SSH_FXF_EXCL;
    buffer_add_u32(buffer,id);
    filename=string_from_char(file);
    buffer_add_ssh_string(buffer,filename);
    free(filename);
    buffer_add_u32(buffer,htonl(flags));
    buffer_add_attributes(buffer,attr);
    sftp_packet_write(sftp,SSH_FXP_OPEN,buffer);
    buffer_free(buffer);
    while(!msg){
        if(sftp_read_and_dispatch(sftp))
        /* something nasty has happened */
            return NULL;
        msg=sftp_dequeue(sftp,id);
    }
    switch (msg->packet_type){
        case SSH_FXP_STATUS:
            status=parse_status_msg(msg);
            sftp_message_free(msg);
            if(!status)
                return NULL;
            ssh_set_error(sftp->session,SSH_REQUEST_DENIED,"sftp server : %s",status->errormsg);
            status_msg_free(status);
            return NULL;
        case SSH_FXP_HANDLE:
            handle=parse_handle_msg(msg);
            sftp_message_free(msg);
            return handle;
        default:
            ssh_set_error(sftp->session,SSH_INVALID_DATA,"Received message %d during open!",msg->packet_type);
            sftp_message_free(msg);
    }
    return NULL; 
}

void sftp_file_set_nonblocking(SFTP_FILE *handle){
    handle->nonblocking=1;
}
void sftp_file_set_blocking(SFTP_FILE *handle){
    handle->nonblocking=0;
}

int sftp_read(SFTP_FILE *handle, void *data, int len){
    SFTP_MESSAGE *msg=NULL;
    STATUS_MESSAGE *status;
    SFTP_SESSION *sftp=handle->sftp;
    STRING *datastring;
    int id;
    int err=0;
    BUFFER *buffer;
    if(handle->eof)
        return 0; 
    buffer=buffer_new();
    id=sftp_get_new_id(handle->sftp);
    buffer_add_u32(buffer,id);
    buffer_add_ssh_string(buffer,handle->handle);
    buffer_add_u64(buffer,htonll(handle->offset));
    buffer_add_u32(buffer,htonl(len));
    sftp_packet_write(handle->sftp,SSH_FXP_READ,buffer);
    buffer_free(buffer);
    while(!msg){
        if (handle->nonblocking){
            if(channel_poll(handle->sftp->channel,0)==0){
                /* we cannot block */
                return 0;
            }
        }
        if(sftp_read_and_dispatch(handle->sftp))
        /* something nasty has happened */
            return -1;
        msg=sftp_dequeue(handle->sftp,id);
    }
    switch (msg->packet_type){
        case SSH_FXP_STATUS:
            status=parse_status_msg(msg);
            sftp_message_free(msg);
            if(!status)
                return -1;
            if(status->status != SSH_FX_EOF){
                ssh_set_error(sftp->session,SSH_REQUEST_DENIED,"sftp server : %s",status->errormsg);
                err=-1;
            }
            else
                handle->eof=1;
            status_msg_free(status);
            return err?err:0;
        case SSH_FXP_DATA:
            datastring=buffer_get_ssh_string(msg->payload);
            sftp_message_free(msg);
            if(!datastring){
                ssh_set_error(sftp->session,SSH_INVALID_DATA,"Received invalid DATA packet from sftp server");
                return -1;
            }
            if(string_len(datastring)>len){
                ssh_set_error(sftp->session,SSH_INVALID_DATA,"Received a too big DATA packet from sftp server : %d and asked for %d",
                    string_len(datastring),len);
                free(datastring);
                return -1;
            }
            len=string_len(datastring);
            handle->offset+=len;
            memcpy(data,datastring->string,len);
            free(datastring);
            return len;
        default:
            ssh_set_error(sftp->session,SSH_INVALID_DATA,"Received message %d during read!",msg->packet_type);
            sftp_message_free(msg);
            return -1;
    }
    return -1; /* not reached */
}

int sftp_write(SFTP_FILE *file, void *data, int len){
    SFTP_MESSAGE *msg=NULL;
    STATUS_MESSAGE *status;
    STRING *datastring;
    SFTP_SESSION *sftp=file->sftp;
    int id;
    int err=0;
    BUFFER *buffer;
    buffer=buffer_new();
    id=sftp_get_new_id(file->sftp);
    buffer_add_u32(buffer,id);
    buffer_add_ssh_string(buffer,file->handle);
    buffer_add_u64(buffer,htonll(file->offset));
    datastring=string_new(len);
    string_fill(datastring,data,len);
    buffer_add_ssh_string(buffer,datastring);
    free(datastring);
    if(sftp_packet_write(file->sftp,SSH_FXP_WRITE,buffer) != buffer_get_len(buffer)){
        ssh_say(1,"sftp_packet_write did not write as much data as expected\n");
    }
    buffer_free(buffer);
    while(!msg){
        if(sftp_read_and_dispatch(file->sftp))
        /* something nasty has happened */
            return -1;
        msg=sftp_dequeue(file->sftp,id);
    }
    switch (msg->packet_type){
        case SSH_FXP_STATUS:
            status=parse_status_msg(msg);
            sftp_message_free(msg);
            if(!status)
                return -1;
            if(status->status != SSH_FX_OK){
                ssh_set_error(sftp->session,SSH_REQUEST_DENIED,"sftp server : %s",status->errormsg);
                err=-1;
            }
            file->offset+=len;
            status_msg_free(status);
            return (err?err:len);
        default:
            ssh_set_error(sftp->session,SSH_INVALID_DATA,"Received message %d during write!",msg->packet_type);
            sftp_message_free(msg);
            return -1;
    }
    return -1; /* not reached */
}

void sftp_seek(SFTP_FILE *file, int new_offset){
    file->offset=new_offset;
}

unsigned long sftp_tell(SFTP_FILE *file){
    return file->offset;
}

void sftp_rewind(SFTP_FILE *file){
    file->offset=0;
}

/* code written by Nick */
int sftp_rm(SFTP_SESSION *sftp, char *file) {
    u32 id = sftp_get_new_id(sftp);
    BUFFER *buffer = buffer_new();
    STRING *filename = string_from_char(file);
    SFTP_MESSAGE *msg = NULL;
    STATUS_MESSAGE *status = NULL;

    buffer_add_u32(buffer, id);
    buffer_add_ssh_string(buffer, filename);
    free(filename);
    sftp_packet_write(sftp, SSH_FXP_REMOVE, buffer);
    buffer_free(buffer);
    while (!msg) {
        if (sftp_read_and_dispatch(sftp)) {
            return -1;
        }
        msg = sftp_dequeue(sftp, id);
    }
    if (msg->packet_type == SSH_FXP_STATUS) {
         /* by specification, this command's only supposed to return SSH_FXP_STATUS */    
         status = parse_status_msg(msg);
         sftp_message_free(msg);
         if (!status)
             return -1;
         if (status->status != SSH_FX_OK) {
          /* status should be SSH_FX_OK if the command was successful, if it didn't, then there was an error */
              ssh_set_error(sftp->session,SSH_REQUEST_DENIED, "sftp server: %s", status->errormsg);
              status_msg_free(status);
              return -1;
         }
         status_msg_free(status);
         return 0;   /* at this point, everything turned out OK */
    } else {
        ssh_set_error(sftp->session,SSH_INVALID_DATA, "Received message %d when attempting to remove file", msg->packet_type);
        sftp_message_free(msg);
    }
    return -1;
}

/* code written by Nick */
int sftp_rmdir(SFTP_SESSION *sftp, char *directory) {
    u32 id = sftp_get_new_id(sftp);
    BUFFER *buffer = buffer_new();
    STRING *filename = string_from_char(directory);
    SFTP_MESSAGE *msg = NULL;
    STATUS_MESSAGE *status = NULL;

    buffer_add_u32(buffer, id);
    buffer_add_ssh_string(buffer, filename);
    free(filename);
    sftp_packet_write(sftp, SSH_FXP_RMDIR, buffer);
    buffer_free(buffer);
    while (!msg) {
        if (sftp_read_and_dispatch(sftp))
        {
            return -1;
        }
        msg = sftp_dequeue(sftp, id);
    }
    if (msg->packet_type == SSH_FXP_STATUS) /* by specification, this command's only supposed to return SSH_FXP_STATUS */
    {
        status = parse_status_msg(msg);
        sftp_message_free(msg);
        if (!status)
        {
            return -1;
        }
        else if (status->status != SSH_FX_OK)   /* status should be SSH_FX_OK if the command was successful, if it didn't, then there was an error */
        {
            ssh_set_error(sftp->session,SSH_REQUEST_DENIED, "sftp server: %s", status->errormsg);
            status_msg_free(status);
            return -1;
        }
        status_msg_free(status);
        return 0;   /* at this point, everything turned out OK */
    }
    else
    {
        ssh_set_error(sftp->session,SSH_INVALID_DATA, "Received message %d when attempting to remove directory", msg->packet_type);
        sftp_message_free(msg);
    }
    return -1;
}

/* Code written by Nick */
int sftp_mkdir(SFTP_SESSION *sftp, char *directory, SFTP_ATTRIBUTES *attr) {
    u32 id = sftp_get_new_id(sftp);
    BUFFER *buffer = buffer_new();
    STRING *path = string_from_char(directory);
    SFTP_MESSAGE *msg = NULL;
    STATUS_MESSAGE *status = NULL;

    buffer_add_u32(buffer, id);
    buffer_add_ssh_string(buffer, path);
    free(path);
    buffer_add_attributes(buffer, attr);
    sftp_packet_write(sftp, SSH_FXP_MKDIR, buffer);
    buffer_free(buffer);
    while (!msg) {
        if (sftp_read_and_dispatch(sftp))
            return -1;
        msg = sftp_dequeue(sftp, id);
    }
    if (msg->packet_type == SSH_FXP_STATUS) {
        /* by specification, this command's only supposed to return SSH_FXP_STATUS */
        status = parse_status_msg(msg);
        sftp_message_free(msg);
        if (!status)
            return -1;
        else
        if (status->status != SSH_FX_OK) {
            /* status should be SSH_FX_OK if the command was successful, if it didn't, then there was an error */
            ssh_set_error(sftp->session,SSH_REQUEST_DENIED, "sftp server: %s", status->errormsg);
            status_msg_free(status);
            return -1;
        }
        status_msg_free(status);
        return 0;   /* at this point, everything turned out OK */
    } else {
       ssh_set_error(sftp->session,SSH_INVALID_DATA, "Received message %d when attempting to make directory", msg->packet_type);
       sftp_message_free(msg);
    }
    return -1;
}

/* code written by nick */
int sftp_rename(SFTP_SESSION *sftp, char *original, char *newname) {
    u32 id = sftp_get_new_id(sftp);
    BUFFER *buffer = buffer_new();
    STRING *oldpath = string_from_char(original);
    STRING *newpath = string_from_char(newname);
    SFTP_MESSAGE *msg = NULL;
    STATUS_MESSAGE *status = NULL;

    buffer_add_u32(buffer, id);
    buffer_add_ssh_string(buffer, oldpath);
    free(oldpath);
    buffer_add_ssh_string(buffer, newpath);
    free(newpath);
    sftp_packet_write(sftp, SSH_FXP_RENAME, buffer);
    buffer_free(buffer);
    while (!msg) {
        if (sftp_read_and_dispatch(sftp))
            return -1;
        msg = sftp_dequeue(sftp, id);
    }
    if (msg->packet_type == SSH_FXP_STATUS) {
         /* by specification, this command's only supposed to return SSH_FXP_STATUS */
        status = parse_status_msg(msg);
        sftp_message_free(msg);
        if (!status)
            return -1;
        else if (status->status != SSH_FX_OK) {
               /* status should be SSH_FX_OK if the command was successful, if it didn't, then there was an error */
            ssh_set_error(sftp->session,SSH_REQUEST_DENIED, "sftp server: %s", status->errormsg);
            status_msg_free(status);
            return -1;
        }
        status_msg_free(status);
        return 0;   /* at this point, everything turned out OK */
    } else {
        ssh_set_error(sftp->session,SSH_INVALID_DATA, "Received message %d when attempting to rename", msg->packet_type);
        sftp_message_free(msg);
    }
    return -1;
}

/* Code written by Nick */
int sftp_setstat(SFTP_SESSION *sftp, char *file, SFTP_ATTRIBUTES *attr) {
    u32 id = sftp_get_new_id(sftp);
    BUFFER *buffer = buffer_new();
    STRING *path = string_from_char(file);
    SFTP_MESSAGE *msg = NULL;
    STATUS_MESSAGE *status = NULL;

    buffer_add_u32(buffer, id);
    buffer_add_ssh_string(buffer, path);
    free(path);
    buffer_add_attributes(buffer, attr);
    sftp_packet_write(sftp, SSH_FXP_SETSTAT, buffer);
    buffer_free(buffer);
    while (!msg) {
        if (sftp_read_and_dispatch(sftp))
            return -1;
        msg = sftp_dequeue(sftp, id);
    }
    if (msg->packet_type == SSH_FXP_STATUS) {
         /* by specification, this command's only supposed to return SSH_FXP_STATUS */
        status = parse_status_msg(msg);
        sftp_message_free(msg);
        if (!status)
            return -1;
        else if (status->status != SSH_FX_OK) {
               /* status should be SSH_FX_OK if the command was successful, if it didn't, then there was an error */
            ssh_set_error(sftp->session,SSH_REQUEST_DENIED, "sftp server: %s", status->errormsg);
            status_msg_free(status);
            return -1;
        }
        status_msg_free(status);
        return 0;   /* at this point, everything turned out OK */
    } else {
        ssh_set_error(sftp->session,SSH_INVALID_DATA, "Received message %d when attempting to set stats", msg->packet_type);
        sftp_message_free(msg);
    }
    return -1;
}

/* another code written by Nick */
char *sftp_canonicalize_path(SFTP_SESSION *sftp, char *path)
{
	u32 id = sftp_get_new_id(sftp);
	BUFFER *buffer = buffer_new();
	STRING *pathstr = string_from_char(path);
	STRING *name = NULL;
	SFTP_MESSAGE *msg = NULL;
	STATUS_MESSAGE *status = NULL;
	char *cname;
	u32 ignored;
	
	buffer_add_u32(buffer, id);
	buffer_add_ssh_string(buffer, pathstr);
	free(pathstr);
	sftp_packet_write(sftp, SSH_FXP_REALPATH, buffer);
	buffer_free(buffer);
	while (!msg)
	{
		if (sftp_read_and_dispatch(sftp))
			return NULL;
		msg = sftp_dequeue(sftp, id);
	}
	if (msg->packet_type == SSH_FXP_NAME)   /* good response */
	{
		buffer_get_u32(msg->payload, &ignored);	/* we don't care about "count" */
		name = buffer_get_ssh_string(msg->payload); /* we only care about the file name string */
		cname = string_to_char(name);
		free(name);
		return cname;
	}
	else if (msg->packet_type == SSH_FXP_STATUS)	/* bad response (error) */
	{
		status = parse_status_msg(msg);
		sftp_message_free(msg);
		if (!status)
            return NULL;
        ssh_set_error(sftp->session,SSH_REQUEST_DENIED, "sftp server: %s", status->errormsg);
        status_msg_free(status);
	}
	else	/* this shouldn't happen */
	{
		ssh_set_error(sftp->session,SSH_INVALID_DATA, "Received message %d when attempting to set stats", msg->packet_type);
        sftp_message_free(msg);
	}
	return NULL;
}

SFTP_ATTRIBUTES *sftp_xstat(SFTP_SESSION *sftp, char *path,int param){
    u32 id=sftp_get_new_id(sftp);
    BUFFER *buffer=buffer_new();
    STRING *pathstr= string_from_char(path);
    SFTP_MESSAGE *msg=NULL;
    STATUS_MESSAGE *status=NULL;
    SFTP_ATTRIBUTES *pattr=NULL;

    buffer_add_u32(buffer,id);
    buffer_add_ssh_string(buffer,pathstr);
    free(pathstr);
    sftp_packet_write(sftp,param,buffer);
    buffer_free(buffer);
    while(!msg){
        if(sftp_read_and_dispatch(sftp))
            return NULL;
        msg=sftp_dequeue(sftp,id);
    }
    if(msg->packet_type==SSH_FXP_ATTRS){
        pattr=sftp_parse_attr(sftp,msg->payload,0);
        return pattr;
    }
    if(msg->packet_type== SSH_FXP_STATUS){
        status=parse_status_msg(msg);
        sftp_message_free(msg);
        if(!status)
            return NULL;
        ssh_set_error(sftp->session,SSH_REQUEST_DENIED,"sftp server: %s",status->errormsg);
        status_msg_free(status);
        return NULL;
    }
    ssh_set_error(sftp->session,SSH_INVALID_DATA,"Received mesg %d during stat(),mesg->packet_type");
    sftp_message_free(msg);
    return NULL;
}

SFTP_ATTRIBUTES *sftp_stat(SFTP_SESSION *session, char *path){
    return sftp_xstat(session,path,SSH_FXP_STAT);
}
SFTP_ATTRIBUTES *sftp_lstat(SFTP_SESSION *session, char *path){
    return sftp_xstat(session,path,SSH_FXP_LSTAT);
}

SFTP_ATTRIBUTES *sftp_fstat(SFTP_FILE *file) {
    u32 id=sftp_get_new_id(file->sftp);
    BUFFER *buffer=buffer_new();
    SFTP_MESSAGE *msg=NULL;
    STATUS_MESSAGE *status=NULL;
    SFTP_ATTRIBUTES *pattr=NULL;

    buffer_add_u32(buffer,id);
    buffer_add_ssh_string(buffer,file->handle);
    sftp_packet_write(file->sftp,SSH_FXP_FSTAT,buffer);
    buffer_free(buffer);
    while(!msg){
        if(sftp_read_and_dispatch(file->sftp))
            return NULL;
        msg=sftp_dequeue(file->sftp,id);
    }
    if(msg->packet_type==SSH_FXP_ATTRS){
        pattr=sftp_parse_attr(file->sftp,msg->payload,0);
        return pattr;
    }
    if(msg->packet_type== SSH_FXP_STATUS){
        status=parse_status_msg(msg);
        sftp_message_free(msg);
        if(!status)
            return NULL;
        ssh_set_error(file->sftp->session,SSH_REQUEST_DENIED,"sftp server: %s",status->errormsg);
        status_msg_free(status);
        return NULL;
    }
    ssh_set_error(file->sftp->session,SSH_INVALID_DATA,"Received mesg %d during fstat(),mesg->packet_type");
    sftp_message_free(msg);
    return NULL;
}


#endif /* NO_SFTP */
