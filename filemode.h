/* Make a string describing file modes.

   Copyright (C) 1998, 1999, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef FILEMODE_H_

#include "libssh/libssh.h"

#define S_IFMT		0170000 /* type of file */
#define S_IFSOCK	0140000 /* socket						's' */
#define S_IFLNK		0120000 /* symbolic link				'l' */
#define S_IFREG		0100000 /* regular						'-' */
#define S_IFBLK		0060000 /* block special				'b' */
#define S_IFDIR		0040000 /* directory					'd' */
#define S_IFCHR		0020000 /* character special			'c' */
#define S_IFIFO		0010000 /* fifo							'p' */
#define S_IFDOOR	0150000	/* Solaris door 				'D' */		
#define S_IFNAM		0050000	/* XENIX named file				'x' */
#define S_IFMPB		0070000	/* multiplexed block special 	'B'	*/
#define S_IFMPC		0030000	/* multiplexed char special		'm' */
#define S_IFWHT		0160000 /* BSD whiteout					'w' */
#define S_IFNWK		110000	/* HP-UX network special		'n' */ 

/* TODO: Add support for other obscure types
 *   S_IFCNT				Contiguous file					'C'
 *   S_IFSHAD 130000		Solaris shadow inode for ACL (not seen by userspace)
 *   S_IFEVC				UNOS eventcount
 *   S_ISOFD				Cray DMF: off line with data	'M'
 *   S_ISOFL				Cray DMF: off line with no data 'M'
 */

#define S_ISUID		0004000 /* set user id on execution */
#define S_ISGID		0002000 /* set group id on execution */
#define S_ISVTX		0001000 /* save swapped text even after use */

#define S_IRWXU		(S_IRUSR | S_IWUSR | S_IXUSR)
#define S_IRUSR		0000400 /* read permission, owner */
#define S_IWUSR		0000200 /* write permission, owner */
#define S_IXUSR		0000100 /* execute/search permission, owner */
#define S_IRWXG		(S_IRGRP | S_IWGRP | S_IXGRP)
#define S_IRGRP		0000040 /* read permission, group */
#define S_IWGRP		0000020 /* write permission, grougroup */
#define S_IXGRP		0000010 /* execute/search permission, group */
#define S_IRWXO		(S_IROTH | S_IWOTH | S_IXOTH)
#define S_IROTH		0000004 /* read permission, other */
#define S_IWOTH		0000002 /* write permission, other */
#define S_IXOTH		0000001 /* execute/search permission, other */

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)
#define S_ISDOOR(m) (((m) & S_IFMT) == S_IFDOOR) /* Solaris 2.5 and up */
#define S_ISNAM(m)  (((m) & S_IFMT) == S_IFNAM)  /* Xenix */
#define S_ISMPB(m)  (((m) & S_IFMT) == S_IFMPB)  /* V7 */
#define S_ISMPC(m)  (((m) & S_IFMT) == S_IFMPC)  /* V7 */
#define S_ISWHT(m)  (((m) & S_IFMT) == S_IFWHT)  /* BSD whiteout */
#define S_ISNWK(m)  (((m) & S_IFMT) == S_IFNWK)  /* HP/UX */
/* contiguous */
# ifndef S_ISCTG
#  define S_ISCTG(p) 0
# endif
/* Cray DMF (data migration facility): offline, with data  */
# ifndef S_ISOFD
#  define S_ISOFD(p) 0
# endif
/* Cray DMF (data migration facility): offline, with no data  */
# ifndef S_ISOFL
#  define S_ISOFL(p) 0
# endif

typedef u32 mode_t;

void mode_string (mode_t mode, char *str);

#endif
