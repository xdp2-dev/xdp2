SRCDIR = ../..

include $(SRCDIR)/config.mk

TARGETS= test_accel

OBJ = $(TARGETS:%=%.o)

.PHONY: all
all: $(TARGETS)

LDLIBS += $(SRCDIR)/lib/xdp2/libxdp2.a
LDLIBS += $(SRCDIR)/lib/cli/libcli.a

$(TARGETS): %: %.o
	$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@

.PHONY: install
install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $< $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGETS) $(OBJ)
