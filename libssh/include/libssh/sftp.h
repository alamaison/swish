/* sftp headers */
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

#ifndef SFTP_H
#define SFTP_H
#include <libssh/libssh.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct sftp_session_struct {
    SSH_SESSION *session;
    CHANNEL *channel;
    int server_version;
    struct request_queue *queue;
    u32 id_counter;
} SFTP_SESSION ;

typedef struct {
    SFTP_SESSION *sftp;
    u8 type;
    BUFFER *payload;
} SFTP_PACKET;

/* file handler */
typedef struct sftp_file{
    SFTP_SESSION *sftp;
    char *name;
    u64 offset;
    STRING *handle;
    int eof;
    int nonblocking;
}  SFTP_FILE ;

typedef struct sftp_dir {
    SFTP_SESSION *sftp;
    char *name;
    STRING *handle; /* handle to directory */
    BUFFER *buffer; /* contains raw attributes from server which haven't been parsed */
    u32 count; /* counts the number of following attributes structures into buffer */
    int eof; /* end of directory listing */
} SFTP_DIR;

typedef struct {
    SFTP_SESSION *sftp;
    u8 packet_type;
    BUFFER *payload;
    u32 id;
} SFTP_MESSAGE;

typedef struct request_queue{
    struct request_queue *next;
    SFTP_MESSAGE *message;
} REQUEST_QUEUE;

/* SSH_FXP_MESSAGE described into .7 page 26 */
typedef struct {
    u32 id;
    u32 status;
    STRING *error;
    STRING *lang;
    char *errormsg;
    char *langmsg;
} STATUS_MESSAGE;

/* don't worry much of these aren't really used */
typedef struct {
    char *name;
    char *longname; /* some weird stuff */
    u32 flags;
    u8 type;
    u64 size;
    u32 uid;
    u32 gid;
    char *owner;
    char *group;
    u32 permissions;
    u64 atime64;
    u32 atime;
    u32 atime_nseconds;
    u64 createtime;
    u32 createtime_nseconds;
    u64 mtime64;
    u32 mtime;
    u32 mtime_nseconds;
    STRING *acl;
    u32 extended_count;
    STRING *extended_type;
    STRING *extended_data;
} SFTP_ATTRIBUTES;

#define LIBSFTP_VERSION 3

SFTP_SESSION *sftp_new(SSH_SESSION *session);
void sftp_free(SFTP_SESSION *sftp);
int sftp_init(SFTP_SESSION *sftp);
SFTP_DIR *sftp_opendir(SFTP_SESSION *session, char *path);
/* reads one file and attribute from opened directory. fails at end */
SFTP_ATTRIBUTES *sftp_readdir(SFTP_SESSION *session, SFTP_DIR *dir);
/* returns 1 if the directory was EOF */
int sftp_dir_eof(SFTP_DIR *dir);
SFTP_ATTRIBUTES *sftp_stat(SFTP_SESSION *session, char *path);
SFTP_ATTRIBUTES *sftp_lstat(SFTP_SESSION *session, char *path);
/* sftp_lstat stats a file but doesn't follow symlinks */
SFTP_ATTRIBUTES *sftp_fstat(SFTP_FILE *file);
void sftp_attributes_free(SFTP_ATTRIBUTES *file);
int sftp_dir_close(SFTP_DIR *dir);
int sftp_file_close(SFTP_FILE *file);
/* access are the sames than the ones from ansi fopen() */
SFTP_FILE *sftp_open(SFTP_SESSION *session, char *file, int access, SFTP_ATTRIBUTES *attr);
int sftp_read(SFTP_FILE *file, void *dest, int len);
int sftp_write(SFTP_FILE *file, void *source, int len);
void sftp_seek(SFTP_FILE *file, int new_offset);
unsigned long sftp_tell(SFTP_FILE *file);
void sftp_rewind(SFTP_FILE *file);
int sftp_rm(SFTP_SESSION *sftp, char *file);
int sftp_rmdir(SFTP_SESSION *sftp, char *directory);
int sftp_mkdir(SFTP_SESSION *sftp, char *directory, SFTP_ATTRIBUTES *attr);
int sftp_rename(SFTP_SESSION *sftp, char *original, char *newname);
int sftp_setstat(SFTP_SESSION *sftp, char *file, SFTP_ATTRIBUTES *attr);
char *sftp_canonicalize_path(SFTP_SESSION *sftp, char *path);

/* SFTP commands and constants */
#define SSH_FXP_INIT 1
#define SSH_FXP_VERSION 2
#define SSH_FXP_OPEN 3
#define SSH_FXP_CLOSE 4
#define SSH_FXP_READ 5
#define SSH_FXP_WRITE 6
#define SSH_FXP_LSTAT 7
#define SSH_FXP_FSTAT 8
#define SSH_FXP_SETSTAT 9
#define SSH_FXP_FSETSTAT 10
#define SSH_FXP_OPENDIR 11
#define SSH_FXP_READDIR 12
#define SSH_FXP_REMOVE 13
#define SSH_FXP_MKDIR 14
#define SSH_FXP_RMDIR 15
#define SSH_FXP_REALPATH 16
#define SSH_FXP_STAT 17
#define SSH_FXP_RENAME 18
#define SSH_FXP_READLINK 19
#define SSH_FXP_SYMLINK 20

#define SSH_FXP_STATUS 101
#define SSH_FXP_HANDLE 102
#define SSH_FXP_DATA 103
#define SSH_FXP_NAME 104
#define SSH_FXP_ATTRS 105

#define SSH_FXP_EXTENDED 200
#define SSH_FXP_EXTENDED_REPLY 201

/* attributes */
/* sftp draft is completely braindead : version 3 and 4 have different flags for same constants */
/* and even worst, version 4 has same flag for 2 different constants */
/* follow up : i won't develop any sftp4 compliant library before having a clarification */

#define SSH_FILEXFER_ATTR_SIZE 0x00000001
#define SSH_FILEXFER_ATTR_PERMISSIONS 0x00000004
#define SSH_FILEXFER_ATTR_ACCESSTIME 0x00000008
#define SSH_FILEXFER_ATTR_ACMODTIME  0x00000008
#define SSH_FILEXFER_ATTR_CREATETIME 0x00000010
#define SSH_FILEXFER_ATTR_MODIFYTIME 0x00000020
#define SSH_FILEXFER_ATTR_ACL 0x00000040
#define SSH_FILEXFER_ATTR_OWNERGROUP 0x00000080
#define SSH_FILEXFER_ATTR_SUBSECOND_TIMES 0x00000100
#define SSH_FILEXFER_ATTR_EXTENDED 0x80000000
#define SSH_FILEXFER_ATTR_UIDGID 0x00000002

/* types */
#define SSH_FILEXFER_TYPE_REGULAR 1
#define SSH_FILEXFER_TYPE_DIRECTORY 2
#define SSH_FILEXFER_TYPE_SYMLINK 3
#define SSH_FILEXFER_TYPE_SPECIAL 4
#define SSH_FILEXFER_TYPE_UNKNOWN 5

/* server responses */
#define SSH_FX_OK 0
#define SSH_FX_EOF 1
#define SSH_FX_NO_SUCH_FILE 2
#define SSH_FX_PERMISSION_DENIED 3
#define SSH_FX_FAILURE 4
#define SSH_FX_BAD_MESSAGE 5
#define SSH_FX_NO_CONNECTION 6
#define SSH_FX_CONNECTION_LOST 7
#define SSH_FX_OP_UNSUPPORTED 8
#define SSH_FX_INVALID_HANDLE 9
#define SSH_FX_NO_SUCH_PATH 10
#define SSH_FX_FILE_ALREADY_EXISTS 11
#define SSH_FX_WRITE_PROTECT 12
#define SSH_FX_NO_MEDIA 13

/* file flags */
#define SSH_FXF_READ 0x01
#define SSH_FXF_WRITE 0x02
#define SSH_FXF_APPEND 0x04
#define SSH_FXF_CREAT 0x08
#define SSH_FXF_TRUNC 0x10
#define SSH_FXF_EXCL 0x20
#define SSH_FXF_TEXT 0x40

#ifdef __cplusplus
} ;
#endif

#endif /* SFTP_H */
