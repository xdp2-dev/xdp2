include ../../config.mk

TARGETS = test_bitmap

MAKE_FUNCS = make_bitmap_funcs bitmap_funcs.h

all: $(TARGETS)

LDLIBS += ../../lib/xdp2/libxdp2.a
LDLIBS += ../../../src/lib/cli/libcli.a

CFLAGS += -g

all: $(TARGETS)

test_bitmap: bitmap_funcs.h test_bitmap.o
	$(QUIET_LINK)$(CC) test_bitmap.o $(LDFLAGS) $(LDLIBS) -o $@

make_bitmap_funcs: make_bitmap_funcs.c
	$(QUIET_LINK)$(HOST_CC) -o $@ $^

bitmap_funcs.h: make_bitmap_funcs
	./make_bitmap_funcs > ./bitmap_funcs.h

install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $< $(INSTALLDIR)$(BINDIR)

clean:
	@rm -f $(TARGETS) $(TARGETS:%=%.o) $(MAKE_FUNCS)
