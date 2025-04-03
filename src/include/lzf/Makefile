include ../../config.mk

TARGETS= lzf_compress.h lzf_decompress.h

INCDIR=$(INSTALLDIR)$(HDRDIR)/lzf

.PHONY: all
all: $(TARGETS)

.PHONY: install
install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $^ $(INCDIR)

.PHONY: clean
clean:
