include ../../config.mk

TOPTARGETS := all clean install

SUBDIRS = uet falcon sue sunh superp

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
