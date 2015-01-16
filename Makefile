DESTDIR     ?=
prefix      ?= /usr
exec_prefix ?= $(prefix)
sbindir     ?= $(exec_prefix)/sbin
sysconfdir  ?= /etc

EPIPHANY_HOME ?= /opt/adapteva/esdk
CROSS_COMPILE ?= arm-linux-gnueabihf-

EXTRA_CFLAGS := $(CFLAGS)
CFLAGS  := -Wall -g -I$(EPIPHANY_HOME)/tools/host/include $(EXTRA_CFLAGS)
LDLIBS  := -L$(EPIPHANY_HOME)/tools/host/lib $(LDFLAGS) -lm -le-hal

ARCH    ?= $(shell uname -p)
ifneq ($(ARCH), armv7l)
	CC := $(CROSS_COMPILE)gcc
endif

.PHONY: all clean install uninstall

all: thermald parallella-thermald.conf

thermald: thermald.c

parallella-thermald.conf: parallella-thermald.conf.in
	sed s,@parallella-thermald-path@,$(sbindir)/parallella-thermald,g \
		< $< > $@

install: thermald parallella-thermald.conf
	install -o root -g root thermald \
		$(DESTDIR)$(sbindir)/parallella-thermald
	install -o root -g root -m 644 parallella-thermald.conf \
		$(DESTDIR)$(sysconfdir)/init/parallella-thermald.conf

uninstall:
	rm -f $(DESTDIR)$(sbindir)/parallella-thermald
	rm -f $(DESTDIR)$(sysconfdir)/init/parallella-thermald.conf

# Test target. Reads temperature sensors from /tmp/thermald.
thermald-test: thermald.c
	$(CC) -DTEST=1 $(CFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -f thermald thermald-test parallella-thermald.conf



