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

#ifndef __XDP2_PROTO_IPV4IP_H__
#define __XDP2_PROTO_IPV4IP_H__

/* IPv4 in IP protocol definitions */

#include <linux/ip.h>

#include "xdp2/parser.h"

static inline int ipv4_proto_default(const void *viph)
{
	return 0; /* Indicates IPv4 */
}

#endif /* __XDP2_PROTO_IPV4IP_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* parse_ipv4ip protocol definition
 *
 * Parses IPv4IP header
 *
 * Next protocol operation returns 0 indicating IPv4
 */
static const struct xdp2_proto_def xdp2_parse_ipv4ip __unused() = {
	.name = "IPv4 in IP",
	.encap = 1,
	.overlay = 1,
	.min_len = sizeof(struct iphdr),
	.ops.next_proto = ipv4_proto_default,
};

#endif /* XDP2_DEFINE_PARSE_NODE */
