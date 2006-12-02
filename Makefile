#
#   Swish project Makefile
#
#   Copyright (C) 2006  Alexander Lamaison <awl03 (at) doc.ic.ac.uk>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

CC = g++
USE_CYGWIN = -mno-cygwin

PROG = swish
OBJS = swish.o Server.o Mode.o filemode.o

INCLUDES = -I. -Ilibssh/include/ $(INCLUDES_GUI)
LDLIBS += -L. -lssh $(LDLIBS_GUI)

ifndef (WITHOUT_GUI) ################################################# GUI ##
OBJS += gui.o

INCLUDES_GUI = $(shell echo `wx-config --cxxflags`)
	
LDLIBS_GUI = $(shell echo `wx-config --libs`)
LDFLAGS += -mwindows
endif ##########################################################################

LDFLAGS += $(USE_CYGWIN) -g
CFLAGS += -DDEBUG $(INCLUDES) $(USE_CYGWIN) -g
CXXFLAGS += -DDEBUG $(INCLUDES) $(USE_CYGWIN) -g


all : $(PROG)

$(PROG) : $(OBJS)

swish.o : swish.h gui.h
gui.o : gui.h

.PHONY : clean
clean :
	$(RM) $(OBJS)
	$(RM) $(PROG).exe
	$(RM) *.stackdump

# $Id$
