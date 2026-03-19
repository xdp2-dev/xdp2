SRCDIR = ../..

include $(SRCDIR)/config.mk

TOPTARGETS := all clean install

.PHONY: $(TOPTARGETS)

INCDIR=$(INSTALLDIR)$(HDRDIR)/superp

TARGETS = parser_test.h proto_superp.h superp.h

all:

install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $(TARGETS) $(INCDIR)
