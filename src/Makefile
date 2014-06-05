CC=gcc
CFLAGS=-Wall
CLIBX=-lX11
CPTHRD=-pthread
CLIBRT=-lrt
GPIOSRCS=para_morse.c para_gpio.c
GPIODEPS=Makefile $(GPIOSRCS) para_morse.h para_gpio.h

DESTDIR = 
PREFIX  = "/usr/local"
DATADIR = "${PREFIX}/share/parallella"
MANDIR  = "${PREFIX}/share/man"
BINDIR  = "${PREFIX}/bin"

all: xtemp pmorse

xtemp: xtemp.c
	$(CC) -o xtemp xtemp.c $(CFLAGS) $(CLIBX) $(CPTHRD)

pmorse: pmorse.c $(GPIODEPS)
	$(CC) -o pmorse pmorse.c $(GPIOSRCS) $(CFLAGS)

gpiotest: gpiotest.c $(GPIODEPS)
	$(CC) -o gpiotest gpiotest.c $(GPIOSRCS) $(CFLAGS) $(CLIBRT)

clean:
	rm -f xtemp pmorse gpiotest

install: install-exec

install-exec: xtemp pmorse
	install -D xtemp "${DESTDIR}${BINDIR}/xtemp"
	install -D pmorse "${DESTDIR}${BINDIR}/pmorse"

uninstall: uninstall-exec

uninstall-exec:
	rm "${DESTDIR}${BINDIR}/xtemp"
	rm "${DESTDIR}${BINDIR}/pmorse"

.PHONY: clean install install-exec uninstall uninstall-exec


