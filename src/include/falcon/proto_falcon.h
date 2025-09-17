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

#ifndef __PROTO_FALCON_H__
#define __PROTO_FALCON_H__

/* Falcon protocol definitions */

#include "falcon/falcon.h"
#include "xdp2/parser.h"

/* Get Falcon version number */
static inline int falcon_version_proto(const void *vhdr)
{
	return ((struct falcon_version_switch_header *)vhdr)->version;
}

/* Get Falcon protocol type */
static inline int falcon_packet_type_switch_proto(const void *vhdr)
{
	return ((struct falcon_packet_type_switch_header *)vhdr)->packet_type;
}

#endif /* __PROTO_FALCON_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* falcon_parse_version protocol definition
 *
 * Parse Falcon version number
 *
 * Next protocol operation returns version number
 */
static const struct xdp2_proto_def falcon_parse_version __unused() = {
	.name = "Falcon version header",
	.min_len = sizeof(struct falcon_version_switch_header),
	.ops.next_proto = falcon_version_proto,
	.overlay = 1,
};

/* falcon_parse_packet_type protocol definition
 *
 * Parse Falcon packet type
 *
 * Next protocol operation returns packet type
 */
static const struct xdp2_proto_def falcon_parse_packet_type __unused() = {
	.name = "Falcon packet type header",
	.min_len = sizeof(struct falcon_packet_type_switch_header),
	.ops.next_proto = falcon_packet_type_switch_proto,
	.overlay = 1,
};

/* falcon_parse_* protocol definitions
 *
 * Parse the various Falcon packet types. These are leaf nodes and all
 * headers are fixed size
 *
 * Types are { pull_request, pull_data, push_data, pull_data, resync,
 *	       base_ack, nack }
 */

static const struct xdp2_proto_def falcon_parse_pull_request __unused() = {
	.name = "Falcon pull request",
	.min_len = sizeof(struct falcon_pull_req_pkt),
};

static const struct xdp2_proto_def falcon_parse_pull_data __unused() = {
	.name = "Falcon pull data",
	.min_len = sizeof(struct falcon_pull_data_pkt),
};

static const struct xdp2_proto_def falcon_parse_push_data __unused() = {
	.name = "Falcon push data",
	.min_len = sizeof(struct falcon_push_data_pkt),
};

static const struct xdp2_proto_def falcon_parse_resync __unused() = {
	.name = "Falcon resync",
	.min_len = sizeof(struct falcon_resync_pkt),
};

static const struct xdp2_proto_def falcon_parse_back __unused() = {
	.name = "Falcon base ack",
	.min_len = sizeof(struct falcon_base_ack_pkt),
};

static const struct xdp2_proto_def falcon_parse_nack __unused() = {
	.name = "Falcon nack",
	.min_len = sizeof(struct falcon_nack_pkt),
};

static const struct xdp2_proto_def falcon_parse_eack __unused() = {
	.name = "Falcon eack",
	.min_len = sizeof(struct falcon_ext_ack_pkt),
};

#endif /* XDP2_DEFINE_PARSE_NODE */
