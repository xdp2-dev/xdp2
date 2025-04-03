include ../../config.mk

INCDIR=$(INSTALLDIR)$(HDRDIR)/parselite

TARGETS= parser.h
TARGETS+= proto_arp_rarp.h proto_batman.h proto_ether.h proto_fcoe.h
TARGETS+= proto_gre.h proto_icmp.h proto_igmp.h proto_ip.h
TARGETS+= proto_ipv4.h proto_ipv4ip.h proto_ipv6_eh.h proto_ipv6.h
TARGETS+= proto_ipv6ip.h proto_mpls.h proto_ports.h proto_ppp.h
TARGETS+= proto_sctp.h proto_tcp.h proto_tipc.h proto_vlan.h

.PHONY: all
all: $(TARGETS)

.PHONY: install
install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $^ $(INCDIR)

.PHONY: clean
clean:
