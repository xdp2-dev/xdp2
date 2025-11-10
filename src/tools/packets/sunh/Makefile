SRCDIR = ../../..

include $(SRCDIR)/config.mk

TARGETS = get_sunh_ether_type sunh.pcap

all: $(TARGETS)

GET_SUNH_ETHER_TYPE = get_sunh_ether_type

%.pcap: make_%.py $(GET_SUNH_ETHER_TYPE)
	$(QUIET_PYTHON)$(PYTHON) $< `./$(GET_SUNH_ETHER_TYPE)`

get_sunh_ether_type: get_sunh_ether_type.c
	$(QUIET_CC)$(HOST_CC) $(CCOPTS) -I$(SRCDIR)/include $(CONFIG_DEFINES) $^ -o $@

.PHONY: install
install: $(TARGETS)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGETS)
