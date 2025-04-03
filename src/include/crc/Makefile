include ../../config.mk

TARGETS= crc.h crc32c.h crc64.h

INCDIR=$(INSTALLDIR)$(HDRDIR)/crc

.PHONY: all
all: $(TARGETS)

.PHONY: install
install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $^ $(INCDIR)

.PHONY: clean
clean:
