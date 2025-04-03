SRCDIR = ..
include $(SRCDIR)/config.mk

TOPTARGETS := all clean install

SUBDIRS = cli siphash crc flowdis lzf

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

