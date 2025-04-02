include ../config.mk

TOPTARGETS := all clean install

SUBDIRS = vstructs switch

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

