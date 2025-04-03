include ../../../config.mk

INCDIR=$(INSTALLDIR)$(HDRDIR)/xdp2/proto_defs

TARGETS = proto_arp_rarp.h proto_batman.h proto_ether.h proto_fcoe.h
TARGETS += proto_gre.h proto_icmp.h proto_igmp.h proto_ip.h proto_ipv4.h
TARGETS += proto_ipv4ip.h proto_ipv6.h proto_ipv6_eh.h proto_ipv6ip.h
TARGETS += proto_ipv6_nd.h proto_l2tp.h proto_l2tp_v0.h

.PHONY: all
all: $(TARGETS)

.PHONY: install
install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $^ $(INCDIR)

.PHONY: clean
clean:
