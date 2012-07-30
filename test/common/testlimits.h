/* Various limits to be used in testing.
 *
 * This is based on remotelimits.h from the main source code.  We purposely
 * keep this copy out-of-sync here in order to prevent changes to remotelimits.h
 * relaxing test constraints
 */

#ifndef TEST_LIMITS_H
#define TEST_LIMITS_H

#pragma once

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

#endif // TEST_LIMITS_H
