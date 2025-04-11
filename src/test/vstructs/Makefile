SRCDIR = ../..

include $(SRCDIR)/config.mk

TEST_TARGET = test_vstructs

OBJS = test_vstructs.o

LDLIBS = $(SRCDIR)/lib/xdp2/libxdp2.a
LDLIBS += $(SRCDIR)/lib/cli/libcli.a
LDLIBS += -lpthread

.PHONY: all
all: $(TEST_TARGET)

$(TEST_TARGET): %: %.o
	$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@ $(LDLIBS_LOCAL)

.PHONY: install
install: $(TEST_TARGET)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $< $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TEST_TARGET) $(OBJS)
