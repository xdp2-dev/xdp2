SRCDIR=../..

include $(SRCDIR)/config.mk

TARGETS = libcrc.so libcrc.a

CFLAGS += -fPIC

UTILOBJ = crc16.o crc32c.o crc64.o crcspeed.o

.PHONY: all
all: $(TARGETS)

libcrc.a: $(UTILOBJ) $(ADDLIB)
	$(QUIET_AR)$(AR) rcs $@ $^

libcrc.so: $(UTILOBJ) $(ADDLIB)
	$(QUIET_CC)$(CC) $(LDLIBS) -shared $^ -o $@

.PHONY: install
install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(LIBDIR)

.PHONY: clean
clean:
	@rm -f $(UTILOBJ) $(ADDLIB) $(TARGETS)
