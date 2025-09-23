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

#ifndef __PROTO_SUE_H__
#define __PROTO_SUE_H__

/* Falcon protocol definitions */

#include "falcon/falcon.h"
#include "xdp2/parser.h"

/* Get Falcon version number */
static inline int sue_get_version(const void *vhdr)
{
	return ((struct sue_version_switch_header *)vhdr)->ver;
}

/* Get Falcon protocol type */
static inline int sue_get_opcode(const void *vhdr)
{
	return ((struct sue_reliability_hdr *)vhdr)->op;
}

#endif /* __PROTO_SUE_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* sue_parse_version protocol definition
 *
 * Parse SUE version number
 *
 * Next protocol operation returns version number
 */
static const struct xdp2_proto_def sue_parse_version_ov __unused() = {
	.name = "SUE version header",
	.min_len = sizeof(struct sue_version_switch_header),
	.ops.next_proto = sue_get_version,
	.overlay = 1,
};

/* sue_parse_opcode protocol definition (overlay)
 *
 * Parse SUE opcode
 *
 * Next protocol operation returns opcode
 */
static const struct xdp2_proto_def sue_parse_opcode_ov __unused() = {
	.name = "SUE opcode header",
	.min_len = sizeof(struct sue_reliability_hdr),
	.ops.next_proto = sue_get_opcode,
	.overlay = 1,
};

/* sue_parse_opcode protocol definition
 *
 * Parse SUE opcode
 *
 * Next protocol operation returns opcode
 */
static const struct xdp2_proto_def sue_parse_rh __unused() = {
	.name = "SUE reliability header",
	.min_len = sizeof(struct sue_reliability_hdr),
};

#endif /* XDP2_DEFINE_PARSE_NODE */
