# Makefile for building offset parser sample
#
# Set XDP2DIR to the install directory for XDP2 like
# 	make XDP2DIR=~/xdp2/install
#

XDP2DIR ?= /usr

INCDIR= $(XDP2DIR)/include
LIBDIR= $(XDP2DIR)/lib
BINDIR= $(XDP2DIR)/bin
CC= gcc
CFLAGS= -I$(INCDIR)
CFLAGS+= -g
LDFLAGS= -L$(LIBDIR)

TARGETS= parser
TMPFILES= parser.p.c parser.p.h parser.o

.PHONY: all
all: $(TARGETS)

# We build parser.o but don't actually link it. This is done to detect any
# compiler errors before calling xdp2-compiler that doesn't seem to produce
# any errors if compilation fails (XXXTH Something to fix)

parser.p.c: parser.c parser.o
	$(BINDIR)/xdp2-compiler -I$(INCDIR) -i $< -o $@

parser: parser.p.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lpcap -lxdp2 -lcli -lsiphash

.PHONY: clean
clean:
	@rm -f $(TARGETS) $(TMPFILES)
