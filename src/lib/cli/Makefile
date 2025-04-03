SRCDIR=../..

include $(SRCDIR)/config.mk

TARGETS = libcli.so libcli.a

CFLAGS += -fPIC

UTILOBJ = cli.o

.PHONY: all
all: $(TARGETS)

libcli.a: $(UTILOBJ) $(ADDLIB)
	$(QUIET_AR)$(AR) rcs $@ $^

libcli.so: $(UTILOBJ) $(ADDLIB)
	$(CC) $(LDLIBS) -shared $^ -o $@

.PHONY: install
install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(LIBDIR)

.PHONY: clean
clean:
	@rm -f $(UTILOBJ) $(ADDLIB) $(TARGETS)
