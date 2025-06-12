# Force no static build

SRCDIR = ../..

include $(SRCDIR)/config.mk

TARGET = parse_dump
OBJ = main.o parser.o print_meta.o tables.o
LDLIBS = $(SRCDIR)/lib/xdp2/libxdp2.a $(SRCDIR)/lib/cli/libcli.a
LDLIBS += -L$(SRCDIR)/../thirdparty/libpcap
LDLIBS += $(SRCDIR)/lib/siphash/libsiphash.a
LDLIBS +=-lpcap -lbsd

$(TARGET): $(OBJ)
	$(QUIET_LINK)$(CC) $^  $(LDLIBS) -o $@

.PHONY: install
install: $(TARGET)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGET) $(OBJ)

