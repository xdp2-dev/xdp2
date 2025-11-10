include ../../config.mk

TOPTARGETS := all clean install

SUBDIRS = uet falcon sue sunh superp

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
