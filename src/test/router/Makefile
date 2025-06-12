SRCDIR = ../..

include $(SRCDIR)/config.mk

TARGET= test_router

OBJS = main.o control.o dataplane.o

.PHONY: all
all: $(TARGET)

LDLIBS = $(SRCDIR)/lib/xdp2/libxdp2.a
LDLIBS += $(SRCDIR)/lib/cli/libcli.a
LDLIBS += -L$(SRCDIR)/../thirdparty/libpcap
LDLIBS += $(SRCDIR)/lib/siphash/libsiphash.a
LDLIBS += -lpcap

$(TARGET): $(OBJS)
	$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@

.PHONY: install
install: $(TARGET)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $< $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGET) $(OBJS)
