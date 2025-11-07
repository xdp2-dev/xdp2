SRCDIR = ..
include $(SRCDIR)/config.mk

TOPTARGETS := all clean install

SUBDIRS = cli siphash crc flowdis lzf murmur3hash xdp2 parselite

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

