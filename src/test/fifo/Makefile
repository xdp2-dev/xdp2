# Force no static build

NO_STATIC_BUILD = y

include ../../config.mk

TEST_TARGET = test_fifo

LDLIBS_LOCAL = ../../../src/lib/xdp2/libxdp2.a

.PHONY: all
all: $(TARGETS)

# No static build of FIFOs test

$(TEST_TARGET): %: %.o
	$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@

.PHONY: install
install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $< $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGETS) $(OBJ)
