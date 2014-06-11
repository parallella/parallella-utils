CC=gcc
CFLAGS	= -pthread -lX11 -Wall

DESTDIR = 
PREFIX  = "/usr/local"
DATADIR = "${PREFIX}/share/parallella"
MANDIR  = "${PREFIX}/share/man"
BINDIR  = "${PREFIX}/bin"

all: xtemp

xtemp: xtemp.c
	$(CC) -o xtemp xtemp.c $(CFLAGS)

clean:
	rm xtemp

install: install-exec

install-exec: xtemp
	install -D xtemp "${DESTDIR}${BINDIR}/xtemp"

uninstall: uninstall-exec

uninstall-exec:
	rm "${DESTDIR}${BINDIR}/xtemp"

.PHONY: clean install install-exec uninstall uninstall-exec
