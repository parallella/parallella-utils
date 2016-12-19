CC=gcc
CPP=g++
CFLAGS=-Wall
CLIBX=-lX11
CPTHRD=-pthread
CLIBRT=-lrt
CLIBPP=-lstdc++
GPIOSRCS=gpio_dir/para_morse.c gpio_dir/para_gpio.c
GPIODEPS=Makefile $(GPIOSRCS) gpio_dir/para_morse.h gpio_dir/para_gpio.h

DESTDIR ?=
PREFIX  ?= /usr/local
DATADIR = $(PREFIX)/share/parallella
MANDIR  = $(PREFIX)/share/man
BINDIR  = $(PREFIX)/bin

all: xtemp/xtemp pmorse

everything: xtemp/xtemp pmorse gpiotest porcutest spitest facetest getfpga/getfpga

xtemp_SRCS=xtemp/xtemp.c
xtemp_DEPS=Makefile $(xtemp_SRCS)
xtemp/xtemp: $(xtemp_DEPS)
	$(CC) $(xtemp_SRCS) $(CFLAGS) $(CLIBX) $(CPTHRD) -o $@

pmorse_SRCS=gpio_dir/pmorse.c $(GPIOSRCS)
pmorse_DEPS=Makefile $(pmorse_SRCS) $(GPIODEPS)
pmorse: $(pmorse_DEPS)
	$(CC) $(pmorse_SRCS) $(CFLAGS) -o $@

gpiotest_SRCS=gpio_dir/gpiotest.c $(GPIOSRCS)
gpiotest_DEPS=Makefile $(gpiotest_SRCS)
gpiotest: $(gpiotest_DEPS)
	$(CC) $(gpiotest_SRCS) $(CFLAGS) $(CLIBRT) -o $@

porcutest_SRCS=gpio_dir/porcutest.cpp gpio_dir/para_gpio.cpp gpio_dir/para_gpio.c
porcutest_DEPS=Makefile $(porcutest_SRCS)
porcutest: $(porcutest_DEPS)
	$(CC) $(porcutest_SRCS) $(CLIBPP) $(CFLAGS) -o $@

spitest_SRCS=gpio_dir/spitest.cpp gpio_dir/para_spi.cpp gpio_dir/para_gpio.c gpio_dir/para_gpio.cpp
spitest_DEPS=Makefile gpio_dir/para_spi.h gpio_dir/para_gpio.h
spitest: $(spitest_DEPS)
	$(CC) $(spitest_SRCS) $(CLIBPP) $(CFLAGS) -o $@

facetest_SRCS=gpio_dir/facetest.cpp gpio_dir/para_face.cpp gpio_dir/para_spi.cpp gpio_dir/para_gpio.c gpio_dir/para_gpio.cpp
facetest_DEPS=Makefile gpio_dir/para_face.h gpio_dir/para_spi.h gpio_dir/para_gpio.h $(facetest_SRCS)
facetest: $(facetest_DEPS)
	$(CC) $(facetest_SRCS) $(CLIBPP) $(CFLAGS) -o $@

getfpga/getfpga: getfpga/getfpga.c
	$(CC) $< $(CFLAGS) -o $@

clean:
	rm -f xtemp/xtemp pmorse gpiotest porcutest spitest facetest getfpga/getfpga

install: install-exec

install-exec: xtemp/xtemp pmorse
	install -D xtemp/xtemp "$(DESTDIR)$(BINDIR)/xtemp"
	install -D pmorse "$(DESTDIR)$(BINDIR)/pmorse"

uninstall: uninstall-exec

uninstall-exec:
	rm "$(DESTDIR)$(BINDIR)/xtemp"
	rm "$(DESTDIR)$(BINDIR)/pmorse"

.PHONY: clean install install-exec uninstall uninstall-exec


