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

#ifndef __XDP2_PROTO_IPV6_EH_H__
#define __XDP2_PROTO_IPV6_EH_H__

/* Generic definitions for IPv6 extension headers */

#ifndef __KERNEL__
#include <arpa/inet.h>
#endif

#include <linux/ipv6.h>

#include "xdp2/parser.h"

struct ipv6_frag_hdr {
	__u8    nexthdr;
	__u8    reserved;
	__be16  frag_off;
	__be32  identification;
};

#define IP6_MF		0x0001
#define IP6_OFFSET	0xFFF8

static inline int ipv6_eh_proto(const void *vopt)
{
	return ((struct ipv6_opt_hdr *)vopt)->nexthdr;
}

static inline ssize_t ipv6_eh_len(const void *vopt, size_t maxlen)
{
	return ipv6_optlen((struct ipv6_opt_hdr *)vopt);
}

static inline int ipv6_frag_proto(const void *vfraghdr)
{
	const struct ipv6_frag_hdr *fraghdr = vfraghdr;

	if (fraghdr->frag_off & htons(IP6_OFFSET)) {
		/* Stop at non-first fragment */
		return XDP2_STOP_OKAY;
	}

	return fraghdr->nexthdr;
}

static inline int ipv6_routing_header_proto(const void *vrthdr)
{
	return ((struct ipv6_rt_hdr *)vrthdr)->type;
};

#endif /* __XDP2_PROTO_IPV6_EH_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

static const struct xdp2_proto_def xdp2_parse_ipv6_eh __unused() = {
	.name = "IPv6 EH",
	.min_len = sizeof(struct ipv6_opt_hdr),
	.ops.next_proto = ipv6_eh_proto,
	.ops.len = ipv6_eh_len,
};

static const struct xdp2_proto_def xdp2_parse_ipv6_routing_hdr __unused() = {
	.name = "IPv6 RH overlay",
	.min_len = sizeof(struct ipv6_rt_hdr),
	.ops.next_proto = ipv6_routing_header_proto,
	.ops.len = ipv6_eh_len,
	.overlay = 1,
};

/* xdp2_parse_ipv6_eh protocol definition
 *
 * Parse IPv6 extension header (Destinaion Options, Hop-by-Hop Options,
 * or Routing Header
 *
 * Next protocol operation returns IP proto number (e.g. IPPROTO_TCP)
 */
static const struct xdp2_proto_def xdp2_parse_ipv6_frag_eh __unused() = {
	.name = "IPv6 EH",
	.min_len = sizeof(struct ipv6_frag_hdr),
	.ops.next_proto = ipv6_frag_proto,
};

/* xdp2_parse_ipv6_frag_eh protocol definition
 *
 * Parse IPv6 fragmentation header, stop parsing at first fragment
 *
 * Next protocol operation returns IP proto number (e.g. IPPROTO_TCP)
 */
static const struct xdp2_proto_def xdp2_parse_ipv6_frag_eh_stop1stfrag
							__unused() = {
	.name = "IPv6 EH",
	.min_len = sizeof(struct ipv6_frag_hdr),
};

#endif /* XDP2_DEFINE_PARSE_NODE */
