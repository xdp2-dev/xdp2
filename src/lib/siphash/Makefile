SRCDIR=../..

include $(SRCDIR)/config.mk

TARGETS = libsiphash.so libsiphash.a

CFLAGS += -fPIC -lc

UTILOBJ = siphash.o

.PHONY: all
all: $(TARGETS)

libsiphash.a: $(UTILOBJ) $(ADDLIB)
	$(QUIET_AR)$(AR) rcs $@ $^

libsiphash.so: $(UTILOBJ) $(ADDLIB)
	$(QUIET_CC)$(CC) $(LDLIBS) -shared $^ -o $@

.PHONY: install
install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(LIBDIR)

.PHONY: clean
clean:
	@rm -f $(UTILOBJ) $(ADDLIB) $(TARGETS)
