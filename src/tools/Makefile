include ../config.mk

TOPTARGETS := all clean install

SUBDIRS = pmacro

ifeq ($(BUILD_OPT_PARSER),y)
	SUBDIRS += compiler
endif

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
