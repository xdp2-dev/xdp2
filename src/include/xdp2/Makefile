SRCDIR = ../..

include $(SRCDIR)/config.mk

TOPTARGETS := all clean install

SUBDIRS =

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

INCDIR=$(INSTALLDIR)$(HDRDIR)/xdp2

TARGETS = compiler_helpers.h utility.h bswap.h bitmap_word.h

clean:

install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $(TARGETS) $(INCDIR)
