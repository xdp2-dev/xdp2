SRCDIR=../../../../../../src

include $(SRCDIR)/config.mk

TARGETS = arch.h

INCDIR=$(INSTALLDIR)$(HDRDIR)/xdp2/arch_generic

.PHONY: all
all: $(TARGETS)

.PHONY: install
install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $^ $(INCDIR)
	@unlink $(INCDIR)/../arch 2>/dev/null || true
	@ln -s arch_generic $(INCDIR)/../arch

.PHONY: clean
clean:
