SRCDIR = ../..

include $(SRCDIR)/config.mk

TOPTARGETS := all clean install

.PHONY: $(TOPTARGETS)

INCDIR=$(INSTALLDIR)$(HDRDIR)/falcon

TARGETS = falcon.h proto_falcon.h debug.h
TARGETS += config.h protocol.h parser_test.h

all:

install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $(TARGETS) $(INCDIR)
