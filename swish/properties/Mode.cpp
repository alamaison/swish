/*  File permissions processing functions

    Copyright (C) 2006, 2009  Alexander Lamaison <awl03 (at) doc.ic.ac.uk>

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

#include "Mode.h"

#include <assert.h>

using namespace swish::properties::mode;

Mode::Mode(mode_t mode) : mode(mode) {}

bool Mode::isSymLink()
{
	assert(S_ISLNK(mode) ^ ( S_ISREG(mode) || S_ISDIR(mode) || S_ISCHR(mode)
		|| S_ISBLK(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)
		|| S_ISDOOR(mode) || S_ISNAM(mode) || S_ISMPB(mode)
		|| S_ISMPC(mode) || S_ISWHT(mode) || S_ISNWK(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISLNK(mode);
}

bool Mode::isRegular()
{
	assert(S_ISREG(mode) ^ ( S_ISLNK(mode) || S_ISDIR(mode) || S_ISCHR(mode)
		|| S_ISBLK(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)
		|| S_ISDOOR(mode) || S_ISNAM(mode) || S_ISMPB(mode)
		|| S_ISMPC(mode) || S_ISWHT(mode) || S_ISNWK(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISREG(mode);
}

bool Mode::isDirectory()
{
	assert(S_ISDIR(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISCHR(mode)
		|| S_ISBLK(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)
		|| S_ISDOOR(mode) || S_ISNAM(mode) || S_ISMPB(mode)
		|| S_ISMPC(mode) || S_ISWHT(mode) || S_ISNWK(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISDIR(mode);
}

bool Mode::isCharacter()
{
	assert(S_ISLNK(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISBLK(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)
		|| S_ISDOOR(mode) || S_ISNAM(mode) || S_ISMPB(mode)
		|| S_ISMPC(mode) || S_ISWHT(mode) || S_ISNWK(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISLNK(mode);
}

bool Mode::isBlock()
{
	assert(S_ISBLK(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISCHR(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)
		|| S_ISDOOR(mode) || S_ISNAM(mode) || S_ISMPB(mode)
		|| S_ISMPC(mode) || S_ISWHT(mode) || S_ISNWK(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISBLK(mode);
}

bool Mode::isFifo()
{
	assert(S_ISFIFO(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISCHR(mode) || S_ISBLK(mode) || S_ISSOCK(mode)
		|| S_ISDOOR(mode) || S_ISNAM(mode) || S_ISMPB(mode)
		|| S_ISMPC(mode) || S_ISWHT(mode) || S_ISNWK(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISFIFO(mode);
}

bool Mode::isSocket()
{
	assert(S_ISSOCK(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode)
		|| S_ISDOOR(mode) || S_ISNAM(mode) || S_ISMPB(mode)
		|| S_ISMPC(mode) || S_ISWHT(mode) || S_ISNWK(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISSOCK(mode);
}

bool Mode::isDoor()                                     /* Solaris door 'D' */
{
	assert(S_ISDOOR(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode)
		|| S_ISSOCK(mode) || S_ISNAM(mode) || S_ISMPB(mode)
		|| S_ISMPC(mode) || S_ISWHT(mode) || S_ISNWK(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISDOOR(mode);
}

bool Mode::isNamed()                                /* XENIX named file 'x' */
{
	assert(S_ISNAM(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode)
		|| S_ISSOCK(mode) || S_ISDOOR(mode) || S_ISMPB(mode)
		|| S_ISMPC(mode) || S_ISWHT(mode) || S_ISNWK(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISNAM(mode);
}

bool Mode::isMultiplexedBlock()            /* multiplexed block special 'B' */
{
	assert(S_ISMPB(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode)
		|| S_ISSOCK(mode) || S_ISDOOR(mode) || S_ISNAM(mode)
		|| S_ISMPC(mode) || S_ISWHT(mode) || S_ISNWK(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISMPB(mode);
}

bool Mode::isMultiplexedChar()             /* multiplexed char special 'm' */
{
	assert(S_ISMPC(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode)
		|| S_ISSOCK(mode) || S_ISDOOR(mode) || S_ISNAM(mode)
		|| S_ISMPB(mode) || S_ISWHT(mode) || S_ISNWK(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISMPC(mode);
}

bool Mode::isWhiteout()                                 /* BSD whiteout 'w' */
{
	assert(S_ISWHT(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode)
		|| S_ISSOCK(mode) || S_ISDOOR(mode) || S_ISNAM(mode)
		|| S_ISMPB(mode) || S_ISMPC(mode) || S_ISNWK(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISWHT(mode);
}

bool Mode::isNetwork()                         /* HP-UX network special 'n' */
{
	assert(S_ISNWK(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode)
		|| S_ISSOCK(mode) || S_ISDOOR(mode) || S_ISNAM(mode)
		|| S_ISMPB(mode) || S_ISMPC(mode) || S_ISWHT(mode)
		|| S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISNWK(mode);
}

bool Mode::isContiguous()                 /* contiguous - returns false 'C' */
{
	assert(S_ISCTG(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode)
		|| S_ISSOCK(mode) || S_ISDOOR(mode) || S_ISNAM(mode)
		|| S_ISMPB(mode) || S_ISMPC(mode) || S_ISWHT(mode)
		|| S_ISNWK(mode) || S_ISCTG(mode) || S_ISOFD(mode) || S_ISOFL(mode) ));
	return S_ISCTG(mode);
}

bool Mode::isOffline()      /* Cray DMF offline no data - returns false 'M' */
{
	assert(S_ISOFL(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode)
		|| S_ISSOCK(mode) || S_ISDOOR(mode) || S_ISNAM(mode)
		|| S_ISMPB(mode) || S_ISMPC(mode) || S_ISWHT(mode)
		|| S_ISNWK(mode) || S_ISCTG(mode) || S_ISOFD(mode) ));
	return S_ISOFL(mode);
}

bool Mode::isOfflineData()   /* Cray DMF offline + data - returns false 'M' */
{
	assert(S_ISOFD(mode) ^ ( S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode)
		|| S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode)
		|| S_ISSOCK(mode) || S_ISDOOR(mode) || S_ISNAM(mode)
		|| S_ISMPB(mode) || S_ISMPC(mode) || S_ISWHT(mode)
		|| S_ISNWK(mode) || S_ISCTG(mode) || S_ISOFL(mode) ));
	return S_ISOFD(mode);
}


bool Mode::isSUID()
{
	return ((mode & S_ISUID) == S_ISUID);
}

bool Mode::isSGID()
{
	return ((mode & S_ISGID) == S_ISGID);
}

bool Mode::isSticky()
{
	return ((mode & S_ISVTX) == S_ISVTX);
}

std::string Mode::toString()
{
	const int MODE_STR_BUFFER_SIZE = 10;
	char buf[MODE_STR_BUFFER_SIZE];
	::mode_string(mode, buf);
	return std::string(buf, MODE_STR_BUFFER_SIZE);
}