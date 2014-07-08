CC=gcc
CPP=g++
CFLAGS=-Wall
CLIBX=-lX11
CPTHRD=-pthread
CLIBRT=-lrt
CLIBPP=-lstdc++
GPIOSRCS=para_morse.c para_gpio.c
GPIODEPS=Makefile $(GPIOSRCS) para_morse.h para_gpio.h

DESTDIR = 
PREFIX  = "/usr/local"
DATADIR = "${PREFIX}/share/parallella"
MANDIR  = "${PREFIX}/share/man"
BINDIR  = "${PREFIX}/bin"

all: xtemp pmorse

everything: xtemp pmorse gpiotest porcutest spitest facetest

xtemp: xtemp.c
	$(CC) -o xtemp xtemp.c $(CFLAGS) $(CLIBX) $(CPTHRD)

pmorse: pmorse.c $(GPIODEPS)
	$(CC) -o pmorse pmorse.c $(GPIOSRCS) $(CFLAGS)

gpiotest: gpiotest.c $(GPIODEPS)
	$(CC) -o gpiotest gpiotest.c $(GPIOSRCS) $(CFLAGS) $(CLIBRT)

porcutest: porcutest.cpp para_gpio.c para_gpio.cpp
	$(CC) -o porcutest porcutest.cpp para_gpio.c para_gpio.cpp $(CLIBPP) $(CFLAGS)

spitest: spitest.cpp para_spi.cpp para_spi.h para_gpio.c para_gpio.cpp para_gpio.h
	$(CC) -o spitest spitest.cpp para_spi.cpp para_gpio.cpp para_gpio.c $(CLIBPP) $(CFLAGS)

facetest: facetest.cpp para_face.cpp para_face.h para_spi.cpp para_spi.h para_gpio.c para_gpio.cpp para_gpio.h
	$(CC) -o facetest facetest.cpp para_face.cpp para_spi.cpp para_gpio.cpp para_gpio.c $(CLIBPP) $(CFLAGS)

clean:
	rm -f xtemp pmorse gpiotest porcutest spitest facetest

install: install-exec

install-exec: xtemp pmorse
	install -D xtemp "${DESTDIR}${BINDIR}/xtemp"
	install -D pmorse "${DESTDIR}${BINDIR}/pmorse"

uninstall: uninstall-exec

uninstall-exec:
	rm "${DESTDIR}${BINDIR}/xtemp"
	rm "${DESTDIR}${BINDIR}/pmorse"

.PHONY: clean install install-exec uninstall uninstall-exec


