SRCDIR = ../..

include $(SRCDIR)/config.mk

TARGET= test_falcon

OBJS = main.o cli.o test_packets_rx.o test_packets_tx.o
OBJS += test_packets.o

.PHONY: all
all: $(TARGET)

LDLIBS = $(SRCDIR)/lib/xdp2/libxdp2.a
LDLIBS += $(SRCDIR)/lib/cli/libcli.a

$(TARGET): $(OBJS)
	$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@

.PHONY: install
install: $(TARGET)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $< $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGET) $(OBJS)
