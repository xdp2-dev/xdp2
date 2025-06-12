SRCDIR = ../..

include $(SRCDIR)/config.mk

CFLAGS += -fPIC

UTILOBJ = vstruct.o timer.o

ALLOBJS = $(UTILOBJ)

TARGETS = libxdp2.so libxdp2.a

.PHONY: all
all: $(TARGETS)

CFLAGS += -I.

libxdp2.a: $(ALLOBJS) $(ADDLIB)
	$(QUIET_AR)$(AR) rcs $@ $^

libxdp2.so: $(ALLOBJS) $(ADDLIB)
	$(CC) -shared $^ -o $@ -lpcap

.PHONY: install-libs
install-libs: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(LIBDIR)

.PHONY: install
install: install-libs

.PHONY: clean
clean:
	@rm -f $(ALLOBJS) $(TARGETS)
