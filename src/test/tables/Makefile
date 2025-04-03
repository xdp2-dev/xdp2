SRCDIR = ../..

include $(SRCDIR)/config.mk

TARGET= test_tables

OBJS = test_table.o
OBJS += sftable_plain.o sftable_tern.o sftable_lpm.o
OBJS += dftable_plain.o dftable_tern.o dftable_lpm.o
OBJS += stable_plain.o stable_tern.o stable_lpm.o
OBJS += dtable_plain.o dtable_tern.o dtable_lpm.o

.PHONY: all
all: $(TARGET)

LDLIBS = $(SRCDIR)/lib/xdp2/libxdp2.a
LDLIBS += $(SRCDIR)/lib/cli/libcli.a
LDLIBS += $(SRCDIR)/lib/siphash/libsiphash.a

$(TARGET): $(OBJS)
	$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@

.PHONY: install
install: $(TARGET)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $< $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGET) $(OBJS)
