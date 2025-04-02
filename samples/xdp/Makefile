TOPTARGETS := all clean install

SUBDIRS = flow_tracker_combo flow_tracker_simple

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
