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

#ifndef __PROTO_IPV4_H__
#define __PROTO_IPV4_H__

/* IPv4 protocol definitions */

#ifndef __KERNEL__
#include <arpa/inet.h>
#endif

#include <linux/ip.h>

#include "xdp2/parser.h"

#define IP_MF		0x2000	/* Flag: "More Fragments"   */
#define IP_OFFSET	0x1FFF	/* "Fragment Offset" part   */

static inline size_t ipv4_len(const void *viph)
{
	return ((struct iphdr *)viph)->ihl * 4;
}

static inline bool ip_is_fragment(const struct iphdr *iph)
{
	return (iph->frag_off & htons(IP_MF | IP_OFFSET)) != 0;
}

static inline int ipv4_proto(const void *viph)
{
	const struct iphdr *iph = viph;

	if (ip_is_fragment(iph) && (iph->frag_off & htons(IP_OFFSET))) {
		/* Stop at a non-first fragment */
		return XDP2_STOP_OKAY;
	}

	return iph->protocol;
}

static inline int ipv4_proto_stop1stfrag(const void *viph)
{
	const struct iphdr *iph = viph;

	if (ip_is_fragment(iph)) {
		/* Stop at all fragments */
		return XDP2_STOP_OKAY;
	}

	return iph->protocol;
}

static inline ssize_t ipv4_length(const void *viph)
{
	return ipv4_len(viph);
}

static inline ssize_t ipv4_length_check(const void *viph)
{
	const struct iphdr *iph = viph;

	if (iph->version != 4)
		return XDP2_STOP_UNKNOWN_PROTO;

	return ipv4_len(viph);
}

#endif /* __PROTO_IPV4_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* xdp2_parse_ipv4 protocol definition
 *
 * Parse IPv4 header
 *
 * Next protocol operation returns IP proto number (e.g. IPPROTO_TCP)
 */
static const struct xdp2_proto_def xdp2_parse_ipv4 __unused() = {
	.name = "IPv4",
	.min_len = sizeof(struct iphdr),
	.ops.len = ipv4_length,
	.ops.next_proto = ipv4_proto,
};

/* xdp2_parse_ipv4_stop1stfrag protocol definition
 *
 * Parse IPv4 header but don't parse into first fragment
 *
 * Next protocol operation returns IP proto number (e.g. IPPROTO_TCP)
 */
static const struct xdp2_proto_def xdp2_parse_ipv4_stop1stfrag __unused() = {
	.name = "IPv4 without parsing 1st fragment",
	.min_len = sizeof(struct iphdr),
	.ops.len = ipv4_length,
	.ops.next_proto = ipv4_proto_stop1stfrag,
};

/* xdp2_parse_ipv4_check protocol definition
 *
 * Check version is four and parse IPv4 header
 *
 * Next protocol operation returns IP proto number (e.g. IPPROTO_TCP)
 */
static const struct xdp2_proto_def xdp2_parse_ipv4_check __unused() = {
	.name = "IPv4-check",
	.min_len = sizeof(struct iphdr),
	.ops.len = ipv4_length_check,
	.ops.next_proto = ipv4_proto,
	.overlay = 1,
};

/* xdp2_parse_ipv4_stop1stfrag_check protocol definition
 *
 * Check IP version is four an parse IPv4 header but don't parse into first
 * fragment
 *
 * Next protocol operation returns IP proto number (e.g. IPPROTO_TCP)
 */
static const struct xdp2_proto_def xdp2_parse_ipv4_stop1stfrag_check
							__unused() = {
	.name = "IPv4 without parsing 1st fragment",
	.min_len = sizeof(struct iphdr),
	.ops.len = ipv4_length,
	.ops.next_proto = ipv4_proto_stop1stfrag,
};

#endif /* XDP2_DEFINE_PARSE_NODE */
