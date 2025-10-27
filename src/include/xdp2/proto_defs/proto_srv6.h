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

#ifndef __XDP2_PROTO_SRV6_H__
#define __XDP2_PROTO_SRV6_H__

#include <linux/seg6.h>

/* SRv6 protocol definitions */

static inline int ipv6_srv6_proto(const void *vopt)
{
	return ((struct ipv6_opt_hdr *)vopt)->nexthdr;
}

static inline ssize_t ipv6_srv6_len(const void *vopt, size_t maxlen)
{
	return ipv6_optlen((struct ipv6_opt_hdr *)vopt);
}

static inline unsigned int ipv6_srv6_num_els(const void *hdr, size_t hlen)
{
	const struct ipv6_sr_hdr *srhdr = hdr;

	return srhdr->first_segment + 1;
}

static inline int ipv6_srv6_el_type(const void *hdr)
{
	return 0;
}

static inline size_t ipv6_srv6_seg_list_start_offset(const void *hdr)
{
	return sizeof(struct ipv6_sr_hdr);
}

#endif /* __XDP2_PROTO_SRV6_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* xdp2_parse_udp protocol definition
 *
 * Parse UDP header
 */
static const struct xdp2_proto_def xdp2_parse_srv6 __unused() = {
	.name = "SRV6",
	.min_len = sizeof(struct ipv6_opt_hdr),
	.ops.next_proto = ipv6_srv6_proto,
	.ops.len = ipv6_srv6_len,
};

static const struct xdp2_proto_array_def xdp2_parse_srv6_seg_list __unused() = {
	.proto_def.name = "SRV6 with seg list",
	.proto_def.min_len = sizeof(struct ipv6_opt_hdr),
	.proto_def.ops.next_proto = ipv6_srv6_proto,
	.proto_def.ops.len = ipv6_srv6_len,
	.proto_def.node_type = XDP2_NODE_TYPE_ARRAY,
	.ops.el_type = ipv6_srv6_el_type,
	.ops.num_els = ipv6_srv6_num_els,
	.ops.start_offset = ipv6_srv6_seg_list_start_offset,
};

#endif /* XDP2_DEFINE_PARSE_NODE */
