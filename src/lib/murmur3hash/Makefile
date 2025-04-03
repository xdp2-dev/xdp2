SRCDIR=../..
include $(SRCDIR)/config.mk

TARGETS = libmurmur3hash.so libmurmur3hash.a

CFLAGS += -fPIC -lc

UTILOBJ = murmur3_hash.o

.PHONY: all
all: $(TARGETS)

libmurmur3hash.a: $(UTILOBJ) $(ADDLIB)
	$(QUIET_AR)$(AR) rcs $@ $^

libmurmur3hash.so: $(UTILOBJ) $(ADDLIB)
	$(QUIET_CC)$(CC) $(LDLIBS) -shared $^ -o $@

.PHONY: install
install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(LIBDIR)

.PHONY: clean
clean:
	@rm -f $(UTILOBJ) $(ADDLIB) $(TARGETS)
