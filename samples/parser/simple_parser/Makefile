# Makefile for building simple parser sample
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

TARGETS= parser_tmpl parser_notmpl
TMPFILES= parser_notmpl.p.c parser_tmpl.p.c parser_notmpl.p.h parser_tmpl.p.h
TMPFILES+= parser_tmpl.o parser_notmpl.o
TMPFILES+= parser_notmpl.ll parser_notmpl.json
TMPFILES+= parser_tmpl.ll parser_otmpl.json

XDP2_COMPILER = $(BINDIR)/xdp2-compiler

ifeq ($(BUILD_PARSER_JSON),y)
PARSER_JSON = parser_notmpl.json parser_tmpl.json
endif

.PHONY: all
all: $(TARGETS) $(PARSER_JSON)

# We build parser_notmpl.o and parser_tmpl.o but don't actually link then. This
# is done to detect any compiler errors before calling xdp2-compiler that
# doesn't seem to produce any errors if compilation fails (XXXTH Something to
# fix)

parser_notmpl.p.c: parser_notmpl.c parser_notmpl.o
	$(BINDIR)/xdp2-compiler -I$(INCDIR) -i $< -o $@

parser_notmpl: parser_notmpl.p.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lpcap -lxdp2 -lcli -lsiphash

parser_notmpl.ll: parser_notmpl.c
	clang $(CFLAGS) $(LDFLAGS) -S $< -emit-llvm

parser_notmpl.json: parser_notmpl.ll
	$(XDP2_COMPILER) -I$(INCDIR) -o $@ -l $< -i ${<:%.ll=%.c}

parser_tmpl.p.c: parser_tmpl.c parser_tmpl.o
	$(BINDIR)/xdp2-compiler -I$(INCDIR) -i $< -o $@

parser_tmpl: parser_tmpl.p.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lpcap -lxdp2 -lcli -lsiphash

parser_tmpl.ll: parser_tmpl.c
	clang $(CFLAGS) $(LDFLAGS) -S $< -emit-llvm

parser_tmpl.json: parser_tmpl.ll
	$(XDP2_COMPILER) -I$(INCDIR) -o $@ -l $< -i ${<:%.ll=%.c}

.PHONY: clean
clean:
	@rm -f $(TARGETS) $(TMPFILES)
