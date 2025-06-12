include ../config.mk

TOPTARGETS := all clean install

SUBDIRS = vstructs switch tables timer pvbuf parser

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

