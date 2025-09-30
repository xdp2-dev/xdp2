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

#ifndef __XDP2_PROTO_GENEVE_H__
#define __XDP2_PROTO_GENEVE_H__

#include "xdp2/parser.h"

#define GENEVE_VERSION_MASK	0xc0

struct geneve_hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u8 optlen: 6;
	__u8 ver: 2;

	__u8 rsvd: 6;
	__u8 C_bit: 1;
	__u8 O_bit: 1;
#elif defined (__BIG_ENDIAN_BITFIELD)
	__u8 ver: 2;
	__u8 optlen: 6;

	__u8 O_bit: 1;
	__u8 C_bit: 1;
	__u8 rsvd: 6;
#else
#error  "Endianness not identified"
#endif
	__u16 protocol;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u32 rsvd2: 8;
	__u32 vni: 24;
#elif defined (__BIG_ENDIAN_BITFIELD)
	__u32 vni: 24;
	__u32 rsvd: 8;
#else
#error  "Endianness not identified"
#endif
} __packed;

struct geneve_opt {
	__u16 option_class;
	__u8 type;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u8 length: 5;
	__u8 rsvd: 3;
#elif defined (__BIG_ENDIAN_BITFIELD)
	__u8 rsvd: 3;
	__u8 length: 5;
#else
#error  "Endianness not identified"
#endif
} __packed;

#include "xdp2/parser.h"

static inline int geneve_base_proto(const void *vgeneve)
{
	return ((__u8 *)vgeneve)[0] & GENEVE_VERSION_MASK;
}

static inline int geneve_proto_v0(const void *vgeneve)
{
	return ((struct geneve_hdr *)vgeneve)->protocol;
}

static inline ssize_t geneve_len_v0(const void *vgeneve, size_t max_len)
{
	const struct geneve_hdr *ghdr = vgeneve;

	return sizeof(*ghdr) + (4 * ghdr->optlen);
}

static inline ssize_t geneve_tlv_len(const void *hdr, size_t max_len)
{
	const struct geneve_opt *opt_hdr = hdr;

	return sizeof(*opt_hdr) + opt_hdr->length * 4;
}

static inline int geneve_tlv_type(const void *hdr)
{
	return ((struct geneve_opt *)hdr)->option_class;
}

static inline int geneve_tlv_overlay_type(const void *hdr)
{
	return ((struct geneve_opt *)hdr)->type;
}

static inline size_t geneve_tlvs_start_offset(const void *hdr)
{
	return sizeof(struct geneve_hdr);
}

#endif /* __XDP2_PROTO_GENEVE_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* xdp2_parse_geneve protocol definition
 *
 * Parse Geneve header
 *
 * Next protocol operation returns Ethertype (e.g. ETH_P_IPV4)
 */

/* Parse version number */
static const struct xdp2_proto_def xdp2_parse_geneve_base __unused() = {
	.name = "Geneve base",
	.min_len = 1,
	.ops.next_proto = geneve_base_proto,
	.overlay = 1,
};

/* Parse version 0 */
static const struct xdp2_proto_tlvs_def xdp2_parse_geneve_v0 __unused() = {
	.proto_def.node_type = XDP2_NODE_TYPE_TLVS,
	.proto_def.name = "Geneve version 0",
	.proto_def.min_len = sizeof(struct geneve_hdr),
	.proto_def.ops.next_proto = geneve_proto_v0,
	.proto_def.ops.len = geneve_len_v0,
	.proto_def.encap = true,
	.ops.len = geneve_tlv_len,
	.ops.type = geneve_tlv_type,
	.ops.start_offset = geneve_tlvs_start_offset,
	.min_len = sizeof(struct geneve_opt),
};

/* TLV overlay. This ineeded since the type in Geneve is split into a
 * sixteen bit call and eight bit type
 */
static const struct xdp2_proto_tlv_def xdp2_parse_geneve_tlv_overlay
							__unused() = {
	.ops.overlay_type = geneve_tlv_overlay_type,
};

#endif /* XDP2_DEFINE_PARSE_NODE */
