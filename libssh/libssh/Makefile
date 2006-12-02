
OBJECTS= client.o packet.o dh.o crypt.o connect.o error.o buffer.o \
	string.o kex.o channels.o options.o keys.o auth.o base64.o \
        keyfiles.o misc.o gzip.o wrapper.o sftp.o server.o malloc.o
SHELL = /bin/sh


subdirs = @subdirs@
top_srcdir = ..
srcdir = .
prefix = /usr/local
exec_prefix = ${prefix}
bindir = $(exec_prefix)/bin
incldir= $(prefix)/include
infodir = $(prefix)/info
libdir = $(prefix)/lib/
mandir = $(prefix)/man/man1

CC = gcc
CFLAGS = -mno-cygwin -Wall -g -I../include/ -fPIC
LDFLAGS = -mno-cygwin
LIBS = -lz -lcrypto 
INSTALL = /usr/bin/install -c
DYLIB_EXTENSION = so
LIBSSH_LDFLAGS = -shared

all: libssh.so

libssh.so: $(OBJECTS)
	$(CC) -o libssh.$(DYLIB_EXTENSION) $(LIBSSH_LDFLAGS) $(OBJECTS) $(LIBS) $(LDFLAGS)
libssh.a: $(OBJECTS)
	rm -f libssh.a
	ar q libssh.a $(OBJECTS)
	ranlib libssh.a
install: all
	$(top_srcdir)/mkinstalldirs $(incldir)
	$(top_srcdir)/mkinstalldirs $(libdir)
	$(INSTALL) libssh.$(DYLIB_EXTENSION) $(libdir)
clean:
	rm -f *~ libssh.a libssh.so *.o
distclean: clean
	rm -f Makefile

