include ../config.mk

TOPTARGETS := all clean install

SUBDIRS = pmacro compiler

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
