SRCDIR=../..

include $(SRCDIR)/config.mk

TARGETS = liblzf_compress.so liblzf_compress.a
TARGETS += liblzf_decompress.so liblzf_decompress.a

CFLAGS += -fPIC

UTILOBJ = lzf_compress.o lzf_decompress.o

.PHONY: all
all: $(TARGETS)

liblzf_compress.a: $(UTILOBJ) $(ADDLIB)
	$(QUIET_AR)$(AR) rcs $@ $^

liblzf_compress.so: $(UTILOBJ) $(ADDLIB)
	$(QUIET_CC)$(CC) $(LDLIBS) -shared $^ -o $@

liblzf_decompress.a: $(UTILOBJ) $(ADDLIB)
	$(QUIET_AR)$(AR) rcs $@ $^

liblzf_decompress.so: $(UTILOBJ) $(ADDLIB)
	$(QUIET_CC)$(CC) $(LDLIBS) -shared $^ -o $@

.PHONY: install
install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(LIBDIR)

.PHONY: clean
clean:
	@rm -f $(UTILOBJ) $(ADDLIB) $(TARGETS)
