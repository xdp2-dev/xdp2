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

#ifndef __XDP2_PROTO_L2TP_H__
#define __XDP2_PROTO_L2TP_H__

/* L2Tp protocol definitions */

#ifndef IPPROTO_L2TP
#define IPPROTO_L2TP 115
#endif

static inline int l2tp_proto_version(const void *vl2tp)
{
	return ((__u8 *)vl2tp)[1] & 0xf;
}

#endif /* __XDP2_PROTO_L2TP_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* xdp2_parse_l2tp_base protocol definition
 *
 * Parse base L2TP header as an overlay to determine L2TP version
 *
 * Next protocol operation returns L2TP version number (i.e. 0, 1, or 3).
 */


/* xdp2_parse_l2tp protocol definition
 *
 * Parse  header
 */
static const struct xdp2_proto_def xdp2_parse_l2tp_base __unused() = {
	.name = "L2TP base",
	.min_len = 2,
	.ops.next_proto = l2tp_proto_version,
	.overlay = true,
	.encap = true,
};

#endif /* XDP2_DEFINE_PARSE_NODE */
