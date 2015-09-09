# Possible DEFINES:
#   -DBROKEN_64B_WRITES
#   -DBROKEN_64B_READS
#   -DSLOW_EPIPHANY_TX

CFLAGS=-Wall -g -DBROKEN_64B_WRITES
DESTDIR=../bin
SOURCES=f-test.c f-link.c ztemp.c
DEPS=$(SOURCES) elink2.h Makefile

$(DESTDIR)/f-test: $(DEPS)
	$(CC) $(CFLAGS) $(SOURCES) -o $@

$(DESTDIR)/f-test-static: $(DEPS)
	$(CC) -static $(CFLAGS) $(SOURCES) -o $@

.PHONY: static
static: $(DESTDIR)/f-test-static

clean:
	rm -f $(DESTDIR)/f-test $(DESTDIR)f-test-static
