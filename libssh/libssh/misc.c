/* misc.c */
/* some misc routines than aren't really part of the ssh protocols but can be useful to the client */

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
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <sys/types.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include "libssh/libssh.h"

/* if the program was executed suid root, don't trust the user ! */
static int is_trusted(){
#ifdef HAVE_GETUID
#  ifdef HAVE_GETEUID
    if(geteuid()!=getuid())
        return 0;
#  endif
#endif
    return 1;
}

static char *get_homedir_from_uid(int uid){
    char *home;
#ifdef HAVE_GETPWENT
    struct passwd *pwd;
    while((pwd=getpwent())){
        if(pwd->pw_uid == uid){
            home=strdup(pwd->pw_dir);
            endpwent();
            return home;
        }
    }
    endpwent();
#else
#ifdef __WIN32__
    home = strdup("c:/");
    return(home);
#endif
#endif
    return NULL;
}

static char *get_homedir_from_login(char *user){
#ifdef HAVE_GETPWENT
    struct passwd *pwd;
    char *home;
    while((pwd=getpwent())){
        if(!strcmp(pwd->pw_name,user)){
            home=strdup(pwd->pw_dir);
            endpwent();
            return home;
        }
    }
    endpwent();
#endif
    return NULL;
}
        
char *ssh_get_user_home_dir(){
    char *home;
    char *user;
    int trusted=is_trusted();
    if(trusted){
        if((home=getenv("HOME")))
            return strdup(home);
        if((user=getenv("USER")))
            return get_homedir_from_login(user);
    }
#ifdef HAVE_GETUID
    return get_homedir_from_uid(getuid());
#else
    return get_homedir_from_uid(0);
#endif
}

/* we have read access on file */
int ssh_file_readaccess_ok(char *file){
    if(!access(file,R_OK))
        return 1;
    return 0;
}


u64 ntohll(u64 a){
#ifdef WORDS_BIGENDIAN
    return a;
#else
    u32 low=a & 0xffffffff;
    u32 high = a >> 32 ;
    low=ntohl(low);
    high=ntohl(high);
    return (( ((u64)low) << 32) | ( high));
#endif
}
