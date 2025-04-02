TOPTARGETS := all clean install

SUBDIRS = flow_tracker_combo flow_tracker_simple flow_tracker_tmpl

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
