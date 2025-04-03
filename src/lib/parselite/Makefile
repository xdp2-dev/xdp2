include ../../config.mk

CFLAGS += -fPIC

UTILOBJ = parser.o

TARGETS = libparselite.so libparselite.a

.PHONY: all
all: $(TARGETS)

libparselite.a: $(UTILOBJ) $(ADDLIB)
	$(QUIET_AR)$(AR) rcs $@ $^

libparselite.so: $(UTILOBJ) $(ADDLIB)
	$(CC) -shared $^ -o $@

.PHONY: install
install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(LIBDIR)

.PHONY: clean
clean:
	@rm -f $(UTILOBJ) $(ADDLIB) $(TARGETS)
