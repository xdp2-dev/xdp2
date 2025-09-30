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

#ifndef __XDP2_PROTO_GRE_H__
#define __XDP2_PROTO_GRE_H__

/* GRE protocol definitions */

#ifndef __KERNEL__
#include <arpa/inet.h>
#endif

#include <linux/ip.h>

#include "xdp2/parser.h"

/* Define common GRE constants. These normally come from linux/if_tunnel.h,
 * however that include file has a lot of other definitions beyond just GRE
 * which can be a problem compiling especially with older kernel includes. So
 * we define the GRE values here if they aren't already defined elsewhere.
 */
#ifndef GRE_CSUM
#define GRE_CSUM	__cpu_to_be16(0x8000)
#endif

#ifndef GRE_ROUTING
#define GRE_ROUTING	__cpu_to_be16(0x4000)
#endif

#ifndef GRE_KEY
#define GRE_KEY		__cpu_to_be16(0x2000)
#endif

#ifndef GRE_SEQ
#define GRE_SEQ		__cpu_to_be16(0x1000)
#endif

#ifndef GRE_ACK
#define GRE_ACK		__cpu_to_be16(0x0080)
#endif

#ifndef GRE_VERSION
#define GRE_VERSION	__cpu_to_be16(0x0007)
#endif

#ifndef GRE_VERSION_0
#define GRE_VERSION_0	__cpu_to_be16(0x0000)
#endif

#ifndef GRE_VERSION_1
#define GRE_VERSION_1	__cpu_to_be16(0x0001)
#endif

#ifndef GRE_PROTO_PPP
#define GRE_PROTO_PPP	__cpu_to_be16(0x880b)
#endif

#ifndef GRE_PPTP_KEY_MASK
#define GRE_PPTP_KEY_MASK	__cpu_to_be32(0xffff)
#endif

/* GRE flag-field definitions */
static const struct xdp2_flag_fields gre_flag_fields = {
	.fields = {
		{
#define GRE_FLAGS_CSUM_IDX	0
			.flag = GRE_CSUM,
			.size = sizeof(__be32),
		},
		{
#define GRE_FLAGS_KEY_IDX	1
			.flag = GRE_KEY,
			.size = sizeof(__be32),
		},
#define GRE_FLAGS_SEQ_IDX	2
		{
			.flag = GRE_SEQ,
			.size = sizeof(__be32),
		},
#define GRE_FLAGS_NUM_IDX	3
	},
	.num_idx = GRE_FLAGS_NUM_IDX
};

#define GRE_FLAGS_V0_MASK	(GRE_CSUM | GRE_KEY | GRE_SEQ | GRE_ROUTING)

static const struct xdp2_flag_fields pptp_gre_flag_fields = {
	.fields = {
		{
#define GRE_PPTP_FLAGS_CSUM_IDX	0
			.flag = GRE_CSUM,
			.size = sizeof(__be32),
		},
		{
#define GRE_PPTP_FLAGS_KEY_IDX	1
			.flag = GRE_KEY,
			.size = sizeof(__be32),
		},
#define GRE_PPTP_FLAGS_SEQ_IDX	2
		{
			.flag = GRE_SEQ,
			.size = sizeof(__be32),
		},
#define GRE_PPTP_FLAGS_ACK_IDX	3
		{
			.flag = GRE_ACK,
			.size = sizeof(__be32),
		},
#define GRE_PPTP_FLAGS_NUM_IDX	4
	},
	.num_idx = GRE_PPTP_FLAGS_NUM_IDX
};

#define GRE_FLAGS_V1_MASK	(GRE_CSUM | GRE_KEY | GRE_SEQ |		\
				 GRE_ROUTING | GRE_ACK)

#define GRE_PPTP_KEY_MASK	__cpu_to_be32(0xffff)

struct gre_hdr {
	__be16 flags;
	__be16 protocol;
	__u8 fields[0];
};

struct xdp2_pptp_id {
	union {
		struct {
			__u16 payload_len;
			__u16 call_id;
		};
		__u32 val32;
	};
};

static inline int gre_proto_version(const void *vgre)
{
	return ntohs(((struct gre_hdr *)vgre)->flags & GRE_VERSION);
}

static inline size_t gre_v0_len_from_flags(unsigned int flags)
{
	return sizeof(struct gre_hdr) +
		xdp2_flag_fields_length(flags, &gre_flag_fields);
}

static inline ssize_t gre_v0_len_check(const void *vgre, size_t maxlen)
{
	const struct gre_hdr *gre = vgre;

	/* Check for valid set of flags */
	if (xdp2_flag_fields_check_invalid(gre->flags, GRE_FLAGS_V0_MASK |
								GRE_VERSION))
		return XDP2_STOP_BAD_FLAG;

	/* Only look inside GRE without routing */
	if (((struct gre_hdr *)vgre)->flags & GRE_ROUTING)
		return XDP2_STOP_OKAY;

	return gre_v0_len_from_flags(gre->flags);
}

static inline int gre_v0_proto(const void *vgre)
{
	return ((struct gre_hdr *)vgre)->protocol;
}

static inline size_t gre_v1_len_from_flags(unsigned int flags)
{
	return sizeof(struct gre_hdr) +
		xdp2_flag_fields_length(flags, &pptp_gre_flag_fields);
}

static inline ssize_t gre_v1_len_check(const void *vgre, size_t maxlen)
{
	const struct gre_hdr *gre = vgre;

	/* Check for valid set of flags */
	if (xdp2_flag_fields_check_invalid(gre->flags, GRE_FLAGS_V1_MASK |
								GRE_VERSION))
		return XDP2_STOP_BAD_FLAG;

	/* Only look inside GRE without routing */
	if (((struct gre_hdr *)vgre)->flags & GRE_ROUTING)
		return XDP2_STOP_OKAY;

	/* Version1 must be PPTP, and check that keyid id set */
	if (!(gre->protocol == GRE_PROTO_PPP && (gre->flags & GRE_KEY)))
		return XDP2_STOP_OKAY;

	return gre_v1_len_from_flags(gre->flags);
}

static inline int gre_v1_proto(const void *vgre)
{
	return ((struct gre_hdr *)vgre)->protocol;
}

static inline __u32 gre_get_flags(const void *hdr)
{
	return ((struct gre_hdr *)hdr)->flags;
}

static inline size_t gre_fields_offset(const void *hdr)
{
	return sizeof(struct gre_hdr);
}

#endif /* __XDP2_PROTO_GRE_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* xdp2_parse_gre_base protocol definition
 *
 * Parse base GRE header as an overlay to determine GRE version
 *
 * Next protocol operation returns GRE version number (i.e. 0 or 1).
 */
static const struct xdp2_proto_def xdp2_parse_gre_base __unused() = {
	.name = "GRE base",
	.overlay = 1,
	.min_len = sizeof(struct gre_hdr),
	.ops.next_proto = gre_proto_version,
};

/* xdp2_parse_gre_v0 protocol definition
 *
 * Parse a version 0 GRE header
 *
 * Next protocol operation returns a GRE protocol (e.g. ETH_P_IPV4).
 */
static const struct xdp2_proto_flag_fields_def
					xdp2_parse_gre_v0 __unused() = {
	.proto_def.node_type = XDP2_NODE_TYPE_FLAG_FIELDS,
	.proto_def.name = "GRE v0",
	.proto_def.encap = 1,
	.proto_def.min_len = sizeof(struct gre_hdr),
	.proto_def.ops.next_proto = gre_v0_proto,
	.proto_def.ops.len = gre_v0_len_check,
	.ops.get_flags = gre_get_flags,
	.ops.start_fields_offset = gre_fields_offset,
	.flag_fields = &gre_flag_fields,
};

/* xdp2_parse_gre_v1 protocol node
 *
 * Parse a version 1 GRE header
 *
 * Next protocol operation returns GRE_PROTO_PPP.
 */
static const struct xdp2_proto_flag_fields_def
					xdp2_parse_gre_v1 __unused() = {
	.proto_def.node_type = XDP2_NODE_TYPE_FLAG_FIELDS,
	.proto_def.name = "GRE v1 - pptp",
	.proto_def.encap = 1,
	.proto_def.min_len = sizeof(struct gre_hdr),
	.proto_def.ops.next_proto = gre_v1_proto,
	.proto_def.ops.len = gre_v1_len_check,
	.ops.get_flags = gre_get_flags,
	.ops.start_fields_offset = gre_fields_offset,
	.flag_fields = &pptp_gre_flag_fields,
};

#endif /* XDP2_DEFINE_PARSE_NODE */
