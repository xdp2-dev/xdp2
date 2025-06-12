SRCDIR = ../..

include $(SRCDIR)/config.mk
include parsers.mk

CFLAGS += -fPIC

UTILOBJ = vstruct.o timer.o cli.o pcap.o packets_helpers.o dtable.o
UTILOBJ += obj_allocator.o pvbuf.o pvpkt.o config_functions.o parser.o
UTILOBJ += accelerator.o

# Parser files are in parsers subdirectory

PARSERDIR = parsers

PARSEROBJSEXT = $(addprefix $(PARSERDIR)/, $(PARSEROBJS))

ALLOBJS = $(UTILOBJ)

TARGETS = libxdp2.so libxdp2.a

.PHONY: all
all: $(TARGETS)

OPTIMIZED_PARSER=n

#ifeq ($(OPTIMIZED_PARSER),n)
PARSEROSEXT = $(addprefix $(PARSERDIR)/, $(PARSEROBJS))
#else

#PARSERCSEXT = $(PARSEROBJSEXT:.o=.p.c)
#PARSEROSEXT = $(PARSEROBJSEXT:.o=.p.o)

$(PARSERCSEXT): %.p.c: %.c
	$(SRCDIR)/tools/compiler/xdp2-compiler $< $@
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
