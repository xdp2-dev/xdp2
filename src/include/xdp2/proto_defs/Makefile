include ../../../config.mk

INCDIR=$(INSTALLDIR)$(HDRDIR)/xdp2/proto_defs

TARGETS = proto_arp_rarp.h

.PHONY: all
all: $(TARGETS)

.PHONY: install
install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $^ $(INCDIR)

.PHONY: clean
clean:
