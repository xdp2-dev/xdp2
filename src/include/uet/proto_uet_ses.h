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

#ifndef __PROTO_UET_SES_H__
#define __PROTO_UET_SES_H__

/* UET Packet Delivery Sub-system (PDS) protocol definitions */

#include "uet/ses.h"
#include "xdp2/parser.h"

/* Get SES opcode/version */
static inline int uet_ses_get_opcode_version(const void *vhdr)
{
	const struct uet_ses_base_hdr *hdr = vhdr;

	return hdr->opcode + (hdr->version << 6);
}

/* Get SOM flag as a next type */
static inline int uet_ses_get_std_msg_som(const void *vhdr)
{
	const struct uet_ses_common_hdr *chdr = vhdr;

	return chdr->start_of_msg;
}

/* Get atomic opcode from an AMO extension header */
static inline int uet_ses_get_atomic_opcode(const void *vhdr)
{
	const struct uet_ses_atomic_op_ext_hdr *ext_hdr = vhdr;

	return ext_hdr->atomic_code;
}

#endif /* __PROTO_UET_PDS_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* Note that PDS contains SES packet type */

/* Atomic extension header */
static const struct xdp2_proto_def xdp2_parse_uet_atomic_op_ext_hdr
							__unused() = {
	.name = "UET-SES atomic extension header",
	.min_len = sizeof(struct uet_ses_atomic_op_ext_hdr),
};

/* Compare and swap atomic extension header */
static const struct xdp2_proto_def uet_ses_parse_atomic_cmp_and_swap_hdr
							__unused() = {
	.name = "UET-SES atomic compare and swap extension header",
	.min_len = sizeof(struct uet_ses_atomic_cmp_and_swap_ext_hdr),
};

/* Common request header for std, medium, and small */
static const struct xdp2_proto_def uet_ses_parse_common_hdr_ov __unused() = {
	.name = "UET-SES common header",
	.min_len = sizeof(struct uet_ses_base_hdr),
	.ops.next_proto = uet_ses_get_opcode_version,
	.overlay = 1,
};

/* Atomic extension header */
static const struct xdp2_proto_def uet_ses_parse_atomic_ext_hdr __unused() = {
	.name = "UET-SES atomic std",
	.min_len = sizeof(struct uet_ses_atomic_op_ext_hdr),
};

/* Compare and swap atomic extension header */
static const struct xdp2_proto_def uet_ses_parse_atomic_cmp_and_swap_ext_hdr
							__unused() = {
	.name = "UET-SES atomic cmp and swap std",
	.min_len = sizeof(struct uet_ses_atomic_cmp_and_swap_ext_hdr),
};

/* Overlay to switch on atomic opcode in extenstion header */
static const struct xdp2_proto_def uet_ses_parse_atomic_op_ov
							__unused() = {
	.name = "UET-SES atomic op",
	.min_len = sizeof(struct uet_ses_atomic_op_ext_hdr),
	.ops.next_proto = uet_ses_get_atomic_opcode,
	.overlay = 1,
};

/* Protocol definitions for standard requests */

/* Overlay for standard request header, next protocol is SES opcode */
static const struct xdp2_proto_def uet_ses_parse_request_std_hdr
							__unused() = {
	.name = "UET-SES standard header",
	.min_len = sizeof(struct uet_ses_request_std_hdr),
};

/* Overlay for standard request header, next protocol is SOM flag */
static const struct xdp2_proto_def uet_ses_parse_std_message_som_base_hdr_ov
							__unused() = {
	.name = "UET-SES standard header for SOM",
	.min_len = sizeof(struct uet_ses_request_std_hdr),
	.ops.next_proto = uet_ses_get_std_msg_som,
	.overlay = 1,
};

/* Deferrabale send */
static const struct xdp2_proto_def uet_ses_parse_defer_send_hdr
							__unused() = {
	.name = "UET-SES deferred send header",
	.min_len = sizeof(struct uet_ses_defer_send_std_hdr),
};

/* Ready to restart */
static const struct xdp2_proto_def uet_ses_parse_ready_to_restart_req_hdr
							__unused() = {
	.name = "UET-SES ready to restart request header",
	.min_len = sizeof(struct uet_ses_ready_to_restart_std_hdr),
};

/* Rendezvous extension header */
static const struct xdp2_proto_def uet_ses_parse_rendezvous_ext_hdr
							__unused() = {
	.name = "UET-SES rendezvous extension header",
	.min_len = sizeof(struct uet_ses_rendezvous_ext_hdr),
};

/* Rendezvous send */
static const struct xdp2_proto_def uet_ses_parse_rendezvous_hdr
							__unused() = {
	.name = "UET-SES rendezvous send",
	.min_len = sizeof(struct uet_ses_rendezvous_std_hdr),
};

/* Protocol definitions for meidum requests */
static const struct xdp2_proto_def uet_ses_parse_request_medium_hdr
							__unused() = {
	.name = "UET-SES request medium header",
	.min_len = sizeof(struct uet_ses_request_medium_hdr),
};

/* Protocol definitions for small requests */
static const struct xdp2_proto_def uet_ses_parse_request_small_hdr
							__unused() = {
	.name = "UET-SES request small header",
	.min_len = sizeof(struct uet_ses_request_small_hdr),
};

/* SES responses */

/* Response with no data */
static const struct xdp2_proto_def uet_ses_parse_uet_ses_nodata_response_hdr
							__unused() = {
	.name = "UET-SES response nodata header",
	.min_len = sizeof(struct uet_ses_nodata_response_hdr),
};

/* Response with data */
static const struct xdp2_proto_def uet_ses_parse_uet_ses_with_data_response_hdr
							__unused() = {
	.name = "UET-SES response with data header",
	.min_len = sizeof(struct uet_ses_with_data_response_hdr),
};

/* Response with small data */
static const struct xdp2_proto_def
	uet_ses_parse_uet_ses_with_small_data_response_hdr __unused() = {
	.name = "UET-SES response with small data header",
	.min_len = sizeof(struct uet_ses_with_small_data_response_hdr),
};

#endif /* XDP2_DEFINE_PARSE_NODE */
