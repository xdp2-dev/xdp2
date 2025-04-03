include ../../../config.mk

INCDIR=$(INSTALLDIR)$(HDRDIR)/xdp2/proto_defs

TARGETS = proto_arp_rarp.h proto_batman.h proto_ether.h proto_fcoe.h
TARGETS += proto_gre.h proto_icmp.h proto_igmp.h proto_ip.h proto_ipv4.h

.PHONY: all
all: $(TARGETS)

.PHONY: install
install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $^ $(INCDIR)

.PHONY: clean
clean:
