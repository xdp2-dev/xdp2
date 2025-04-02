# Makefile for building parser samples
#
# Set XDP2DIR to the install directory for XDP2
#

XDP2DIR ?= /usr

INCDIR= $(XDP2DIR)/include
LIBDIR= $(XDP2DIR)/lib
BINDIR= $(XDP2DIR)/bin
XDP2COMPILER ?= $(BINDIR)
XCC= clang
XCFLAGS= -I$(INCDIR)
XCFLAGS+= -g -O2
XLDFLAGS=

TARGETS= flow_tracker.xdp.o
TMPFILES= parser.xdp.h parser.o flow_tracker.xdp.i flow_tracker.xdp.bc flow_tracker.xdp.s

.PHONY: all
all: $(TARGETS)

parser.xdp.h: parser.c
	$(CC) -I$(INCDIR) -c -o parser.o $<
	$(XDP2COMPILER)/xdp2-compiler -I$(INCDIR) -i $< -o $@

flow_tracker.xdp.o: flow_tracker.xdp.c parser.xdp.h flow_tracker.h
	$(XCC) -save-temps -fverbose-asm -x c -target bpf $(XCFLAGS) $(XLDFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	@rm -f $(TARGETS) $(TMPFILES)
