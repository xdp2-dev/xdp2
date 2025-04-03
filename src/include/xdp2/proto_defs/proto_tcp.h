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

#ifndef __XDP2_PROTO_TCP_H__
#define __XDP2_PROTO_TCP_H__

#include <linux/tcp.h>

#include "xdp2/parser.h"

/* TCP protocol definitions */

#define TCPOPT_NOP		1	/* Padding */
#define TCPOPT_EOL		0	/* End of options */
#define TCPOPT_MSS		2	/* Segment size negotiating */
#define TCPOPT_WINDOW		3	/* Window scaling */
#define TCPOPT_SACK_PERM	4	/* SACK Permitted */
#define TCPOPT_SACK		5	/* SACK Block */
#define TCPOPT_TIMESTAMP	8	/* Better RTT estimations/PAWS */
#define TCPOPT_MD5SIG		19	/* MD5 Signature (RFC2385) */
#define TCPOPT_FASTOPEN		34	/* Fast open (RFC7413) */
#define TCPOPT_EXP		254	/* Experimental */

struct tcp_opt {
	__u8 type;
	__u8 len;
	__u8 data[0];
};

struct tcp_timestamp_option_data {
	__be32 value;
	__be32 echo;
};

struct tcp_sack_option_data {
	__be32 left_edge;
	__be32 right_edge;
};

#define TCP_MAX_SACKS	4

struct tcp_opt_union {
	struct tcp_opt opt;
	union {
		__be16 mss;
		__u8 window_scaling;
		struct tcp_timestamp_option_data timestamp;
		struct tcp_sack_option_data sack[TCP_MAX_SACKS];
	} __packed;
} __packed;

static inline ssize_t tcp_len(const void *vtcp)
{
	return ((struct tcphdr *)vtcp)->doff * 4;
}

static inline ssize_t tcp_tlv_len(const void *hdr)
{
	return ((struct tcp_opt *)hdr)->len;
}

static inline int tcp_tlv_type(const void *hdr)
{
	return ((struct tcp_opt *)hdr)->type;
}

static inline size_t tcp_tlvs_start_offset(const void *hdr)
{
	return sizeof(struct tcphdr);
}

#endif /* __XDP2_PROTO_TCP_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* XDP2 protocol definition for TCP
 *
 * There are two variants:
 *   - Parse TCP header and TLVs
 *   - Just parse header without parsing TLVs
 */

/* xdp2_parse_tcp_tlvs protocol definition
 *
 * Parse TCP header and any TLVs
 */
static const struct xdp2_proto_tlvs_def xdp2_parse_tcp_tlvs __unused() = {
	.proto_def.node_type = XDP2_NODE_TYPE_TLVS,
	.proto_def.name = "TCP with TLVs",
	.proto_def.min_len = sizeof(struct tcphdr),
	.proto_def.ops.len = tcp_len,
	.ops.len = tcp_tlv_len,
	.ops.type = tcp_tlv_type,
	.ops.start_offset = tcp_tlvs_start_offset,
	.pad1_val = TCPOPT_NOP,
	.pad1_enable = 1,
	.eol_val = TCPOPT_EOL,
	.eol_enable = 1,
	.min_len = sizeof(struct tcp_opt),
};

/* xdp2_parse_tcp_no_tlvs protocol definition
 *
 * Parse TCP header without considering TLVs
 */
static const struct xdp2_proto_def xdp2_parse_tcp_notlvs __unused() = {
	.name = "TCP without TLVs",
	.min_len = sizeof(struct tcphdr),
	.ops.len = tcp_len,
};

/* Protocol definitions for individual TLVs */

static const struct xdp2_proto_tlv_def xdp2_parse_tcp_option_unknown
							__unused() = {
	.min_len = sizeof(struct tcp_opt),
};

static const struct xdp2_proto_tlv_def xdp2_parse_tcp_option_mss
							__unused() = {
	.min_len = sizeof(struct tcp_opt) + sizeof(__be16),
};

static const struct xdp2_proto_tlv_def xdp2_parse_tcp_option_window_scaling
							__unused() = {
	.min_len = sizeof(struct tcp_opt) + sizeof(__u8),
};

static const struct xdp2_proto_tlv_def xdp2_parse_tcp_option_timestamp
							__unused() = {
	.min_len = sizeof(struct tcp_opt) +
				sizeof(struct tcp_timestamp_option_data),
};

static const struct xdp2_proto_tlv_def xdp2_parse_tcp_option_sack_permitted
							__unused() = {
	.min_len = sizeof(struct tcp_opt),
};

static const struct xdp2_proto_tlv_def xdp2_parse_tcp_option_sack
							__unused() = {
	.min_len = sizeof(struct tcp_opt),
};

static const struct xdp2_proto_tlv_def xdp2_parse_tcp_option_sack_1
							__unused() = {
	.min_len = sizeof(struct tcp_opt) +
				sizeof(struct tcp_sack_option_data),
};

static const struct xdp2_proto_tlv_def xdp2_parse_tcp_option_sack_2
							__unused() = {
	.min_len = sizeof(struct tcp_opt) +
				2 * sizeof(struct tcp_sack_option_data),
};

static const struct xdp2_proto_tlv_def xdp2_parse_tcp_option_sack_3
							__unused() = {
	.min_len = sizeof(struct tcp_opt) +
				3 * sizeof(struct tcp_sack_option_data),
};

static const struct xdp2_proto_tlv_def xdp2_parse_tcp_option_sack_4
							__unused() = {
	.min_len = sizeof(struct tcp_opt) +
				4 * sizeof(struct tcp_sack_option_data),
};

#endif /* XDP2_DEFINE_PARSE_NODE */
