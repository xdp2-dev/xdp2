include ../../../config.mk

INCDIR=$(INSTALLDIR)$(HDRDIR)/xdp2/parsers

TARGETS = parser_big.h parser_simple_hash.h

.PHONY: all
all: $(TARGETS)

.PHONY: install
install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $^ $(INCDIR)

.PHONY: clean
clean:
