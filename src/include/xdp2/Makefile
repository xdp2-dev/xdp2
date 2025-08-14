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
TARGETS += bitmap.h bitmap_helpers.h pmacro.h _pmacro_gen.h
TARGETS += vstruct.h switch.h locks.h timer.h cli.h pcap.h
TARGETS += packets_helpers.h

PMACRO_GEN = $(SRCDIR)/tools/pmacro/pmacro_gen

PMACRO_TARG = _pmacro_gen.h

PMACRO_MAX_ARGS ?= 256
# Generate pmacro_gen.h from pmacro_gen tool
$(PMACRO_TARG):
	if  [ "$(VERBOSE)" = "0" ]; then \
		echo "    MACROGEN $@"; \
	fi
	$(PMACRO_GEN) $(PMACRO_MAX_ARGS) > $@

all: $(PMACRO_TARG)

clean:
	@rm -f $(PMACRO_TARG)

install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $(TARGETS) $(INCDIR)
