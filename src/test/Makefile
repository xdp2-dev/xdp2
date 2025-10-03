include ../config.mk

TOPTARGETS := all clean install

SUBDIRS = vstructs switch tables timer pvbuf parser parse_dump
SUBDIRS += accelerator router bitmaps uet falcon

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

