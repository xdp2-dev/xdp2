# Makefile for building parser samples
#
# Set XDP2DIR to the install directory for XDP2
#

XDP2DIR ?= /usr

INCDIR= $(XDP2DIR)/include
LIBDIR= $(XDP2DIR)/lib
BINDIR= $(XDP2DIR)/bin

XCC= clang
XCFLAGS= -I$(INCDIR)
XCFLAGS+= -g -O2
XLDFLAGS=

CC= gcc
CFLAGS= -I$(INCDIR)
CFLAGS+= -g
LDFLAGS= -L$(LIBDIR)

TARGETS= flow_tracker.xdp.o flow_parser
TMPFILES= parser.xdp.h parser.p.c parser.p.h

.PHONY: all
all: $(TARGETS)

parser.xdp.h: parser.c parser.o
	$(BINDIR)/xdp2-compiler -I$(INCDIR) -i $< -o $@

flow_tracker.xdp.o: flow_tracker.xdp.c parser.xdp.h
	$(XCC) -x c -target bpf -I$(INCDIR) $(XCFLAGS) $(XLDFLAGS) -c -o $@ $<

parser.p.c: parser.c parser.o
	$(BINDIR)/xdp2-compiler -I$(INCDIR) -i $< -o $@

flow_parser: flow_parser.c parser.p.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ flow_parser.c parser.p.c -lpcap -lxdp2 -lcli -lsiphash

.PHONY: clean
clean:
	@rm -f $(TARGETS) $(TMPFILES)
