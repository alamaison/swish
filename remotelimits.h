#ifndef REMOTELIMITS_H
#define REMOTELIMITS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAX_USERNAME_LEN          32

/* Under libc-5 the utmp and wtmp files only allow 8 chars for username */
#define MAX_USERNAME_LEN_UNIX      8
/* Under libc-6 this is increased to 32 characters */
#define MAX_USERNAME_LEN_LINUX    32
// Apparently Win2k can have names up to 104 characters: 
// http://www.microsoft.com/technet/prodtechnol/windows2000serv/deploy/confeat/08w2kada.mspx
#define MAX_USERNAME_LEN_WIN      20

#define MAX_HOSTNAME_LEN         256

#define MAX_PATH_LEN            1024
#define MAX_PATH_LEN_WIN         248 // 260 including filename
#define MAX_PATH_LEN_LINUX      4096

#endif REMOTELIMITS_H
