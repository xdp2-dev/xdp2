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
TMPFILES= parser.p.c parser.p.h parser.o parser.ll parser.json

XDP2_COMPILER = $(BINDIR)/xdp2-compiler

ifeq ($(BUILD_PARSER_JSON),y)
PARSER_JSON = parser.json
endif

.PHONY: all
all: $(TARGETS) $(PARSER_JSON)

# We build parser.o but don't actually link it. This is done to detect any
# compiler errors before calling xdp2-compiler that doesn't seem to produce
# any errors if compilation fails (XXXTH Something to fix)

parser.p.c: parser.c parser.o
	$(XDP2_COMPILER) -I$(INCDIR) -i $< -o $@

parser: parser.p.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lpcap -lxdp2 -lcli -lsiphash

parser.ll: parser.c
	clang $(CFLAGS) $(LDFLAGS) -S $< -emit-llvm

parser.json: parser.ll
	$(XDP2_COMPILER) -I$(INCDIR) -o $@ -l $< -i ${<:%.ll=%.c}

.PHONY: clean
clean:
	@rm -f $(TARGETS) $(TMPFILES)
