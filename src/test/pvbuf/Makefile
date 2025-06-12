include ../../config.mk

TEST_TARGET = test_pvbuf

OBJS = test_pvbuf.o

LDLIBS = ../../../src/lib/xdp2/libxdp2.a
LDLIBS += ../../../src/lib/cli/libcli.a
LDLIBS += ../../../src/lib/siphash/libsiphash.a

.PHONY: all
all: $(TEST_TARGET)

$(TEST_TARGET): %: %.o
	$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@

.PHONY: install
install: $(TEST_TARGET)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $< $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TEST_TARGET) $(OBJS)
