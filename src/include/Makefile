SRCDIR = ..

include $(SRCDIR)/config.mk

TOPTARGETS := all clean install

SUBDIRS = cli siphash xdp2 parselite uet falcon sue

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
