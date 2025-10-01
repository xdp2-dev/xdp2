// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2025 Tom Herbert
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* XDP2 main parsing logic */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <linux/dccp.h>
#include <linux/icmp.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/if_pppox.h>
#include <linux/igmp.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/ppp_defs.h>
#include <linux/types.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include "parselite/parser.h"
#include "parselite/parser.h"
#include "parselite/proto_arp_rarp.h"
#include "parselite/proto_batman.h"
#include "parselite/proto_ether.h"
#include "parselite/proto_fcoe.h"
#include "parselite/proto_gre.h"
#include "parselite/proto_icmp.h"
#include "parselite/proto_igmp.h"
#include "parselite/proto_ip.h"
#include "parselite/proto_ipv4.h"
#include "parselite/proto_ipv6.h"
#include "parselite/proto_ipv6_eh.h"
#include "parselite/proto_mpls.h"
#include "parselite/proto_ports.h"
#include "parselite/proto_ppp.h"
#include "parselite/proto_ipv4ip.h"
#include "parselite/proto_ipv6ip.h"
#include "parselite/proto_sctp.h"
#include "parselite/proto_tcp.h"
#include "parselite/proto_tipc.h"
#include "parselite/proto_vlan.h"
#include "siphash/siphash.h"

#define GRE_CSUM_OMASK		__cpu_to_be16(0x0)
#define GRE_ROUTING_OMASK	(GRE_CSUM | GRE_CSUM_OMASK)
#define GRE_KEY_OMASK		(GRE_ROUTING | GRE_ROUTING_OMASK)
#define GRE_SEQ_OMASK		(GRE_KEY | GRE_KEY_OMASK)
#define GRE_ACK_OMASK		(GRE_SEQ | GRE_SEQ_OMASK)

static inline size_t __gre_flags_length(unsigned int flags, unsigned int mask)
{
	size_t len = 0;

	flags |= mask;

	if (flags & GRE_CSUM)
		len += 4;

	if (flags & GRE_KEY)
		len += 4;

	if (flags & GRE_SEQ)
		len += 4;

	if (flags & GRE_ACK)
		len += 4;

	return len;
}

static inline size_t gre_v0_flags_length(unsigned int flags, unsigned int mask)
{
	return __gre_flags_length(flags, mask & ~GRE_ACK);
}

static inline size_t gre_v1_flags_length(unsigned int flags, unsigned int mask)
{
	return __gre_flags_length(flags, mask);
}

static inline ssize_t gre_v0_flags_offset(unsigned int flags,
					  unsigned int flag,
					  unsigned int mask)
{
	if (!(flags & flag))
		return -1;

	return gre_v0_flags_length(flags, mask);
}

static inline ssize_t gre_v1_flags_offset(unsigned int flags,
					  unsigned int flag,
					  unsigned int mask)
{
	if (!(flags & flag))
		return -1;

	return gre_v1_flags_length(flags, mask);
}

static inline __u8 ipv6_get_dsfield(const struct ipv6hdr *ipv6h)
{
	return ntohs(*(__be16 *)ipv6h) >> 4;
}

/* Parse a packet
 *
 * Arguments:
 *   - hdr: pointer to start of packet
 *   - len: length of packet
 *   - metadata: metadata structure
 *   - flags: allowed parameterized parsing
 *   - max_encaps: maximum encapsulation layers
 */
bool parselite_parse(void *hdr, size_t len,
		     struct parselite_metadata *metadata,
		     unsigned int flags, unsigned int max_encaps,
		     unsigned int start_mode)
{
	struct ethhdr *eth;
	bool ret = false;
	__u8 ip_proto;
	size_t hlen;

	switch (start_mode) {
	case PARSELITE_START_ETHER:
		break;
	case PARSELITE_START_ETHTYPE:
		/* metadata->eth_proto carries protocol already */
		goto switch_ether_type;
	case PARSELITE_START_IP: {
		struct ip_hdr_byte *ihb;

		hlen = sizeof(*ihb);
		if (len < hlen)
			goto doreturn;

		ihb = hdr;
		switch (ihb->version) {
		case 4:
			metadata->eth_proto =  __cpu_to_be16(ETH_P_IP);
			goto switch_ether_type;
		case 6:
			metadata->eth_proto =  __cpu_to_be16(ETH_P_IPV6);
			goto switch_ether_type;
		default:
			goto doreturn;
		}
	}
	default:
		goto doreturn;
	}

	hlen = sizeof(*eth);
	if (len < hlen)
		return false;

	eth = hdr;
	hdr += hlen;
	len -= hlen;

	metadata->eth_proto = eth->h_proto;
	memcpy(metadata->eth_addrs, &eth->h_dest,
	       sizeof(metadata->eth_addrs));

	/* Process an EtherType */

switch_ether_type:
	switch (metadata->eth_proto) {
	case __cpu_to_be16(ETH_P_IP):
	{
		struct iphdr *iph;
		bool is_frag;

		if (len < sizeof(*iph))
			goto doreturn;

		hlen = ipv4_len(hdr);
		if (len < hlen)
			goto doreturn;

		iph = hdr;
		hdr += hlen;
		len -= hlen;

		ip_proto = iph->protocol;
		is_frag = !!(iph->frag_off & htons(IP_MF | IP_OFFSET));

		metadata->ip_proto = ip_proto;
		metadata->tos = iph->tos;
		metadata->ttl = iph->ttl;
		metadata->addr_type = PARSELITE_ATYPE_IPV4;
		memcpy(metadata->addrs.v4_addrs, &iph->saddr,
		       sizeof(metadata->addrs.v4_addrs));

		if (is_frag) {
			metadata->is_fragment = 1;
			metadata->first_frag =
				!(iph->frag_off & htons(IP_OFFSET));

			if (!(flags & PARSELITE_F_PARSE_1STFRAG) ||
			    (iph->frag_off & htons(IP_OFFSET))) {
				ret = true;
				goto doreturn;
			}
		}

		break;
	}
	case __cpu_to_be16(ETH_P_IPV6):
	{
		struct ipv6hdr *iph;
		__u32 flowlabel;

		hlen = sizeof(*iph);
		if (len < hlen)
			goto doreturn;

		iph = hdr;
		hdr += hlen;
		len -= hlen;

		ip_proto = iph->nexthdr;
		flowlabel = ip6_flowlabel(iph);

		metadata->ip_proto = iph->nexthdr;
		metadata->tos = ipv6_get_dsfield(iph);
		metadata->ttl = iph->hop_limit;
		metadata->addr_type = PARSELITE_ATYPE_IPV6;
		metadata->flow_label = ntohl(flowlabel);
		memcpy(metadata->addrs.v6_addrs, &iph->saddr,
		       sizeof(metadata->addrs.v6_addrs));

		if (flowlabel && (flags & PARSELITE_F_STOP_FLOWLABEL)) {
			ret = true;
			goto doreturn;
		}

		break;
	}
	case __cpu_to_be16(ETH_P_8021AD):
	case __cpu_to_be16(ETH_P_8021Q):
	{
		__be16 save_eth_proto = metadata->eth_proto;
		struct vlan_hdr *vlan;
		int index;

		hlen = sizeof(*vlan);
		if (len < hlen)
			goto doreturn;

		vlan = hdr;
		hdr += hlen;
		len -= hlen;

		metadata->eth_proto = vlan->h_vlan_encapsulated_proto;

		index = (metadata->vlan_count < 2) ?
			metadata->vlan_count++ : 1;

		metadata->vlan[index].id =
			ntohs(vlan->h_vlan_TCI) & VLAN_VID_MASK;
		metadata->vlan[index].priority =
			(ntohs(vlan->h_vlan_TCI) &
			 VLAN_PRIO_MASK) >> VLAN_PRIO_SHIFT;
		metadata->vlan[index].tpid = ntohs(save_eth_proto);

		goto switch_ether_type;
	}

	case __cpu_to_be16(ETH_P_MPLS_UC):
	case __cpu_to_be16(ETH_P_MPLS_MC):
	{
		struct mpls_label *mpls;
		__u32 entry, label;

		hlen = sizeof(*mpls);
		if (len < hlen)
			goto doreturn;

		mpls = hdr;
		hdr += hlen;
		len -= hlen;

		entry = ntohl(mpls[0].entry);
		label = (entry & MPLS_LS_LABEL_MASK) >> MPLS_LS_LABEL_SHIFT;

		metadata->mpls.label = label;
		metadata->mpls.ttl =
			(entry & MPLS_LS_TTL_MASK) >> MPLS_LS_TTL_SHIFT;
		metadata->mpls.tc =
			(entry & MPLS_LS_TC_MASK) >> MPLS_LS_TC_SHIFT;
		metadata->mpls.bos =
			(entry & MPLS_LS_S_MASK) >> MPLS_LS_S_SHIFT;

		if (label == MPLS_LABEL_ENTROPY)
			metadata->keyid =
				mpls[1].entry & htonl(MPLS_LS_LABEL_MASK);

		ret = true;
		goto doreturn;
	}
	case __cpu_to_be16(ETH_P_ARP):
	case __cpu_to_be16(ETH_P_RARP):
	{
		struct earphdr *earp;
		struct arphdr *arp;

		hlen = sizeof(*earp);
		if (len < hlen)
			return false;

		earp = hdr;
		arp = &earp->arp;

		if (arp->ar_hrd != htons(ARPHRD_ETHER) ||
		    arp->ar_pro != htons(ETH_P_IP) ||
		    arp->ar_hln != ETH_ALEN ||
		    arp->ar_pln != 4 ||
		    (arp->ar_op != htons(ARPOP_REPLY) &&
		     arp->ar_op != htons(ARPOP_REQUEST)))
			goto doreturn;

		hdr += hlen;
		len -= hlen;

		metadata->arp.op = ntohs(earp->arp.ar_op) & 0xff;

		/* Record Ethernet addresses */
		memcpy(metadata->arp.sha, earp->ar_sha, ETH_ALEN);
		memcpy(metadata->arp.tha, earp->ar_tha, ETH_ALEN);

		/* Record IP addresses */
		memcpy(&metadata->arp.sip, &earp->ar_sip,
		       sizeof(metadata->arp.sip));
		memcpy(&metadata->arp.tip, &earp->ar_tip,
		       sizeof(metadata->arp.tip));

		ret = true;
		goto doreturn;
	}
	case __cpu_to_be16(ETH_P_TIPC):
	{
		struct tipc_basic_hdr *tipc;
		bool keepalive_msg;
		__u32 w0;

		hlen = sizeof(*tipc);
		if (len < hlen)
			goto doreturn;

		tipc = hdr;
		hdr += hlen;
		len -= hlen;

		w0 = ntohl(tipc->w[0]);
		keepalive_msg = (w0 & TIPC_KEEPALIVE_MSG_MASK) ==
			TIPC_KEEPALIVE_MSG_MASK;
		metadata->addrs.tipckey = keepalive_msg ?
			rand() : tipc->w[3];
		metadata->addr_type = PARSELITE_ATYPE_TIPC;

		ret = true;
		goto doreturn;
	}
	case __cpu_to_be16(ETH_P_BATMAN):
	{
		struct batadv_eth *beth;

		hlen = sizeof(*beth);
		if (len < hlen)
			return false;

		beth = hdr;

		if (beth->batadv_unicast.version != BATADV_COMPAT_VERSION ||
		    beth->batadv_unicast.packet_type != BATADV_UNICAST)
			goto doreturn;

		hdr += hlen;
		len -= hlen;

		metadata->eth_proto = beth->eth.h_proto;

		goto switch_ether_type;
	}
	case __cpu_to_be16(ETH_P_FCOE):
	{
		hlen = FCOE_HEADER_LEN;
		if (len < hlen)
			goto doreturn;

		hdr += hlen;
		len -= hlen;
		ret = true;

		goto doreturn;
	}
	case __cpu_to_be16(ETH_P_PPP_SES):
	{
		struct pppoe_hdr *pppoeh;
		__be16 proto;

		hlen = PPPOE_SES_HLEN;
		if (len < hlen)
			goto doreturn;

		pppoeh = hdr;
		hdr += hlen;
		len -= hlen;

		proto = *((__be16 *)&pppoeh[1]);
		switch (proto) {
		case __cpu_to_be16(PPP_IP):
			metadata->eth_proto = __cpu_to_be16(ETH_P_IP);
			goto switch_ether_type;
		case __cpu_to_be16(PPP_IPV6):
			metadata->eth_proto = __cpu_to_be16(ETH_P_IPV6);
			goto switch_ether_type;
		default:
			goto doreturn;
		}
	}
	default:
		goto doreturn;
	}

	/* Process an IP protocol */

switch_ip_proto:
	switch (ip_proto) {
	case IPPROTO_HOPOPTS:
	case IPPROTO_ROUTING:
	case IPPROTO_DSTOPTS: {
		struct ipv6_opt_hdr *opth;

		if (metadata->eth_proto != __cpu_to_be16(ETH_P_IPV6))
			return false;

		hlen = sizeof(*opth);
		if (len < hlen)
			goto doreturn;

		opth = hdr;

		hlen = ipv6_optlen(opth);
		if (len < hlen)
			goto doreturn;

		hdr += hlen;
		len -= hlen;

		ip_proto = opth->nexthdr;

		goto switch_ip_proto;
	}
	case IPPROTO_FRAGMENT: {
		struct ipv6_frag_hdr *fragh;
		__be16 offset;

		if (metadata->eth_proto != __cpu_to_be16(ETH_P_IPV6))
			return false;

		hlen = sizeof(*fragh);
		if (len < hlen)
			goto doreturn;

		fragh = hdr;
		hdr += hlen;
		len -= hlen;

		offset = (fragh->frag_off & htons(IP6_OFFSET));

		if (!(flags & PARSELITE_F_PARSE_1STFRAG) || offset)
			return true;

		ip_proto = fragh->nexthdr;

		metadata->ip_proto = fragh->nexthdr;
		metadata->is_fragment = 1;
		metadata->first_frag = !offset;

		goto switch_ip_proto;
	}

	case IPPROTO_TCP: {
		struct tcphdr *tcph;

		hlen = sizeof(*tcph);
		if (len < hlen)
			goto doreturn;

		hlen = tcp_len(hdr);
		if (len < hlen)
			goto doreturn;

		tcph = hdr;
		hdr += hlen;
		len -= hlen;

		if (!metadata->is_fragment)
			metadata->ports = ((struct port_hdr *)tcph)->ports;

		metadata->tcp.flags = tcp_flag_word(tcph) &
					__cpu_to_be16(0x0fff);

		ret = true;
		goto doreturn;
	}
	case IPPROTO_UDP: {
		struct udphdr *udph;

		hlen = sizeof(*udph);
		if (len < hlen)
			goto doreturn;

		udph = hdr;
		hdr += hlen;
		len -= hlen;

		if (!metadata->is_fragment)
			metadata->ports = ((struct port_hdr *)udph)->ports;

		ret = true;
		goto doreturn;
	}
	case IPPROTO_SCTP: {
		struct sctphdr *sctph;

		hlen = sizeof(*sctph);
		if (len < hlen)
			goto doreturn;

		sctph = hdr;
		hdr += hlen;
		len -= hlen;

		metadata->ports = ((struct port_hdr *)sctph)->ports;

		ret = true;
		goto doreturn;
	}
	case IPPROTO_DCCP: {
		struct dccp_hdr *dccph;

		hlen = sizeof(*dccph);
		if (len < hlen)
			goto doreturn;

		dccph = hdr;
		hdr += hlen;
		len -= hlen;

		metadata->ports = ((struct port_hdr *)dccph)->ports;

		ret = true;
		goto doreturn;
	}
	case IPPROTO_GRE: {
		struct gre_hdr *greh;
		ssize_t offset;

		hlen = sizeof(*greh);
		if (len < hlen)
			goto doreturn;

		greh = hdr;

		/* Only look inside GRE without routing */
		if (greh->flags & GRE_ROUTING)
			goto doreturn;

		switch (greh->flags & GRE_VERSION) {
		case 0:
			hlen = sizeof(*greh) +
					gre_v0_flags_length(greh->flags, -1U);
			if (len < hlen)
				goto doreturn;

			hdr += hlen;
			len -= hlen;

			metadata->eth_proto = greh->protocol;

			offset = gre_v0_flags_offset(greh->flags, GRE_KEY,
						     GRE_KEY_OMASK);
			if (offset >= 0)
				metadata->keyid = *(__u32 *)(hdr + offset);

			if (metadata->eth_proto == __cpu_to_be16(ETH_P_TEB)) {
				struct ethhdr *eth;

				hlen = sizeof(*eth);
				if (len < hlen)
					goto doreturn;

				hdr += hlen;
				len -= hlen;
			}
			goto switch_ether_type;
		case 1:
		{
			__u8 *ppph;

			/* Version1 must be PPTP, and check that keyid id set */

			if (!(greh->protocol == GRE_PROTO_PPP &&
					(greh->flags & GRE_KEY)))
				goto doreturn;

			hlen = sizeof(*greh) +
					gre_v1_flags_length(greh->flags, -1U);
			if (len < hlen)
				goto doreturn;

			hdr += hlen;
			len -= hlen;

			metadata->eth_proto = GRE_PROTO_PPP;

			offset = gre_v1_flags_offset(greh->flags, GRE_KEY,
						     GRE_KEY_OMASK);
			if (offset > 0)
				metadata->keyid = *(__u32 *)(hdr + offset);

			ppph = hdr;

			switch (PPP_PROTOCOL(ppph)) {
			case PPP_IP:
				metadata->eth_proto = __cpu_to_be16(ETH_P_IP);
				hdr += PPP_HDRLEN;
				len -= PPP_HDRLEN;
				goto switch_ether_type;
			case PPP_IPV6:
				metadata->eth_proto =
						__cpu_to_be16(ETH_P_IPV6);
				hdr += PPP_HDRLEN;
				len -= PPP_HDRLEN;
				goto switch_ether_type;
			default:
				goto doreturn;
			}
		}
		default:
			goto doreturn;
		}
	}
	case IPPROTO_ICMP: {
		struct icmphdr *icmph;

		hlen = sizeof(*icmph);
		if (len < hlen)
			goto doreturn;

		icmph = hdr;
		hdr += hlen;
		len -= hlen;

		metadata->icmp.type = icmph->type;
		metadata->icmp.code = icmph->code;
		if (icmp_has_id(icmph->type))
			metadata->icmp.id = icmph->un.echo.id ? : 1;
		else
			metadata->icmp.id = 0;

		ret = true;
		goto doreturn;
	}
	case IPPROTO_IGMP: {
		struct igmphdr *igmph;

		hlen = sizeof(*igmph);
		if (len < hlen)
			goto doreturn;

		hdr += hlen;
		len -= hlen;
		ret = true;
		goto doreturn;
	}
	case IPPROTO_MPLS:
		metadata->eth_proto = __cpu_to_be16(ETH_P_MPLS_UC);
		goto switch_ether_type;
	case IPPROTO_IPIP:
		metadata->eth_proto = __cpu_to_be16(ETH_P_IP);
		goto switch_ether_type;
	case IPPROTO_IPV6:
		metadata->eth_proto = __cpu_to_be16(ETH_P_IPV6);
		goto switch_ether_type;
	}

doreturn:
	return ret;
}

siphash_key_t __parselite_hash_key;

void parselite_hash_secret_init(siphash_key_t *init_key)
{
	if (init_key) {
		__parselite_hash_key = *init_key;
	} else {
		__u8 *bytes = (__u8 *)&__parselite_hash_key;
		int i;

		for (i = 0; i < sizeof(__parselite_hash_key); i++)
			bytes[i] = rand();
	}
}

void parselite_print_metadata(struct parselite_metadata *metadata)
{
	char a4buf[INET_ADDRSTRLEN];
	char a6buf[INET6_ADDRSTRLEN];

	switch (metadata->addr_type) {
	case PARSELITE_ATYPE_IPV4:
		printf("IPv4 source address: %s\n",
		inet_ntop(AF_INET, &metadata->addrs.v4_addrs[0],
			  a4buf, sizeof(a4buf)));
		printf("IPv4 destination address: %s\n",
		       inet_ntop(AF_INET, &metadata->addrs.v4_addrs[1],
		       a4buf, sizeof(a4buf)));
		break;
	case PARSELITE_ATYPE_IPV6:
		printf("IPv6 source address: %s\n",
		       inet_ntop(AF_INET6, &metadata->addrs.v6_addrs[0],
				 a6buf, sizeof(a6buf)));
		printf("IPv6 destination address: %s\n",
		       inet_ntop(AF_INET6, &metadata->addrs.v6_addrs[1],
				 a6buf, sizeof(a6buf)));
		break;
	}
	printf("Source port %04x\n", ntohs(metadata->port16[0]));
	printf("Destination port %04x\n", ntohs(metadata->port16[1]));
}

void parselite_print_hash_input(struct parselite_metadata *metadata)
{
	const void *start = &metadata->PARSELITE_HASH_START_FIELD;
	const __u8 *data = start;
	size_t len = parselite_hash_length(metadata);
	int i;

	printf("Hash input (size %lu): ", len);
	for (i = 0; i < len; i++)
		printf("%02x ", data[i]);
	printf("\n");
}
