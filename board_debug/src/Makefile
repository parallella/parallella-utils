CC=gcc
CPP=g++
CFLAGS=-Wall
CLIBX=-lX11
CPTHRD=-pthread
CLIBRT=-lrt
CLIBPP=-lstdc++
CFLAGS_S=-Wall -static -g

DESTDIR=../bin

$(DESTDIR)/f-test: f-test.c f-link.c elink2.h ztemp.c
	$(CC) -o $(DESTDIR)/f-test f-test.c f-link.c ztemp.c $(CFLAGS)

static: $(DESTDIR)/f-test-static

$(DESTDIR)/f-test-static: f-test.c f-link.c elink2.h ztemp.c
	$(CC) -o $(DESTDIR)/f-test-static f-test.c f-link.c ztemp.c $(CFLAGS_S)

.PHONY: static
