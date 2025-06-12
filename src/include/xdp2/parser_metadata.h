/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
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

#ifndef __XDP2_PARSER_METADATA_H__
#define __XDP2_PARSER_METADATA_H__

/* Helper definitions for XDP2 parser metadata handling
 *
 * This defines a set of macros, constants, and functions that can be
 * optionally used in constructing parse nodes and to assist in meta
 * data handling as well as packet hashing.
 */

#ifndef __KERNEL__
#include <string.h>
#endif

#include <linux/if_ether.h>
#include <linux/mpls.h>

#include "xdp2/parser.h"
#include "xdp2/proto_defs.h"

/* The XDP2 helpers defines a common set of fields that may be used in
 * parser specific metadata structures. This is done at the granularity of
 * field names. When the common names and their types are used in meta
 * data structure then helper marcos can be used to create functions
 * that take the parser specific data structure as an argument but
 * operate on the common fields. In this way we can essentially have
 * the same functions operate on different input structures, in particular
 * we can define per protocol macros that extract common fields into
 * different metadata structures. The type of the structure is an argument
 * to the macro, and then from that a function definition can be ommited that
 * uses the type. Here is an example to extract common metadata for IPv4
 * into a user defined metadata structure.
 *
 * #define XDP2_METADATA_ipv4_addrs(NAME, STRUCT)			\
 * static void NAME(const void *viph, void *iframe)			\
 * {									\
 *	struct STRUCT *frame = iframe;					\
 *	const struct iphdr *iph = viph;					\
 *									\
 *       frame->addr_type = XDP2_ADDR_TYPE_IPV4;			\
 *       frame->ip_proto = iph->protocol;				\
 *       memcpy(frame->addrs.v4_addrs, &iph->saddr,			\
 *              sizeof(frame->addrs.v4_addrs));				\
 * }
 *
 * In this example the common metadata field names used are addr_type,
 * addrs.v4, and ip_proto.
 *
 * #defines for metadata names and their types are below. Note the macros
 * can be used to define the common metadata fields in a data structure,
 * however this is not required. As long as the types and names are
 * maintained differnt definitions may be used. This is particulary relevant
 * when common names are in data structures and the user may wish to add
 * other elements in the structure
 */

/* Common metadata names and macro definitions. Add new common meta
 * data names to this list
 */

#define XDP2_METADATA_eth_proto	__be16	eth_proto
#define XDP2_METADATA_eth_addrs	__u8 eth_addrs[2 * ETH_ALEN]

enum xdp2_addr_types {
	XDP2_ADDR_TYPE_INVALID = 0, /* Invalid addr type */
	XDP2_ADDR_TYPE_IPV4,
	XDP2_ADDR_TYPE_IPV6,
	XDP2_ADDR_TYPE_TIPC,
};

#define	XDP2_METADATA_addr_type	__u8 addr_type
#define XDP2_METADATA_addrs						\
	union {								\
		union {							\
			__be32		v4_addrs[2];			\
			struct {					\
				__be32	saddr;				\
				__be32	daddr;				\
			} v4;						\
		};							\
		union {							\
			struct in6_addr v6_addrs[2];			\
			struct {					\
				struct in6_addr saddr;			\
				struct in6_addr daddr;			\
			} v6;						\
		};							\
		__be32		tipckey;				\
	} addrs

#define	XDP2_METADATA_ip_proto	__u8 ip_proto
#define	XDP2_METADATA_is_fragment	__u8 is_fragment: 1
#define	XDP2_METADATA_first_frag	__u8 first_frag: 1

#define XDP2_METADATA_flow_label	__u32 flow_label

#define XDP2_METADATA_l2_off		__u16 l2_off
#define XDP2_METADATA_l3_off		__u16 l3_off
#define XDP2_METADATA_l4_off		__u16 l4_off

#define XDP2_METADATA_ports						\
	union {								\
		__be32 ports;						\
		__be16 port16[2];					\
		struct {						\
			__be16 src_port;				\
			__be16 dst_port;				\
		};							\
		struct {						\
			__be16 sport;					\
			__be16 dport;					\
		} port_pair;						\
	}

#define XDP2_METADATA_tcp_options					\
	struct {							\
		__u16 mss;						\
		__u8 window_scaling;					\
		struct {						\
			__u32 value;					\
			__u32 echo;					\
		} timestamp;						\
		struct {						\
			__u32 left_edge;				\
			__u32 right_edge;				\
		} sack[TCP_MAX_SACKS];					\
	} tcp_options

#define XDP2_METADATA_keyid		__be32  keyid

#define XDP2_MAX_VLAN_CNT	2
#define XDP2_METADATA_vlan_count	__u8 vlan_count : 2
#define XDP2_METADATA_vlan						\
	struct {							\
		union {							\
			struct {					\
				__u16   id:12,				\
					dei:1,				\
					priority:3;			\
			};						\
			__be16  tci;					\
		};							\
		__be16  tpid;						\
	} vlan[XDP2_MAX_VLAN_CNT]

#define XDP2_METADATA_icmp						\
	struct {							\
		__u8	type;						\
		__u8	code;						\
		__u16	id;						\
	} icmp

#define XDP2_METADATA_mpls						\
	struct {							\
		__u32	ttl: 8;						\
		__u32	bos: 1;						\
		__u32	tc: 3;						\
		__u32	label: 20;					\
	} mpls

#define XDP2_METADATA_arp						\
	struct {							\
		__u32	sip;						\
		__u32	tip;						\
		__u8	op;						\
		__u8	sha[ETH_ALEN];					\
		__u8	tha[ETH_ALEN];					\
	} arp

#define XDP2_METADATA_gre						\
	struct {							\
		__u32 flags;						\
		__be16 csum;						\
		__be32 keyid;						\
		__be32 seq;						\
		__be32 routing;						\
	} gre

#define XDP2_METADATA_gre_pptp						\
	struct {							\
		__u32 flags;						\
		__be16 length;						\
		__be16 callid;						\
		__be32 seq;						\
		__be32 ack;						\
	} gre_pptp

/* Meta data structure containing all common metadata in canonical field
 * order. eth_proto is declared as the hash start field for the common
 * metadata structure. addrs is last field for canonical hashing.
 */
struct xdp2_metadata_all {
	XDP2_METADATA_addr_type;
	XDP2_METADATA_is_fragment;
	XDP2_METADATA_first_frag;
	XDP2_METADATA_vlan_count;
	XDP2_METADATA_eth_addrs;
	XDP2_METADATA_tcp_options;
	XDP2_METADATA_mpls;
	XDP2_METADATA_arp;
	XDP2_METADATA_gre;
	XDP2_METADATA_gre_pptp;
	XDP2_METADATA_l2_off;
	XDP2_METADATA_l3_off;
	XDP2_METADATA_l4_off;


#define XDP2_HASH_START_FIELD_ALL eth_proto
	XDP2_METADATA_eth_proto __aligned(8);
	XDP2_METADATA_ip_proto;
	XDP2_METADATA_flow_label;
	XDP2_METADATA_vlan;
	XDP2_METADATA_keyid;
	XDP2_METADATA_ports;
	XDP2_METADATA_icmp;

	XDP2_METADATA_addrs; /* Must be last */
};

#define XDP2_HASH_OFFSET_ALL						\
	offsetof(struct xdp2_metadata_all,				\
		 XDP2_HASH_START_FIELD_ALL)

/* Template for hash consistentify. Sort the source and destination IP (and the
 * ports if the IP address are the same) to have consistent hash within the two
 * directions.
 */
#define XDP2_HASH_CONSISTENTIFY(FRAME) do {				\
	int addr_diff, i;						\
									\
	switch ((FRAME)->addr_type) {					\
	case XDP2_ADDR_TYPE_IPV4:					\
		addr_diff = (FRAME)->addrs.v4_addrs[1] -		\
					(FRAME)->addrs.v4_addrs[0];	\
		if ((addr_diff < 0) ||					\
		    (addr_diff == 0 && ((FRAME)->port16[1] <		\
					(FRAME)->port16[0]))) {		\
			XDP2_SWAP((FRAME)->addrs.v4_addrs[0],		\
				   (FRAME)->addrs.v4_addrs[1]);		\
			XDP2_SWAP((FRAME)->port16[0],			\
				   (FRAME)->port16[1]);			\
		}							\
		break;							\
	case XDP2_ADDR_TYPE_IPV6:					\
		addr_diff = memcmp(&(FRAME)->addrs.v6_addrs[1],		\
				   &(FRAME)->addrs.v6_addrs[0],		\
				   sizeof((FRAME)->addrs.v6_addrs[1]));	\
		if ((addr_diff < 0) ||					\
		    (addr_diff == 0 && ((FRAME)->port16[1] <		\
					(FRAME)->port16[0]))) {		\
			for (i = 0; i < 4; i++)				\
				XDP2_SWAP((FRAME)->addrs.v6_addrs[0].	\
							s6_addr32[i],	\
				     (FRAME)->addrs.v6_addrs[1].	\
							s6_addr32[i]);	\
			XDP2_SWAP((FRAME)->port16[0],			\
				   (FRAME)->port16[1]);			\
		}							\
		break;							\
	}								\
} while (0)

/* Helper to get starting address for hash start. This is just the
 * address of the field name in HASH_START_FIELD of a metadata
 * structure instance (indicated by pointer in FRAME)
 */
#define XDP2_HASH_START(FRAME, HASH_START_FIELD)			\
	(&(FRAME)->HASH_START_FIELD)

/* Helper that returns the hash length for a metadata structure. This
 * returns the end of the address fields for the given type (the
 * address fields are assumed to be the common metadata fields in a nion
 * in the last fields in the metadata structure). The macro returns the
 * offset of the last byte of address minus the offset of the field
 * where the hash starts as indicated by the HASH_OFFSET argument.
 */
#define XDP2_HASH_LENGTH(FRAME, HASH_OFFSET) ({				\
	size_t diff = HASH_OFFSET + sizeof((FRAME)->addrs);		\
									\
	switch ((FRAME)->addr_type) {					\
	case XDP2_ADDR_TYPE_IPV4:					\
		diff -= sizeof((FRAME)->addrs.v4_addrs);		\
		break;							\
	case XDP2_ADDR_TYPE_IPV6:					\
		diff -= sizeof((FRAME)->addrs.v6_addrs);		\
		break;							\
	}								\
	sizeof(*(FRAME)) - diff;					\
})

/* Helpers to extract common metadata */

/* Meta data helper for Ethernet.
 * Uses common metadata fields: eth_proto, eth_addrs
 */
#define XDP2_METADATA_TEMP_ether(NAME, STRUCT)				\
static void NAME(const void *veth, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	frame->eth_proto = ((struct ethhdr *)veth)->h_proto;		\
	memcpy(frame->eth_addrs, &((struct ethhdr *)veth)->h_dest,	\
	       sizeof(frame->eth_addrs));				\
}

/* Meta data helper for Ethernet with setting L2 offset.
 * Uses common metadata fields: eth_proto, eth_addrs, l2_off
 */
#define XDP2_METADATA_TEMP_ether_off(NAME, STRUCT)			\
static void NAME(const void *veth, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	frame->l2_off = ctrl.hdr.hdr_offset;				\
	frame->eth_proto = ((struct ethhdr *)veth)->h_proto;		\
	memcpy(frame->eth_addrs, &((struct ethhdr *)veth)->h_dest,	\
	       sizeof(frame->eth_addrs));				\
}

/* Meta data helper for Ethernet without extracting addresses.
 * Uses common metadata fields: eth_proto
 */
#define XDP2_METADATA_TEMP_ether_noaddrs(NAME, STRUCT)			\
static void NAME(const void *veth, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	frame->eth_proto = ((struct ethhdr *)veth)->h_proto;		\
}

/* Meta data helper for IPv4.
 * Uses common metadata fields: is_fragment, first_frag, ip_proto,
 * addr_type, addrs.v4_addrs, l3_off
 */
#define XDP2_METADATA_TEMP_ipv4(NAME, STRUCT)				\
static void NAME(const void *viph, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
	const struct iphdr *iph = viph;					\
									\
	if (ip_is_fragment(iph)) {					\
		frame->is_fragment = 1;					\
		frame->first_frag =					\
				!(iph->frag_off & htons(IP_OFFSET));	\
	}								\
									\
	frame->l3_off = ctrl.hdr.hdr_offset;				\
	frame->addr_type = XDP2_ADDR_TYPE_IPV4;				\
	frame->ip_proto = iph->protocol;				\
	memcpy(frame->addrs.v4_addrs, &iph->saddr,			\
	       sizeof(frame->addrs.v4_addrs));				\
}

/* Meta data helper for IPv4 to only extract IP address.
 * Uses common meta * data fields: ip_proto, addr_type, addrs.v4_addrs
 */
#define XDP2_METADATA_TEMP_ipv4_addrs(NAME, STRUCT)			\
static void NAME(const void *viph, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
	const struct iphdr *iph = viph;					\
									\
	frame->addr_type = XDP2_ADDR_TYPE_IPV4;				\
	frame->ip_proto = iph->protocol;				\
	memcpy(frame->addrs.v4_addrs, &iph->saddr,			\
	       sizeof(frame->addrs.v4_addrs));				\
}

/* Meta data helper for IPv6.
 * Uses common metadata fields: ip_proto, addr_type, flow_label,
 * addrs.v6_addrs, l3_off
 */
#define XDP2_METADATA_TEMP_ipv6(NAME, STRUCT)				\
static void NAME(const void *viph, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
	const struct ipv6hdr *iph = viph;				\
									\
	frame->l3_off = ctrl.hdr.hdr_offset;				\
	frame->ip_proto = iph->nexthdr;					\
	frame->addr_type = XDP2_ADDR_TYPE_IPV6;				\
	frame->flow_label = ntohl(ip6_flowlabel(iph));			\
	memcpy(frame->addrs.v6_addrs, &iph->saddr,			\
	       sizeof(frame->addrs.v6_addrs));				\
}

/* Meta data helper for IPv6 to only extract IP address.
 * Uses common metadata fields: ip_proto, addr_type, addrs.v6_addrs
 */
#define XDP2_METADATA_TEMP_ipv6_addrs(NAME, STRUCT)			\
static void NAME(const void *viph, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
	const struct ipv6hdr *iph = viph;				\
									\
	frame->ip_proto = iph->nexthdr;					\
	frame->addr_type = XDP2_ADDR_TYPE_IPV6;				\
	memcpy(frame->addrs.v6_addrs, &iph->saddr,			\
	       sizeof(frame->addrs.v6_addrs));				\
}

/* Meta data helper for transport ports.
 * Uses common metadata fields: ports
 */
#define XDP2_METADATA_TEMP_ports(NAME, STRUCT)				\
static void NAME(const void *vphdr, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	frame->ports = ((struct port_hdr *)vphdr)->ports;		\
}

/* Meta data helper for transport with ports and offset
 * Uses common metadata fields: ports, l4_off
 */
#define XDP2_METADATA_TEMP_ports_off(NAME, STRUCT)			\
static void NAME(const void *vphdr, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	frame->ports = ((struct port_hdr *)vphdr)->ports;		\
	frame->l4_off = ctrl.hdr.hdr_offset;				\
}

/* Meta data helpers for TCP options */

/* Meta data helper for TCP MSS option
 * Uses common metadata field: tcp_options
 */
#define XDP2_METADATA_TEMP_tcp_option_mss(NAME, STRUCT)			\
static void NAME(const void *vopt, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	const struct tcp_opt_union *opt = vopt;				\
	struct STRUCT *frame = iframe;					\
									\
	frame->tcp_options.mss = ntohs(opt->mss);			\
}

/* Meta data helper for TCP window scaling option
 * Uses common metadata field: tcp_options
 */
#define XDP2_METADATA_TEMP_tcp_option_window_scaling(NAME, STRUCT)	\
static void NAME(const void *vopt, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	const struct tcp_opt_union *opt = vopt;				\
	struct STRUCT *frame = iframe;					\
									\
	frame->tcp_options.window_scaling = opt->window_scaling;	\
}

/* Meta data helper for TCP timestamps option
 * Uses common metadata field: tcp_options
 */
#define XDP2_METADATA_TEMP_tcp_option_timestamp(NAME, STRUCT)		\
static void NAME(const void *vopt, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	const struct tcp_opt_union *opt = vopt;				\
	struct STRUCT *frame = iframe;					\
									\
	frame->tcp_options.timestamp.value =				\
				ntohl(opt->timestamp.value);		\
	frame->tcp_options.timestamp.echo =				\
				ntohl(opt->timestamp.echo);		\
}

/* Common macro to set one metadata entry for sack. N indicates which
 * entry (per protocol specification that is 0, 1, 2, or 3)
 */
#define XDP2_METADATA_SET_TCP_SACK(N, VOPT, IFRAME, STRUCT) do {	\
	const struct tcp_opt_union *opt = vopt;				\
	struct STRUCT *frame = iframe;					\
									\
	frame->tcp_options.sack[N].left_edge =				\
				ntohl(opt->sack[N].left_edge);		\
	frame->tcp_options.sack[N].right_edge =				\
				ntohl(opt->sack[N].right_edge);		\
} while (0)

/* Meta data helper for setting one TCP sack option
 * Uses common metadata field: tcp_options.sack[0]
 */
#define XDP2_METADATA_TEMP_tcp_option_sack_1(NAME, STRUCT)		\
static void NAME(const void *vopt, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	XDP2_METADATA_SET_TCP_SACK(0, vopt, iframe, STRUCT);		\
}

/* Meta data helper for setting two TCP sack options
 * Uses common metadata field: tcp_options.sack[0], tcp_options.sack[1]
 */
#define XDP2_METADATA_TEMP_tcp_option_sack_2(NAME, STRUCT)		\
static void NAME(const void *vopt, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	XDP2_METADATA_SET_TCP_SACK(0, vopt, iframe, STRUCT);		\
	XDP2_METADATA_SET_TCP_SACK(1, vopt, iframe, STRUCT);		\
}

/* Meta data helper for setting three TCP sack options
 * Uses common metadata field: tcp_options.sack[0], tcp_options.sack[1],
 * tcp_options.sack[2]
 */
#define XDP2_METADATA_TEMP_tcp_option_sack_3(NAME, STRUCT)		\
static void NAME(const void *vopt, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	XDP2_METADATA_SET_TCP_SACK(0, vopt, iframe, STRUCT);		\
	XDP2_METADATA_SET_TCP_SACK(1, vopt, iframe, STRUCT);		\
	XDP2_METADATA_SET_TCP_SACK(2, vopt, iframe, STRUCT);		\
}

/* Meta data helper for setting four TCP sack options
 * Uses common metadata field: tcp_options.sack[0], tcp_options.sack[1],
 * tcp_options.sack[2], tcp_options.sack[3]
 */
#define XDP2_METADATA_TEMP_tcp_option_sack_4(NAME, STRUCT)		\
static void NAME(const void *vopt, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	XDP2_METADATA_SET_TCP_SACK(0, vopt, iframe, STRUCT);		\
	XDP2_METADATA_SET_TCP_SACK(1, vopt, iframe, STRUCT);		\
	XDP2_METADATA_SET_TCP_SACK(2, vopt, iframe, STRUCT);		\
	XDP2_METADATA_SET_TCP_SACK(3, vopt, iframe, STRUCT);		\
}

/* Meta data helper for IP overlay (differentiate based on version number).
 * Uses common metadata fields: eth_proto
 */
#define XDP2_METADATA_TEMP_ip_overlay(NAME, STRUCT)			\
static void NAME(const void *viph, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	switch (((struct ip_hdr_byte *)viph)->version) {		\
	case 4:								\
		frame->eth_proto = __cpu_to_be16(ETH_P_IP);		\
		break;							\
	case 6:								\
		frame->eth_proto = __cpu_to_be16(ETH_P_IPV6);		\
		break;							\
	}								\
}

/* Meta data helper for Routing, DestOpt, and Hop-by-Hop extension headers.
 * Uses common metadata fields: ip_proto
 */
#define XDP2_METADATA_TEMP_ipv6_eh(NAME, STRUCT)			\
static void NAME(const void *vopt, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	((struct STRUCT *)iframe)->ip_proto =				\
			((struct ipv6_opt_hdr *)vopt)->nexthdr;		\
}

/* Meta data helper for Fragmentation extension header.
 * Uses common metadata fields: ip_proto, is_fragment, first_frag
 */
#define XDP2_METADATA_TEMP_ipv6_frag(NAME, STRUCT)			\
static void NAME(const void *vfrag, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
	const struct ipv6_frag_hdr *frag = vfrag;			\
									\
	frame->ip_proto = frag->nexthdr;				\
	frame->is_fragment = 1;						\
	frame->first_frag = !(frag->frag_off & htons(IP6_OFFSET));	\
}

/* Meta data helper for Fragmentation extension header without info.
 * Uses common metadata fields: ip_proto
 */
#define XDP2_METADATA_TEMP_ipv6_frag_noinfo(NAME, STRUCT)		\
static void NAME(const void *vfrag, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	((struct STRUCT *)iframe)->ip_proto =				\
			((struct ipv6_frag_hdr *)vfrag)->nexthdr;	\
}

#define XDP2_METADATA_TEMP_arp_rarp(NAME, STRUCT)			\
static void NAME(const void *vearp, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
	const struct earphdr *earp = vearp;				\
									\
	frame->arp.op = ntohs(earp->arp.ar_op) & 0xff;			\
									\
	/* Record Ethernet addresses */					\
	memcpy(frame->arp.sha, earp->ar_sha, ETH_ALEN);			\
	memcpy(frame->arp.tha, earp->ar_tha, ETH_ALEN);			\
									\
	/* Record IP addresses */					\
	memcpy(&frame->arp.sip, &earp->ar_sip, sizeof(frame->arp.sip));	\
	memcpy(&frame->arp.tip, &earp->ar_tip, sizeof(frame->arp.tip));	\
}

/* Meta data helper for VLAN.
 * Uses common metadata fields: vlan_count, vlan[0].id, vlan[0].priority,
 * vlan[0].tci, vlan[0].tpid, vlan[1].id, vlan[1].priority, vlan[1].tci,
 * vlan[1].tpid
 */
#define XDP2_METADATA_TEMP_vlan_set_tpid(NAME, STRUCT, TPID)		\
static void NAME(const void *vvlan, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
	const struct vlan_hdr *vlan = vvlan;				\
	int index = (frame->vlan_count < XDP2_MAX_VLAN_CNT) ?		\
			frame->vlan_count++ : XDP2_MAX_VLAN_CNT - 1;	\
									\
	frame->vlan[index].id = ntohs(vlan->h_vlan_TCI) &		\
				VLAN_VID_MASK;				\
	frame->vlan[index].priority = (ntohs(vlan->h_vlan_TCI) &	\
				VLAN_PRIO_MASK) >> VLAN_PRIO_SHIFT;	\
	frame->vlan[index].tpid = TPID;					\
}

#define XDP2_METADATA_TEMP_vlan_8021AD(NAME, STRUCT)			\
	XDP2_METADATA_TEMP_vlan_set_tpid(NAME, STRUCT, ETH_P_8021AD)

#define XDP2_METADATA_TEMP_vlan_8021Q(NAME, STRUCT)			\
	XDP2_METADATA_TEMP_vlan_set_tpid(NAME, STRUCT, ETH_P_8021Q)

/* Meta data helper for ICMP (ICMPv4 or ICMPv6).
 * Uses common metadata fields: icmp.type, icmp.code, icmp.id
 */
#define XDP2_METADATA_TEMP_icmp(NAME, STRUCT)				\
static void NAME(const void *vicmp, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
	const struct icmphdr *icmp = vicmp;				\
									\
	frame->icmp.type = icmp->type;					\
	frame->icmp.code = icmp->code;					\
	if (icmp_has_id(icmp->type))					\
		frame->icmp.id = icmp->un.echo.id ? : 1;		\
	else								\
		frame->icmp.id = 0;					\
}

/* Meta data helper for MPLS.
 * Uses common metadata fields: mpls.label, mpls.ttl, mpls.tc, mpls.bos, keyid
 */
#define XDP2_METADATA_TEMP_mpls(NAME, STRUCT)				\
static void NAME(const void *vmpls, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
	const struct mpls_label *mpls = vmpls;				\
	__u32 entry, label;						\
									\
	entry = ntohl(mpls[0].entry);					\
	label = (entry & MPLS_LS_LABEL_MASK) >> MPLS_LS_LABEL_SHIFT;	\
									\
	frame->mpls.label = label;					\
	frame->mpls.ttl =						\
		(entry & MPLS_LS_TTL_MASK) >> MPLS_LS_TTL_SHIFT;	\
	frame->mpls.tc = (entry & MPLS_LS_TC_MASK) >> MPLS_LS_TC_SHIFT;	\
	frame->mpls.bos = (entry & MPLS_LS_S_MASK) >> MPLS_LS_S_SHIFT;	\
									\
	if (label == MPLS_LABEL_ENTROPY)				\
		frame->keyid =						\
			mpls[1].entry & htonl(MPLS_LS_LABEL_MASK);	\
}

/* Meta data helper for tipc.
 * Uses common metadata fields: addr_type, tipckwy
 *
 * For non keepalive message set source node identity in tipc addresses.
 * For keepalive messages set the tipc address to a random number fo
 * spread PROBE/PROBE_REPLY messages across cores.
 */
#define XDP2_METADATA_TEMP_tipc(NAME, STRUCT)				\
static void NAME(const void *vtipc, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
	const struct tipc_basic_hdr *tipc = vtipc;			\
									\
	__u32 w0 = ntohl(tipc->w[0]);					\
	bool keepalive_msg;						\
									\
	keepalive_msg = (w0 & TIPC_KEEPALIVE_MSG_MASK) ==		\
					TIPC_KEEPALIVE_MSG_MASK;	\
	frame->addrs.tipckey = keepalive_msg ? 0 : tipc->w[3];		\
	frame->addr_type = XDP2_ADDR_TYPE_TIPC;			\
}

/* Meta data helper for GRE (v0)
 * Uses common metadata field: gre.flags
 */
#define XDP2_METADATA_TEMP_gre(NAME, STRUCT)				\
static void NAME(const void *vhdr, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	frame->gre.flags = gre_get_flags(vhdr);				\
}

/* Meta data helper for GRE-PPTP (GRE v1)
 * Uses common metadata field: gre_pptp.flags
 */
#define XDP2_METADATA_TEMP_gre_pptp(NAME, STRUCT)			\
static void NAME(const void *vhdr, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	frame->gre_pptp.flags = gre_get_flags(vhdr);			\
}

/* Meta data helper for GRE checksum
 * Uses common metadata field: gre.checksum
 */
#define XDP2_METADATA_TEMP_gre_checksum(NAME, STRUCT)			\
static void NAME(const void *vdata, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	frame->gre.csum = *(__u16 *)vdata;				\
}

/* Meta data helper for GRE keyid
 * Uses common metadata field: gre.keyid and keyid
 */
#define XDP2_METADATA_TEMP_gre_keyid(NAME, STRUCT)			\
static void NAME(const void *vdata, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
	__u32 v = *(__u32 *)vdata;					\
									\
	frame->gre.keyid = v;						\
	frame->keyid = v;						\
}

/* Meta data helper for GRE sequence number
 * Uses common metadata field: gre.seq
 */
#define XDP2_METADATA_TEMP_gre_seq(NAME, STRUCT)			\
static void NAME(const void *vdata, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	frame->gre.seq = *(__u32 *)vdata;				\
}

/* Meta data helper for GRE routing
 * Uses common metadata field: gre.routing
 */
#define XDP2_METADATA_TEMP_gre_routing(NAME, STRUCT)			\
static void NAME(const void *vdata, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	frame->gre.routing = *(__u32 *)vdata;				\
}


/* Meta data helper for GRE keyid
 * Uses common metadata field: pptp.length, pptp.call_id, and keyid
 */
#define XDP2_METADATA_TEMP_gre_pptp_key(NAME, STRUCT)			\
static void NAME(const void *vdata, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
	struct xdp2_pptp_id *key = (struct xdp2_pptp_id *)vdata;	\
									\
	frame->keyid = key->val32;					\
	frame->gre_pptp.length = key->payload_len;			\
	frame->gre_pptp.callid = key->call_id;				\
}

/* Meta data helper for GRE-pptp sequence number
 * Uses common metadata field: pptp.seq
 */
#define XDP2_METADATA_TEMP_gre_pptp_seq(NAME, STRUCT)			\
static void NAME(const void *vdata, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	frame->gre_pptp.seq = *(__u32 *)vdata;				\
}

/* Meta data helper for GRE-pptp ACK
 * Uses common metadata field: pptp.ack
 */
#define XDP2_METADATA_TEMP_gre_pptp_ack(NAME, STRUCT)			\
static void NAME(const void *vdata, void *iframe,			\
		 struct xdp2_ctrl_data ctrl)				\
{									\
	struct STRUCT *frame = iframe;					\
									\
	frame->gre_pptp.ack = *(__u32 *)vdata;				\
}

/* Helper function to define a function to print common metadata */
#define XDP2_PRINT_METADATA(FRAME) do {				\
	char a4buf[INET_ADDRSTRLEN];					\
	char a6buf[INET6_ADDRSTRLEN];					\
									\
	switch ((FRAME)->addr_type) {					\
	case XDP2_ADDR_TYPE_IPV4:					\
		printf("IPv4 source address: %s\n",			\
		inet_ntop(AF_INET, &(FRAME)->addrs.v4_addrs[0],		\
			  a4buf, sizeof(a4buf)));			\
		printf("IPv4 destination address: %s\n",		\
		       inet_ntop(AF_INET, &(FRAME)->addrs.v4_addrs[1],	\
		       a4buf, sizeof(a4buf)));				\
		break;							\
	case XDP2_ADDR_TYPE_IPV6:					\
		printf("IPv6 source address: %s\n",			\
		       inet_ntop(AF_INET6, &(FRAME)->addrs.v6_addrs[0],	\
				 a6buf, sizeof(a6buf)));		\
		printf("IPv6 destination address: %s\n",		\
		       inet_ntop(AF_INET6, &(FRAME)->addrs.v6_addrs[1],	\
				 a6buf, sizeof(a6buf)));		\
		break;							\
	}								\
	printf("Source port %04x\n", ntohs((FRAME)->port16[0]));	\
	printf("Destination port %04x\n", ntohs((FRAME)->port16[1]));	\
} while (0)

#endif /* __XDP2_PARSER_METADATA_H__ */
