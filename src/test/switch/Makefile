SRCDIR = ../..

include $(SRCDIR)/config.mk

TARGETS= test_switch

OBJ = $(TARGETS:%=%.o)

.PHONY: all
all: $(TARGETS)

$(TARGETS): %: %.o
	$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@

.PHONY: install
install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $< $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGETS) $(OBJ)
