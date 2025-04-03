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

#ifndef __XDP2_PROTO_ICMPV6_ND_H__
#define __XDP2_PROTO_ICMPV6_ND_H__

/* IPv6 neighbor discovery ICMP messages */

#include <linux/icmp.h>
#include <linux/icmpv6.h>

#include "xdp2/parser.h"

struct icmpv6_nd_opt {
	__u8 type;
	__u8 len;
	__u8 data[0];
};

struct icmpv6_nd_neigh_advert {
	struct icmp6hdr icmphdr;
	struct in6_addr target;
};

#endif /* __XDP2_PROTO_ICMPV6_ND_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

static inline ssize_t icmpv6_nd_all_len(const void *v, size_t max_len)
{
	return max_len;
}

static inline ssize_t icmpv6_nd_tlv_len(const void *hdr)
{
	return 8 * ((struct icmpv6_nd_opt *)hdr)->len;
}

static inline int icmpv6_nd_tlv_type(const void *hdr)
{
	return ((struct icmpv6_nd_opt *)hdr)->type;
}

static inline size_t icmpv6_nd_tlvs_start_offset(const void *hdr)
{
	return sizeof(struct icmpv6_nd_neigh_advert);
}

/* xdp2_parse_icmpv4 protocol definition
 *
 * Parse ICMPv4 header
 */
static const struct xdp2_proto_tlvs_def xdp2_parse_icmpv6_nd_solicit
							__unused() = {
	.proto_def.node_type = XDP2_NODE_TYPE_TLVS,
	.proto_def.name = "ICMPv6 neighbor solicit",
	.proto_def.min_len = sizeof(struct icmpv6_nd_neigh_advert),
	.proto_def.ops.len_maxlen = icmpv6_nd_all_len,
	.ops.len = icmpv6_nd_tlv_len,
	.ops.type = icmpv6_nd_tlv_type,
	.ops.start_offset = icmpv6_nd_tlvs_start_offset,
	.min_len = sizeof(struct icmpv6_nd_opt),
};

#endif /* XDP2_DEFINE_PARSE_NODE */
