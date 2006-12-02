/*string.c */
/* string manipulations... */
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
#include <stdlib.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include <unistd.h>
#include <string.h>

STRING *string_new(u32 size){
    STRING *str=malloc(size + 4);
    str->size=htonl(size);
    return str;
}

void string_fill(STRING *str,void *data,int len){
    memcpy(str->string,data,len);
}

STRING *string_from_char(char *what){
	STRING *ptr;
	int len=strlen(what);
	ptr=malloc(4 + len);
	ptr->size=htonl(len);
	memcpy(ptr->string,what,len);
	return ptr;
}

int string_len(STRING *str){
	return ntohl(str->size);
}

char *string_to_char(STRING *str){
    int len=ntohl(str->size)+1;
    char *string=malloc(len);
    memcpy(string,str->string,len-1);
    string[len-1]=0;
    return string;
}

STRING *string_copy(STRING *str){
    STRING *ret=malloc(ntohl(str->size)+4);
    ret->size=str->size;
    memcpy(ret->string,str->string,ntohl(str->size));
    return ret;
}
