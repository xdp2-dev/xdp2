# Top level Makefile

# Only getting INSTALLDIR from config.mk
TOP_LEVEL_MAKE=y
include config.mk
TOP_LEVEL_MAKE=

ifeq ($(VERBOSE),0)
MAKEFLAGS += --no-print-directory -s
MFLAGS += -s
endif

ifneq ($(V), 1)
MFLAGS += -s
endif

LIBDIR?=/lib
SBINDIR?=/sbin
BINDIR?=/bin
ETCDIR?=/etc
CONFDIR?=/etc
DATADIR?=/share
HDRDIR?=/include
MANDIR?=$(DATADIR)/man
KERNEL_INCLUDE?=/usr/include
BASH_COMPDIR?=$(DATADIR)/bash-completion/completions

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRDIR := $(dir $(MKFILE_PATH))

SHARED_LIBS = y

ifneq ($(SHARED_LIBS),y)
DEFINES+= -DNO_SHARED_LIBS
endif

ifeq ($(OPTIMIZED_PARSER),n)
DEFINES+= -DNO_OPTIMIZED_PARSER
endif

DEFINES+=-DCONFDIR=\"$(CONFDIR)\"

CC := gcc
INSTALL := install
HOSTCC ?= $(CC)

#DEFINES += -D_GNU_SOURCE
# Turn on transparent support for LFS
DEFINES += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE

CCOPTS ?= -O2
WFLAGS := -Wall -Wstrict-prototypes  -Wmissing-prototypes
WFLAGS += -Wmissing-declarations -Wold-style-definition -Wformat=2

CFLAGS := $(WFLAGS) $(CCOPTS) -I$(CURRDIR)/include
ifeq ($(UAPI), 1)
CFLAGS += -I$(CURRDIR)/include/uapi
endif
CFLAGS += $(DEFINES)

YACCFLAGS = -d -t -v
LDFLAGS += -L$(CURRDIR)/libpcap

# For kernel modules
KDIR ?= /lib/modules/`uname -r`/build

SUBDIRS= libpcap

all: config.mk $(EXTRA_TARGETS)
	@set -e; \
	for i in $(SUBDIRS); \
	do echo; echo $$i; $(MAKE) $(MFLAGS) -C $$i; done

config.mk:
	sh configure $(KERNEL_INCLUDE)

install: all
	@install -m 0755 -d $(INSTALLDIR)$(SBINDIR)
	@install -m 0755 -d $(INSTALLDIR)$(BINDIR)
	@install -m 0755 -d $(INSTALLDIR)$(LIBDIR)
	@install -m 0755 -d $(INSTALLDIR)$(ETCDIR)
	@install -m 0755 -d $(INSTALLDIR)$(HDRDIR)
	@for i in $(SUBDIRS); \
	do echo; echo $$i; $(MAKE) $(MFLAGS) -C $$i install; done

install_tar: install
	@tar --directory=$(INSTALLDIR) . -czf install.tgz		\
			`echo $(INSTALLDIR) | awk -F/ '{ print $$NF }'`

test_install_tar: install
	cp -r test/parser/ $(INSTALLDIR)/
	cp -r ../data/pcaps $(INSTALLDIR)/parser/
	@tar --directory=$(INSTALLDIR) . -czf install.tgz		\
			`echo $(INSTALLDIR) | awk -F/ '{ print $$NF }'`

snapshot:
	echo "static const char SNAPSHOT[] = \""`date +%y%m%d`"\";" \
		> include/SNAPSHOT.h

help:
	@echo "For verbose output: make V=1"
	@echo "To include UAPI headers: make UAPI=1"

clean:
	@for i in $(SUBDIRS) ;			\
		do $(MAKE) $(MFLAGS) -C $$i clean; done

clobber:
	touch config.mk
	$(MAKE) $(MFLAGS) clean
	rm -f config.mk cscope.*

distclean: clobber

cscope:
	cscope -b -q -R -Iinclude -sip -slib -smisc -snetem -stc

.EXPORT_ALL_VARIABLES:
