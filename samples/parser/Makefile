# Makefile for building parser samples
#
# Set XDP2DIR to the install directory for XDP2 like
#	make XDP2DIR=~/xdp2/install
#

TOPTARGETS := all clean install

SUBDIRS = offset_parser simple_parser ports_parser

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
