# Makefile for building parser samples
#
# Set XDP2DIR to the install directory for XDP2
#

SRC = $(shell realpath ../../../src/)
XDP2DIR ?= /usr

INCDIR= $(XDP2DIR)/include
LIBDIR= $(XDP2DIR)/lib
BINDIR= $(XDP2DIR)/bin
CC= gcc
CFLAGS= -I$(INCDIR)
LDFLAGS= -L$(LIBDIR)
KDIR ?= /lib/modules/$(shell uname -r)/build
INSTALLDIR ?= /lib/modules/$(shell uname -r)/extra

# We depend on the classifier's symbols
CLS_SYMBOLS ?= $(SRC)/kernel/Module.symvers

TARGETS= xdp2_big.ko
TMPFILES= parser.kmod.c

.PHONY: all
all: $(TARGETS)

$(CLS_SYMBOLS):
	$(MAKE) -C $(SRC) kernel KDIR=$(KDIR)

parser.kmod.c: bigparser.c
	$(BINDIR)/xdp2-compiler $< $@

xdp2_big.ko: $(CLS_SYMBOLS) parser.kmod.c
	$(MAKE) -C $(KDIR) M=$(shell pwd) KBUILD_EXTRA_SYMBOLS=$(CLS_SYMBOLS) XDP2DIR=$(XDP2DIR)

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(shell pwd) $@
	@rm -f $(TARGETS) $(TMPFILES)

.PHONY: modules_install
modules_install: $(TARGETS)
	@install -m 0755 -d $(INSTALLDIR)
	install -m 0644 $(TARGETS) $(INSTALLDIR)
	depmod
