# Makefile for building parser samples
#
# Set XDP2DIR to the install directory for XDP2
#

XDP2DIR ?= /usr

INCDIR= $(XDP2DIR)/include
LIBDIR= $(XDP2DIR)/lib
BINDIR= $(XDP2DIR)/bin
CC= gcc
CFLAGS= -I$(INCDIR)
CFLAGS+= -g
LDFLAGS= -L$(LIBDIR)

# uapi files are not installed. If UAPI is set assume that we are in
# the simple_parser subdirectory of samples and derive a relative
# path to find src/include/uapi

ifeq ($(UAPI), 1)
CFLAGS += -I../../src/include/uapi
endif

TARGETS= parser
TMPFILES= parser.p.c parser.p.h

.PHONY: all
all: $(TARGETS)

parser.p.c: parser.c
	$(BINDIR)/xdp2-compiler $< $@

parser: parser.p.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lpcap -lxdp2

.PHONY: clean
clean:
	@rm -f $(TARGETS) $(TMPFILES)
