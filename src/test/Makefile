include ../config.mk

TOPTARGETS := all clean install

SUBDIRS = vstructs switch tables

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

