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

#ifndef __XDP2_PROTO_ICMP_H__
#define __XDP2_PROTO_ICMP_H__

/* Generic ICMP protocol definitions */

#include <linux/icmp.h>
#include <linux/icmpv6.h>

#include "xdp2/parser.h"

static inline bool icmp_has_id(__u8 type)
{
	switch (type) {
	case ICMP_ECHO:
	case ICMP_ECHOREPLY:
	case ICMP_TIMESTAMP:
	case ICMP_TIMESTAMPREPLY:
	case ICMPV6_ECHO_REQUEST:
	case ICMPV6_ECHO_REPLY:
		return true;
	}

	return false;
}

#endif /* __XDP2_PROTO_ICMP_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

static inline int icmp_get_type(const void *vicmp)
{
	return ((struct icmphdr *)vicmp)->type;
}

static inline ssize_t icmp_all_len(const void *v, size_t max_len)
{
	return max_len;
}

/* xdp2_parse_icmpv4 protocol definition
 *
 * Parse ICMPv4 header
 */
static const struct xdp2_proto_def xdp2_parse_icmpv4 __unused() = {
	.name = "ICMPv4",
	.min_len = sizeof(struct icmphdr),
	.overlay = true,
	.ops.next_proto = icmp_get_type,
	.ops.len_maxlen = icmp_all_len,
};

/* xdp2_parse_icmpv6 protocol definition
 *
 * Parse ICMPv6 header
 */
static const struct xdp2_proto_def xdp2_parse_icmpv6 __unused() = {
	.name = "ICMPv6",
	.min_len = sizeof(struct icmp6hdr),
	.overlay = true,
	.ops.next_proto = icmp_get_type,
	.ops.len_maxlen = icmp_all_len,
};

#endif /* XDP2_DEFINE_PARSE_NODE */
