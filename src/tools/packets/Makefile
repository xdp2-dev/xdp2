include ../../config.mk

TOPTARGETS := all clean install

SUBDIRS = uet falcon sue

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
