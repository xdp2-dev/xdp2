SRCDIR = ../../..

include $(SRCDIR)/config.mk

TARGETS = get_sue_udp_port sue.pcap

all: $(TARGETS)

%.pcap: make_%.py
	$(QUIET_PYTHON)$(PYTHON) $< `./get_sue_udp_port`

get_sue_udp_port: get_sue_udp_port.c
	$(QUIET_CC)$(HOST_CC) $(CCOPTS) -I$(SRCDIR)/include $(CONFIG_DEFINES) $^ -o $@

.PHONY: install
install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGETS)
