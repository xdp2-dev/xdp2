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

#ifndef __PARSELITE_PARSER_H__
#define __PARSELITE_PARSER_H__

#include <linux/if_ether.h>
#include <linux/ipv6.h>
#include <linux/types.h>
#include <string.h>
#include <unistd.h>

#include "xdp2/utility.h"
#include "siphash/siphash.h"

struct parselite_metadata {
	__u8 addr_type;
	__u8 is_fragment: 1;
	__u8 first_frag: 1;
	__u8 vlan_count: 2;
	__u8 tos;
	__u8 ttl;
	__u8 eth_addrs[2 * ETH_ALEN];

	struct {
		__be16	flags;
	} tcp;

	struct {
		__u32   ttl: 8;
		__u32   bos: 1;
		__u32   tc: 3;
		__u32   label: 20;
	} mpls;

	struct {
		__u32 sip;
		__u32 tip;
		__u8 op;
		__u8 sha[ETH_ALEN];
		__u8 tha[ETH_ALEN];
	} arp;

#define PARSELITE_HASH_START_FIELD eth_proto
	__be16  eth_proto __aligned(8);
	__u8	ip_proto;

	__u32	flow_label;

	struct {
		union {
			struct {
				__u16	id:12,
					dei:1,
					priority:3;
			};
			__be16  vlan_tci;
		};
		__be16  tpid;
	} vlan[2];

	__be32	keyid;

	union {
		__be32 ports;
		__be16 port16[2];
		struct {
			__be16 src_port;
			__be16 dst_port;
		};
	};

	struct {
		__u8	type;
		__u8	code;
		__u16	id;
	} icmp;

	/* Addrs must be last for hashing */
	union {
		__be32		v4_addrs[2];
		struct in6_addr	v6_addrs[2];
		__be32		tipckey;
	} addrs;

	/* Force size to be multiple of 8 bytes to maintain alignment in
	 * an array of structures.
	 */
	__u8	align[0] __aligned(8);
};

enum {
	PARSELITE_ATYPE_IPV4,
	PARSELITE_ATYPE_IPV6,
	PARSELITE_ATYPE_TIPC,
};

#define PARSELITE_F_PARSE_1STFRAG		0x1
#define PARSELITE_F_STOP_FLOWLABEL		0x2

#define PARSELITE_HASH_OFFSET					\
	offsetof(struct parselite_metadata,			\
		 PARSELITE_HASH_START_FIELD)

enum {
	PARSELITE_START_ETHER,
	PARSELITE_START_ETHTYPE,
	PARSELITE_START_IP,
};

bool parselite_parse(void *hdr, size_t len,
		     struct parselite_metadata *metadata,
		     unsigned int flags, unsigned int max_encaps,
		     unsigned int start_mode_mode);

void parselite_hash_secret_init(siphash_key_t *init_key);
void parselite_print_metadata(struct parselite_metadata *metadata);
void parselite_print_hash_input(struct parselite_metadata *metadata);

#define PARSELITE_ENCAP_DEPTH	4

/* Utility functions for various ways to parse packets and compute packet
 * hashes using the parsers for big parser
 */

/* Parse packet starting with Ethernet header */
static inline bool parselite_parse_ether(void *p, size_t len,
					 struct parselite_metadata *metadata)
{
	return (parselite_parse(p, len, metadata, PARSELITE_F_STOP_FLOWLABEL,
				PARSELITE_ENCAP_DEPTH, PARSELITE_START_ETHER));
}

/* Parse packet starting with a known layer 3 protocol. Determine start
 * node by performing a protocol look up on the root node of the Ethernet
 * parser (i.e. get the start node by looking up the Ethertype in the
 * Ethernet protocol table)
 */
static inline bool parselite_parse_l3(void *p, size_t len, __be16 proto,
				struct parselite_metadata *metadata)
{
	metadata->eth_proto = proto;

	return (parselite_parse(p, len, metadata, PARSELITE_F_STOP_FLOWLABEL,
				PARSELITE_ENCAP_DEPTH,
				PARSELITE_START_ETHTYPE));
}

/* Parse packet starting with IP header. Root node distinguished based
 * on IP version number
 */
static inline bool parselite_parse_ip(void *p, size_t len,
				struct parselite_metadata *metadata)
{
	return (parselite_parse(p, len, metadata, PARSELITE_F_STOP_FLOWLABEL,
				PARSELITE_ENCAP_DEPTH, PARSELITE_START_IP));
}

#define SWAP(a, b)						\
	do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)


static inline size_t parselite_hash_length(
			struct parselite_metadata *metadata)
{
	size_t diff = PARSELITE_HASH_OFFSET + sizeof(metadata->addrs);

	switch (metadata->addr_type) {
	case PARSELITE_ATYPE_IPV4:
		diff -= sizeof(metadata->addrs.v4_addrs);
		break;
	case PARSELITE_ATYPE_IPV6:
		diff -= sizeof(metadata->addrs.v6_addrs);
		break;
	}

	return sizeof(*metadata) - diff;
}

extern siphash_key_t __parselite_hash_key;

static inline __u32 parselite_compute_hash(const void *start, size_t len)
{
	__u32 hash;

	hash = siphash(start, len, &__parselite_hash_key);
	if (!hash)
		hash = 1;

	return hash;
}

/* Produce canonical hash from metadata contents */
static inline __u32 parselite_hash_metadata(
				struct parselite_metadata *metadata)
{
	const void *start = &metadata->PARSELITE_HASH_START_FIELD;
	size_t len = parselite_hash_length(metadata);
	int addr_diff, i;

	switch (metadata->addr_type) {
	case PARSELITE_ATYPE_IPV4:
		addr_diff = metadata->addrs.v4_addrs[1] -
					metadata->addrs.v4_addrs[0];
		if ((addr_diff < 0) ||
		    (addr_diff == 0 && (metadata->port16[1] <
					metadata->port16[0]))) {
			SWAP(metadata->addrs.v4_addrs[0],
			     metadata->addrs.v4_addrs[1]);
			SWAP(metadata->port16[0],
			     metadata->port16[1]);
		}
		break;
	case PARSELITE_ATYPE_IPV6:
		addr_diff = memcmp(&metadata->addrs.v6_addrs[1],
				   &metadata->addrs.v6_addrs[0],
				   sizeof(metadata->addrs.v6_addrs[1]));
		if ((addr_diff < 0) ||
		    (addr_diff == 0 && (metadata->port16[1] <
					metadata->port16[0]))) {
			for (i = 0; i < 4; i++)
				SWAP(metadata->addrs.v6_addrs[0].s6_addr32[i],
				     metadata->addrs.v6_addrs[1].s6_addr32[i]);
			SWAP(metadata->port16[0],
			     metadata->port16[1]);
		}
		break;
	}

	return parselite_compute_hash(start, len);
}

/* Return hash for packet starting with Ethernet header */
static inline __u32 parselite_hash_ether(void *p, size_t len)
{
	struct parselite_metadata metadata;

	memset(&metadata, 0, sizeof(metadata));

	if (parselite_parse_ether(p, len, &metadata))
		return parselite_hash_metadata(&metadata);

	return 0;
}

/* Return hash for a packet starting with the indicated layer 3 protols
 * (i.e. an EtherType)
 */
static inline __u32 parselite_hash_l3(void *p, size_t len, __be16 proto)
{
	struct parselite_metadata metadata;

	memset(&metadata, 0, sizeof(metadata));

	if (parselite_parse_l3(p, len, proto, &metadata))
		return parselite_hash_metadata(&metadata);

	return 0;
}

/* Return hash for packet starting with in IP header (IPv4 or
 * IPv6 distinguished by inspecting IP version number
 */
static inline __u32 parselite_hash_ip(void *p, size_t len)
{
	struct parselite_metadata metadata;

	memset(&metadata, 0, sizeof(metadata));

	if (parselite_parse_ip(p, len, &metadata))
		return parselite_hash_metadata(&metadata);

	return 0;
}
#endif /* __PARSELITE_PARSER_H__ */
