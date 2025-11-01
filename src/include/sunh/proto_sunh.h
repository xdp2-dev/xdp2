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

#ifndef __PROTO_SUNH_H__
#define __PROTO_SUNH_H__

/* SUNH protocol definitions */

#include "sunh/sunh.h"
#include "xdp2/parser.h"

static inline int sunh_next_hdr(const void *vhdr)
{
	const struct sunh_hdr *sunh = vhdr;

	return  sunh->next_header;
}

#endif /* __PROTO_SUNH_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* sunh_parse protocol definition
 *
 * Parse Scale UP Network Header (SUNH)
 *
 * Next protocol operation returns next header field
 */
static const struct xdp2_proto_def sunh_parse __unused() = {
	.name = "SUNH Scale Up Network Header",
	.min_len = sizeof(struct sunh_hdr),
	.ops.next_proto = sunh_next_hdr,
};

#endif /* XDP2_DEFINE_PARSE_NODE */
