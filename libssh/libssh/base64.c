/* base64 contains the needed support for base64 alphabet system, */
/* as described in RFC1521 */
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

/* just the dirtiest part of code i ever made */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "libssh/priv.h"
static char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "abcdefghijklmnopqrstuvwxyz"
                        "0123456789+/" ;

/* transformations */
#define SET_A(n,i) do { n |= (i&63) <<18; } while (0)
#define SET_B(n,i) do { n |= (i&63) <<12; } while (0)
#define SET_C(n,i) do { n |= (i&63) << 6; } while (0)
#define SET_D(n,i) do { n |= (i&63); } while (0)

#define GET_A(n) ((n & 0xff0000) >> 16)
#define GET_B(n) ((n & 0xff00) >> 8)
#define GET_C(n) (n & 0xff)

static int _base64_to_bin(unsigned char dest[3], char *source,int num);
static int get_equals(char *string);

/* first part : base 64 to binary */

/* base64_to_bin translates a base64 string into a binary one. important, if something went wrong (ie incorrect char)*/
/* it returns NULL */
BUFFER *base64_to_bin(char *source){
    int len;
    int equals;
    BUFFER *buffer=buffer_new();
    unsigned char block[3];

    /* get the number of equals signs, which mirrors the padding */
    equals=get_equals(source);
    if(equals>2){
        buffer_free(buffer);
        return NULL;
    }

    len=strlen(source);
    while(len>4){
        if(_base64_to_bin(block,source,3)){
            buffer_free(buffer);
            return NULL;
        }
        buffer_add_data(buffer,block,3);
        len-=4;
        source+=4;
    }
    /* depending of the number of bytes resting, there are 3 possibilities (from the rfc) */
    switch(len){
/* (1) the final quantum of encoding input is an integral
   multiple of 24 bits; here, the final unit of encoded output will be
   an integral multiple of 4 characters with no "=" padding */
        case 4:
            if(equals!=0){
                buffer_free(buffer);
                return NULL;
            }
            if(_base64_to_bin(block,source,3)){
                buffer_free(buffer);
                return NULL;
            }
            buffer_add_data(buffer,block,3);
            return buffer;
/*(2) the final quantum of encoding input is exactly 8 bits; here, the final
   unit of encoded output will be two characters followed by two "="
   padding characters */
        case 2:
            if(equals!=2){
                buffer_free(buffer);
                return NULL;
            }
            if(_base64_to_bin(block,source,1)){
                buffer_free(buffer);
                return NULL;
            }
            buffer_add_data(buffer,block,1);
            return buffer;
/* the final quantum of encoding input is
   exactly 16 bits; here, the final unit of encoded output will be three
   characters followed by one "=" padding character */
        case 3:
            if(equals!=1){
                buffer_free(buffer);
                return NULL;
            }
            if(_base64_to_bin(block,source,2)){
                buffer_free(buffer);
                return NULL;
            }
            buffer_add_data(buffer,block,2);
            return buffer;
        default:
            /* 4,3,2 are the only padding size allowed */
            buffer_free(buffer);
            return NULL;
   }
   return NULL;
}
    
#define BLOCK(letter,n) do { ptr=strchr(alphabet,source[n]);\
                                        if(!ptr) return -1;\
                                        i=ptr-alphabet;\
                                        SET_##letter(*block,i);\
                                        } while(0)
/* returns 0 if ok, -1 if not (ie invalid char into the stuff) */
static int to_block4(unsigned long *block, char *source,int num){
    char *ptr;
    unsigned int i;
    *block=0;
    if(num<1)
        return 0;
    BLOCK(A,0); /* 6 bits */
    BLOCK(B,1); /* 12 */
    if(num<2)
        return 0;
    BLOCK(C,2); /* 18 */
    if(num < 3)
        return 0;
    BLOCK(D,3); /* 24 */
    return 0;
}

/* num = numbers of final bytes to be decoded */ 
static int _base64_to_bin(unsigned char dest[3], char *source,int num){
   unsigned long block;
   if(to_block4(&block,source,num))
       return -1;
   dest[0]=GET_A(block);
   dest[1]=GET_B(block);
   dest[2]=GET_C(block);
   return 0;
}

/* counts the number of "=" signs, and replace them by zeroes */
static int get_equals(char *string){
    char *ptr=string;
    int num=0;
    while((ptr=strchr(ptr,'='))){
        num++;
        *ptr=0;
        ptr++;
    }
    
    return num;
}

/* thanks sysk for debugging my mess :) */
#define BITS(n) ((1<<n)-1)
static void _bin_to_base64(unsigned char *dest, unsigned char source[3], int len){
    switch (len){
        case 1:
            dest[0]=alphabet[(source[0]>>2)];
            dest[1]=alphabet[((source[0] & BITS(2)) << 4)];
            dest[2]='=';
            dest[3]='=';
            break;
        case 2:
            dest[0]=alphabet[source[0]>>2];
            dest[1]=alphabet[(source[1]>>4) | ((source[0] & BITS(2)) << 4)];
            dest[2]=alphabet[(source[1]&BITS(4)) << 2]; 
            dest[3]='=';
            break;
        case 3:
            dest[0]=alphabet[(source[0]>>2)]; 
            dest[1]=alphabet[(source[1]>>4) | ((source[0] & BITS(2)) << 4)];
            dest[2]=alphabet[ (source[2] >> 6) | (source[1]&BITS(4)) << 2]; 
            dest[3]=alphabet[source[2]&BITS(6)];
        break;
    }
}

char *bin_to_base64(unsigned char *source, int len){
    int flen=len + (3 - (len %3)); /* round to upper 3 multiple */
    char *buffer;
    char *ptr;
    flen=(4 * flen)/3 + 1 ;
    ptr=buffer=malloc(flen);
    while(len>0){
        _bin_to_base64(ptr,source,len>3?3:len);
        ptr+=4;
        source +=3;
        len -=3;
    }
    ptr[0]=0;
    return buffer;
}
