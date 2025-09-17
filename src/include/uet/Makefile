SRCDIR = ../..

include $(SRCDIR)/config.mk

TOPTARGETS := all clean install

.PHONY: $(TOPTARGETS)

INCDIR=$(INSTALLDIR)$(HDRDIR)/uet

TARGETS = pds.h  proto_uet_pds.h  proto_uet_ses.h  ses.h

all:

install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $(TARGETS) $(INCDIR)
