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

#ifndef __XDP2_PROTO_PPPOE_H__
#define __XDP2_PROTO_PPPOE_H__

#include "xdp2/parser.h"

struct pppoe_hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u8 type : 4;
	__u8 ver : 4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	__u8 ver : 4;
	__u8 type : 4;
#else
#error	"Please fix <asm/byteorder.h>"
#endif
	__u8 code;
	__be16 sid;
	__be16 length;
	__be16 protocol;
} __packed;

//int static_assert_global_v[sizeof(struct pppoe_hdr) == 6 ? -1 : 1];

/* PPP protocol definitions */
static inline int pppoe_proto(const void *vppp)
{
	return ((struct pppoe_hdr *)vppp)->protocol;
}

#endif /* __XDP2_PROTO_PPP_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* xdp2_parse_ppp protocol definition
 *
 * Parse PPP header
 *
 * Next protocol operation returns IP proto number (e.g. IPPROTO_TCP)
 */
static const struct xdp2_proto_def xdp2_parse_pppoe __unused() = {
	.name = "PPPoE",
	.min_len = sizeof(struct pppoe_hdr),
	.ops.next_proto = pppoe_proto,
};

#endif /* XDP2_DEFINE_PARSE_NODE */
