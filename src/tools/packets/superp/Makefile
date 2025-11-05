SRCDIR = ../../..

include $(SRCDIR)/config.mk

TARGETS = get_superp_proto_numbers superp.pcap

all: $(TARGETS)

%.pcap: make_%.py
	$(QUIET_PYTHON)$(PYTHON) $< `./get_superp_proto_numbers`

get_superp_proto_numbers: get_superp_proto_numbers.c
	$(QUIET_CC)$(HOST_CC) $(CCOPTS) -I$(SRCDIR)/include $(CONFIG_DEFINES) $^ -o $@

.PHONY: install
install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGETS)
