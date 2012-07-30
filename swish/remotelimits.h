/*  Defines constant limits given that resources are not local and
    therefore do not follow OS-defined constants

    Copyright (C) 2007  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef REMOTELIMITS_H
#define REMOTELIMITS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// String limits should not include the terminating NULL
// TODO: Not sure if this is the case currently

#define MAX_USERNAME_LEN          32
#define MAX_USERNAME_LENZ         33

// Under libc-5 the utmp and wtmp files only allow 8 chars for username
#define MAX_USERNAME_LEN_UNIX      8
// Under libc-6 this is increased to 32 characters
#define MAX_USERNAME_LEN_LINUX    32
// Apparently Win2k can have names up to 104 characters: 
// http://www.microsoft.com/technet/prodtechnol/windows2000serv/deploy/confeat/08w2kada.mspx
#define MAX_USERNAME_LEN_WIN      20

#define MAX_HOSTNAME_LEN         255 // http://en.wikipedia.org/wiki/Hostname
#define MAX_HOSTNAME_LENZ        256

#define MAX_PATH_LEN            1023
#define MAX_PATH_LENZ           1024
#define MAX_PATH_LEN_WIN         248 // 260 including filename
#define MAX_PATH_LEN_LINUX      4096

#define MAX_FILENAME_LEN         255 // Choosing lower val as Windows FAT is
#define MAX_FILENAME_LENZ        256 // also limited to 255. Makes things easier
#define MAX_FILENAME_LEN_WIN     256 
#define MAX_FILENAME_LEN_LINUX   255

#define MIN_PORT                   0
#define MAX_PORT               65535
#define MAX_PORT_STR_LEN           5 // length of '65535' as a string

#define PROTOCOL_LEN               7 // length of 'sftp://' as a string

// Complete connection description of the form:
//     sftp://username@hostname:port/path
#define MAX_CANONICAL_LEN \
    (PROTOCOL_LEN + MAX_USERNAME_LEN + MAX_HOSTNAME_LEN \
    + MAX_PATH_LEN + MAX_PORT_STR_LEN + 2)

#define MAX_LABEL_LEN             30 // Arbitrary - chosen to be easy to display
#define MAX_LABEL_LENZ            31 

#define SFTP_DEFAULT_PORT         22

#define MAX_LONGENTRY_LEN        127 // 128 should be long enough to fit any
#define MAX_LONGENTRY_LENZ       128 // long entry (certainly what we need)

#endif REMOTELIMITS_H
