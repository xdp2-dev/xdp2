# Makefile for building xdp samples
#
# Set XDP2DIR to the install directory for XDP2 like
#	make XDP2DIR=~/xdp2/install
#

TOPTARGETS := all clean install

SUBDIRS = flow_tracker_combo flow_tracker_simple flow_tracker_tlvs

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
