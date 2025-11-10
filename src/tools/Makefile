include ../config.mk

TOPTARGETS := all clean install

SUBDIRS = pmacro packets

ifneq ($(NO_BUILD_COMPILER),y)
	SUBDIRS += compiler
endif

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
