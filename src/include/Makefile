SRCDIR = ..

include $(SRCDIR)/config.mk

TOPTARGETS := all clean install

SUBDIRS = siphash xdp2 parselite kernel

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
