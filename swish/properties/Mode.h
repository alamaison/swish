/*  File permissions processing functions (header).

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

#pragma once
#ifndef MODE_H_
#define MODE_H_

#include "filemode.h"
#include <string>

namespace swish {
	namespace properties {
		namespace mode {

			class Mode
			{
			public:
				Mode(mode_t mode);

				bool isRegular();          ///< regular                    '-'
				bool isSymLink();          ///< Symbolic link              'l'
				bool isDirectory();        ///< directory                  'd'
				bool isCharacter();        ///< character special          'c'
				bool isBlock();            ///< block special              'b'
				bool isFifo();             ///< fifo                       'p'
				bool isSocket();           ///< socket                     's'
				bool isDoor();             ///< Solaris door               'D'
				bool isNamed();            ///< XENIX named file           'x'
				bool isMultiplexedBlock(); ///< multiplexed block special  'B'
				bool isMultiplexedChar();  ///< multiplexed char special   'm'
				bool isWhiteout();         ///< BSD whiteout               'w'
				bool isNetwork();          ///< HP-UX network special      'n'
				bool isContiguous();       ///< contiguous                 'C'
				bool isOffline();          ///< Cray DMF offline no data   'M'
				bool isOfflineData();      ///< Cray DMF offline + data    'M'

				bool isSUID();
				bool isSGID();
				bool isSticky();

				std::string toString();

			private:
				mode_t mode;
			};
		}
	}
}

#endif /*MODE_H_*/
