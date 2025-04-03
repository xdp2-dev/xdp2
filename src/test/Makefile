include ../config.mk

TOPTARGETS := all clean install

SUBDIRS = switch tables parser parse_dump accelerator vstructs pvbuf

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

