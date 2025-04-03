include ../../config.mk

TARGETS= murmur3_hash.h

INCDIR=$(INSTALLDIR)$(HDRDIR)/murmur3_hash

.PHONY: all
all: $(TARGETS)

.PHONY: install
install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $^ $(INCDIR)

.PHONY: clean
clean:
