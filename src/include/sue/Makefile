SRCDIR = ../..

include $(SRCDIR)/config.mk

TOPTARGETS := all clean install

.PHONY: $(TOPTARGETS)

INCDIR=$(INSTALLDIR)$(HDRDIR)/sue

TARGETS = sue.h proto_sue.h parser_test.h

all:

install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $(TARGETS) $(INCDIR)
