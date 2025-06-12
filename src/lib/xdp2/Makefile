SRCDIR = ../..

include $(SRCDIR)/config.mk

CFLAGS += -fPIC

UTILOBJ = vstruct.o timer.o cli.o pcap.o packets_helpers.o dtable.o
UTILOBJ += obj_allocator.o pvbuf.o pvpkt.o config_functions.o

ALLOBJS = $(UTILOBJ)

TARGETS = libxdp2.so libxdp2.a

.PHONY: all
all: $(TARGETS)

CFLAGS += -I.

libxdp2.a: $(ALLOBJS) $(ADDLIB)
	$(QUIET_AR)$(AR) rcs $@ $^

libxdp2.so: $(ALLOBJS) $(ADDLIB)
	$(CC) -shared $^ -o $@ -lpcap

.PHONY: install-libs
install-libs: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(LIBDIR)

.PHONY: install
install: install-libs

.PHONY: clean
clean:
	@rm -f $(ALLOBJS) $(TARGETS)
