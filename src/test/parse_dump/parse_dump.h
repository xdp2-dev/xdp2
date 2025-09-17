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

#ifndef __XDP2_TEST_PARSE_DUMP_H__
#define __XDP2_TEST_PARSE_DUMP_H__

#include <stdbool.h>

#include "xdp2/utility.h"
#include "xdp2/parser.h"

#define MAX_VLAN_CNT 2

extern int verbose;
extern bool use_colors;

struct metametadata {
	union {
		struct {
			struct {
				__u32 value;
				__u32 echo;
			} timestamp;
			__u8 num_sacks;
			struct {
				__u32 left_edge;
				__u32 right_edge;
			} sack[4];
			__u16 mss;

			bool have_window_scaling;
			__u8 window_scaling;

		} tcp_options;

		struct {
			__u8 type;
			__u8 code;
			__u32 data;
			bool is_ipv6;
		} icmp;

		struct {
			__u16 op;
			__u8 sha[ETH_ALEN];
			__u32 sip;
			__u8 tha[ETH_ALEN];
			__u32 tip;
		} arp;
	};

	bool arp_present;
	bool tcp_present;
	bool icmp_present;

	unsigned long timestamp;
	unsigned char num_encaps;
	__u16 flowid;

	__u32 hash;
};

/* Metadata structure with addresses and port numbers */
struct metadata {
	struct {
		union {
			__u32 val;
			struct {
				__u16 callid;
				__u16 payload_len;
			};
		} key_id;
		__u32 sequence;
		__u32 ack;
	} gre;

	struct {
		__u16 id;
		__u8 dei;
		__u8 priority;
		__be16 tci;
		__be16 tpid;
		__be16 enc_proto;
	} vlan[MAX_VLAN_CNT];
	__u8 vlan_cnt;

	__u8 gre_version;

	__u16 ipv4_hdr_csum;
	__u16 pseudo_checksum;

	__u8 ip_ttl;
	__u8 ip_hlen;
	__u16 ip_off;
	__u16 ip_frag_off;
	bool is_ipv6_frag;
	__u32 ip_frag_id;

#define HASH_START_FIELD addr_type

	__u8 addr_type __aligned(8);
	__u8 ip_proto;

	__u16 ether_type;

	union {
		struct {
			__be16 sport;
			__be16 dport;
		};
		__u32 v;
	} port_pair;

	union {
		union {
			__be32 v4_addrs[2];
			struct {
				__be32 saddr;
				__be32 daddr;
			} v4;
		};
		union {
			struct in6_addr v6_addrs[2];
			struct {
				struct in6_addr saddr;
				struct in6_addr daddr;
			} v6;
		};
	} addrs;
} __packed;

#define METADATA_FRAME_COUNT 3

struct pmetadata {
	struct metametadata metametadata;
	struct metadata metadata[METADATA_FRAME_COUNT];
};

struct one_packet {
	__u8 *packet;
	size_t hdr_size;
	size_t cap_len;
	char *pcap_file;
	unsigned int file_number;
};

void print_metadata(void *_metadata, const struct xdp2_ctrl_data *ctrl);

#define PRINTFC(SEQNO, ...) do {				\
	if (use_colors)						\
		XDP2_PRINT_COLOR_SEL(SEQNO, __VA_ARGS__);	\
	else							\
		XDP2_CLI_PRINT(NULL, __VA_ARGS__);		\
} while (0)

void lookup_tuple(struct metadata *frame, int seqno);

void init_tables(void);

#endif /* __XDP2_TEST_PARSE_DUMP_H__ */
