DESTDIR     ?=
prefix      ?= /usr
exec_prefix ?= $(prefix)
sbindir     ?= $(exec_prefix)/sbin
sysconfdir  ?= /etc

EPIPHANY_HOME ?= /opt/adapteva/esdk
CROSS_COMPILE ?= arm-linux-gnueabihf-

EXTRA_CFLAGS := $(CFLAGS)
CFLAGS  := -Wall -g $(EXTRA_CFLAGS)
LDLIBS  := $(LDFLAGS) -lm

ARCH    ?= $(shell uname -m)
ifneq ($(ARCH), armv7l)
	CC := $(CROSS_COMPILE)gcc
endif

.PHONY: all clean install uninstall

all: thermald parallella-thermald.conf parallella-thermald@.service

thermald: thermald.c

parallella-thermald.conf: parallella-thermald.conf.in
	sed s,@parallella-thermald-path@,$(sbindir)/parallella-thermald,g \
		< $< > $@

parallella-thermald@.service: parallella-thermald@.service.in
	sed s,@parallella-thermald-path@,$(sbindir)/parallella-thermald,g \
		< $< > $@


install: thermald parallella-thermald.conf parallella-thermald@.service parallella-thermald.default
	mkdir -p $(DESTDIR)$(sbindir)
	install -o root -g root thermald \
		$(DESTDIR)$(sbindir)/parallella-thermald
	mkdir -p $(DESTDIR)$(sysconfdir)/init
	install -o root -g root -m 644 parallella-thermald.conf \
		$(DESTDIR)$(sysconfdir)/init/parallella-thermald.conf
	mkdir -p $(DESTDIR)/lib/systemd/system
	install -o root -g root -m 644 parallella-thermald@.service \
		$(DESTDIR)/lib/systemd/system/parallella-thermald@.service
	mkdir -p $(DESTDIR)$(sysconfdir)/default
	install -o root -g root -m 644 parallella-thermald.default \
		$(DESTDIR)$(sysconfdir)/default/parallella-thermald


uninstall:
	rm -f $(DESTDIR)$(sbindir)/parallella-thermald
	rm -f $(DESTDIR)$(sysconfdir)/init/parallella-thermald.conf
	rm -f $(DESTDIR)/lib/systemd/system/parallella-thermald@.service
	rm -f $(DESTDIR)$(sysconfdir)/default/parallella-thermald

# Test target. Reads temperature sensors from /tmp/thermald.
thermald-test: thermald.c
	$(CC) -DTEST=1 $(CFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -f thermald thermald-test parallella-thermald.conf \
		parallella-thermald@.service



