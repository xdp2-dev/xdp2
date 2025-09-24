SRCDIR = ../..

include $(SRCDIR)/config.mk
include parsers.mk

CFLAGS += -fPIC

UTILOBJ = vstruct.o timer.o cli.o pcap.o packets_helpers.o dtable.o
UTILOBJ += obj_allocator.o pvbuf.o pvpkt.o config_functions.o parser.o
UTILOBJ += accelerator.o locks.o

# Parser files are in parsers subdirectory

PARSERDIR = parsers

PARSEROBJSEXT = $(addprefix $(PARSERDIR)/, $(PARSEROBJS))

ALLOBJS = $(UTILOBJ)

TARGETS = libxdp2.so libxdp2.a

.PHONY: all
all: $(TARGETS)

XDP2_COMPILER = $(SRCDIR)/tools/compiler/xdp2-compiler

OPTIMIZED_PARSER=n

#ifneq ($(BUILD_OPT_PARSER),y)
PARSEROSEXT = $(addprefix $(PARSERDIR)/, $(PARSEROBJS))
#else

PARSERCSEXT = $(PARSEROBJSEXT:.o=.p.c)
PARSEROSEXT = $(PARSEROBJSEXT:.o=.p.o)

$(PARSERCSEXT): %.p.c: %.c
	$(XDP2_COMPILER) -I$(SRCDIR)/include -o $@ -i $<
#endif

CFLAGS += -I.
ALLOBJS += $(PARSEROSEXT)

libxdp2.a: $(ALLOBJS) $(ADDLIB)
	$(QUIET_AR)$(AR) rcs $@ $^

libxdp2.so: $(ALLOBJS) $(ADDLIB)
	$(CC) -shared $^ -o $@ -lpcap

.PHONY: install-libs
install-libs: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(LIBDIR)

.PHONY: install
install: install-libs

.PHONY: clean
clean:
	@rm -f $(ALLOBJS) $(TARGETS) $(PARSERCSEXT) $(PARSEROSEXT)
