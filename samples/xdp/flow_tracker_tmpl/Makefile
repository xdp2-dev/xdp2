# Makefile for building parser samples
#
# Set XDP2DIR to the install directory for XDP2 like
# 	make XDP2DIR=~/xdp2/install
#

XDP2DIR ?= /usr

INCDIR= $(XDP2DIR)/include
LIBDIR= $(XDP2DIR)/lib
BINDIR= $(XDP2DIR)/bin
CC= clang # Use clang just to test it
CFLAGS= -I$(INCDIR)
CFLAGS+= -g -O2
LDFLAGS=

TARGETS= flow_tracker.xdp.o
TMPFILES= parser.xdp.h parser.o

.PHONY: all
all: $(TARGETS)

# We build parser.o but don't actually link it. This is done to detect any
# compiler errors before calling xdp2-compiler that doesn't seem to produce
# any errors if compilation fails (XXXTH Something to fix)
#
parser.xdp.h: parser.c parser.o
	$(BINDIR)/xdp2-compiler -I$(INCDIR) -i $< -o $@

flow_tracker.xdp.o: flow_tracker.xdp.c parser.xdp.h
	$(CC) -x c -target bpf $(CFLAGS) $(LDFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	@rm -f $(TARGETS) $(TMPFILES)
