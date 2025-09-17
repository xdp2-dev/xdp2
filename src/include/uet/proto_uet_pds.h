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

#ifndef __PROTO_UET_PDS_H__
#define __PROTO_UET_PDS_H__

/* UET Packet Delivery Sub-system (PDS) protocol definitions */

#include "uet/pds.h"
#include "xdp2/parser.h"

/* Get PDS protocol type */
static inline int uet_pds_packet_type_switch_proto(const void *vhdr)
{
	return ((struct uet_pds_prologue *)vhdr)->v &
						UET_PDS_PROLOGUE_TYPE_MASK;
}

/* Get PDS next header type */
static inline int uet_pds_next_proto(const void *vhdr)
{
	return uet_pds_common_get_next_hdr(vhdr);
}

/* Get next ACK CC node */
static inline int uet_pds_next_ack_cc(const void *vhdr)
{
	return ((struct uet_pds_ack_cc *)vhdr)->cc_type;
}

#endif /* __PROTO_UET_PDS_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* uet_pds_parse_packet_type protocol definition
 *
 * Parse UET-PDS packet type
 *
 * Next protocol operation returns packet type
 */
static const struct xdp2_proto_def uet_pds_parse_packet_type_ov __unused() = {
	.name = "UET-PDS packet type header",
	.min_len = sizeof(struct uet_pds_prologue),
	.ops.next_proto = uet_pds_packet_type_switch_proto,
	.overlay = 1,
};

/* uet_pds_parse_* protocol definitions
 *
 * Parse the various UET-PDS packet types. All except for control are
 * non-leaf nodes, and all of them are fixed size
 *
 * Types are { pull_request, pull_data, push_data, pull_data, resync,
 *	       base_ack, nack }
 */

static const struct xdp2_proto_def uet_pds_parse_rud_rod_request __unused() = {
	.name = "UET-PDS RUD/ROD request",
	.min_len = sizeof(struct uet_pds_rud_rod_request),
	.ops.next_proto = uet_pds_next_proto,
};

static const struct xdp2_proto_def uet_pds_parse_rud_rod_request_cc
								__unused() = {
	.name = "UET-PDS RUD/ROD request with CC",
	.min_len = sizeof(struct uet_pds_rud_rod_request_cc),
	.ops.next_proto = uet_pds_next_proto,
};

static const struct xdp2_proto_def uet_pds_parse_ack __unused() = {
	.name = "UET-PDS ACK",
	.min_len = sizeof(struct uet_pds_ack),
	.ops.next_proto = uet_pds_next_proto,
};

static const struct xdp2_proto_def uet_pds_parse_ack_cc_ov __unused() = {
	.name = "UET-PDS ACK CC",
	.min_len = sizeof(struct uet_pds_ack_cc),
	.overlay = 1,
	.ops.next_proto = uet_pds_next_ack_cc,
};

static const struct xdp2_proto_def uet_pds_parse_ack_cc_nscc __unused() =
									{
	.name = "UET-PDS ACK CC NSCC",
	.min_len = sizeof(struct uet_pds_ack_cc),
	.ops.next_proto = uet_pds_next_proto,
};

static const struct xdp2_proto_def uet_pds_parse_ack_cc_credit __unused() =
									{
	.name = "UET-PDS ACK CC credit",
	.min_len = sizeof(struct uet_pds_ack_cc),
	.ops.next_proto = uet_pds_next_proto,
};

static const struct xdp2_proto_def uet_pds_parse_ack_ccx __unused() = {
	.name = "UET-PDS ACK CC extended",
	.min_len = sizeof(struct uet_pds_ack_ccx),
	.ops.next_proto = uet_pds_next_proto,
};

static const struct xdp2_proto_def uet_pds_parse_control_pkt __unused() = {
	.name = "UET-PDS control packet",
	.min_len = sizeof(struct uet_pds_control_pkt),
};

static const struct xdp2_proto_def uet_pds_parse_rudi_req_resp __unused() = {
	.name = "UET-PDS RUDI request response",
	.min_len = sizeof(struct uet_pds_rudi_req_resp),
	.ops.next_proto = uet_pds_next_proto,
};

static const struct xdp2_proto_def uet_pds_parse_nack __unused() = {
	.name = "UET-PDS Negative ACK",
	.min_len = sizeof(struct uet_pds_nack),
	.ops.next_proto = uet_pds_next_proto,
};

static const struct xdp2_proto_def uet_pds_parse_nack_ccx __unused() = {
	.name = "UET-PDS Negative ACK with CC extended",
	.min_len = sizeof(struct uet_pds_nack_ccx),
	.ops.next_proto = uet_pds_next_proto,
};

static const struct xdp2_proto_def uet_pds_parse_uud_req __unused() = {
	.name = "UET-PDS UUD request",
	.min_len = sizeof(struct uet_pds_uud_req),
	.ops.next_proto = uet_pds_next_proto,
};

#endif /* XDP2_DEFINE_PARSE_NODE */
