SRCDIR = ..
include $(SRCDIR)/config.mk

TOPTARGETS := all clean install

SUBDIRS = cli siphash

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

