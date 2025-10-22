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
#ifndef __UET_PDS_H__
#define __UET_PDS_H__

#include "xdp2/utility.h"

/* Structure definitions and functions for UET-PDS protocol */

#ifndef UET_UDP_PORT_NUM
#define UET_UDP_PORT_NUM 4793
#endif

/* UET-UET entropy header */
struct uet_entropy_hdr {
	__be16 entropy;
	__u16 rsvd;
} __packed;

/* UET-PDS packet types */
enum uet_pds_pkt_type {
	UET_PDS_TYPE_TSS = 1,
	UET_PDS_TYPE_RUD_REQ = 2,
	UET_PDS_TYPE_ROD_REQ = 3,
	UET_PDS_TYPE_RUDI_REQ = 4,
	UET_PDS_TYPE_RUDI_RESP = 5,
	UET_PDS_TYPE_UUD_REQ = 6,
	UET_PDS_TYPE_ACK = 7,
	UET_PDS_TYPE_ACK_CC = 8,
	UET_PDS_TYPE_ACK_CCX = 9,
	UET_PDS_TYPE_NACK = 10,
	UET_PDS_TYPE_CONTROL = 11,
	UET_PDS_TYPE_NACK_CCX = 12,
	UET_PDS_TYPE_RUD_CC_REQ = 13,
	UET_PDS_TYPE_ROD_CC_REQ =14,
};

/* Output a readable string for packet type */
static inline const char *uet_pkt_type_to_text(enum uet_pds_pkt_type type)
{
	switch (type) {
	case UET_PDS_TYPE_TSS:
		return "UET Encryption Header (TSS)";
	case UET_PDS_TYPE_RUD_REQ:
		return "RUD Request (RUD_REQ)";
	case UET_PDS_TYPE_ROD_REQ:
		return "ROD Request (ROD_REQ)";
	case UET_PDS_TYPE_RUDI_REQ:
		return "RUDI Request (RUDI_REQ)";
	case UET_PDS_TYPE_RUDI_RESP:
		return "RUDI Response (RUDI_RESP)";
	case UET_PDS_TYPE_UUD_REQ:
		return "UUD Request (UUD_REQ)";
	case UET_PDS_TYPE_ACK:
		return "ACK";
	case UET_PDS_TYPE_ACK_CC:
		return "ACK with congestion control";
	case UET_PDS_TYPE_ACK_CCX:
		return "ACK with congestion control extended";
	case UET_PDS_TYPE_NACK:
		return "Negative ACK";
	case UET_PDS_TYPE_CONTROL:
		return "Control packet";
	case UET_PDS_TYPE_NACK_CCX:
		return "Negative ACK with congestion control extended";
	case UET_PDS_TYPE_RUD_CC_REQ:
		return "RUD request with congestion control";
	case UET_PDS_TYPE_ROD_CC_REQ:
		return "ROD request with congestion control";
	default:
		return "Unknown type";
	}
}

/* Prologue header common to all packet formats */
struct uet_pds_prologue {
	union {
		struct {
#if defined(__BIG_ENDIAN_BITFIELD)
			__u16 type: 5; /* enum uet_pds_pkt_type */
			__u16 next_hdr_ctrl: 4;
			__u16 flags: 7;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
			__u16 next_hdr_ctrl1: 3;
			__u16 type: 5; /* enum uet_pds_pkt_type */
			__u16 flags: 7;
			__u16 next_hdr_ctrl2: 1;
#else
#error  "Please fix bitfield endianness"
#endif
		};
		__be16 v;
	};
} __packed;

#define UET_PDS_PROLOGUE_TYPE_MASK __cpu_to_be16(0xf804)
#define UET_PDS_MATCH_TYPE(TYPE) __cpu_to_be16((TYPE) << 11)
#define UET_PDS_MATCH_TYPE_SYN(TYPE) __cpu_to_be16(((TYPE) << 11) | (1 << 2))

#define UET_PDS_PROLOGUE_TYPE_SHIFT __cpu_to_be16(0xf804)

/* Helper functions to get/set next-header from bitfields */
static inline __u8 uet_pds_common_get_next_hdr(const void *vhdr)
{
	const struct uet_pds_prologue *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return hdr->next_hdr_ctrl;
#else
	return (hdr->next_hdr_ctrl1 << 1) + hdr->next_hdr_ctrl2;
#endif
}

static inline void uet_pds_common_set_next_hdr(void *vhdr, __u8 val)
{
	struct uet_pds_prologue *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	hdr->next_hdr_ctrl = val;
#else
	hdr->next_hdr_ctrl1 = val >> 1;
	hdr->next_hdr_ctrl2 = val & 0x1;
#endif
}

/* RUD/ROD Request
 *
 * Packet type is UET_PDS_TYPE_RUD_REQ or UET_PDS_TYPE_ROD_REQ
 *
 * Use uet_pds_common_{get,set}_next_hdr to get or set the next header field
 * Use uet_pds_rud_rod_request_{get,set}_psn_offset to set the psn offset
 */
struct uet_pds_rud_rod_request {
#if defined(__BIG_ENDIAN_BITFIELD) /* Prologue */
	__u16 type: 5; /* UET_PDS_TYPE_RUD_REQ or UET_PDS_TYPE_ROD_REQ */
	__u16 next_hdr: 4;
	__u16 rsvd1: 2;
	__u16 retrans: 1;
	__u16 ackreq: 1;
	__u16 syn: 1;
	__u16 rsvd2: 2;
#else
	__u16 next_hdr1: 3;
	__u16 type: 5; /* enum uet_pds_pkt_type */

	__u16 rsvd2: 2;
	__u16 syn: 1;
	__u16 ackreq: 1;
	__u16 retrans: 1;
	__u16 rsvd1: 2;
	__u16 next_hdr2: 1;
#endif
	__be16 clear_psn_offset;
	__be32 psn;
	__be16 spdcid;
	union {
		__be16 dpdcid;
		struct {
#if defined(__BIG_ENDIAN_BITFIELD)
			__u16 use_rsv_pdc: 1;
			__u16 rsvd3: 3;
			__u16 psn_offset: 12;
#else
			__u8 psn_offset1: 4;
			__u8 rsvd3: 3;
			__u8 use_rsv_pdc: 1;

			__u8 psn_offset2;
#endif
		};
	};
} __packed;

/* Helper functions to get/set next-header from bitfields */
static inline __u16 uet_pds_rud_rod_request_get_psn_offset(
		const struct uet_pds_rud_rod_request *req)
{
#if defined(__BIG_ENDIAN_BITFIELD)
	return req->ntohs(psn_offset);
#else
	return (req->psn_offset1 << 8) + req->psn_offset2;
#endif
}

static inline void uet_pds_rud_rod_request_set_psn_offset(
		struct uet_pds_rud_rod_request *req, __be16 val)
{
#if defined(__BIG_ENDIAN_BITFIELD)
	req->psn_offset = htons(val);
#else
	req->psn_offset1 = val >> 8;
	req->psn_offset2 = val & 0xff;
#endif
}

/* RUD/ROD Request with CC state
 *
 * Packet type is UET_PDS_TYPE_RUD_CC_REQ or UET_PDS_TYPE_ROD_CC_REQ
 */
struct uet_pds_rud_rod_request_cc {
	struct uet_pds_rud_rod_request req; /* Set req.type */
	struct {
#if defined(__BIG_ENDIAN_BITFIELD)
		__u32 ccc_id: 8;
		__u32 credit_target: 24;
#else
		__u32 ccc_id: 8;
		__u32 credit_target: 24;
#endif
	} req_cc_state;
} __packed;

/* ACK requests */
enum uet_pds_ack_request_type {
	UET_PDS_ACK_REQUEST_NO_REQUEST = 0,
	UET_PDS_ACK_REQUEST_CLEAR = 1,
	UET_PDS_ACK_REQUEST_CLOSE = 2,
};

/* Output a readable string for ACK request type */
static inline const char *uet_pds_ack_request_type_to_text(
		enum uet_pds_ack_request_type type)
{
	switch (type) {
	case UET_PDS_ACK_REQUEST_NO_REQUEST:
		return "No request";
	case UET_PDS_ACK_REQUEST_CLEAR:
		return "Clear request";
	case UET_PDS_ACK_REQUEST_CLOSE:
		return "Close request";
	default:
		return "Unknown type";
	}
}

/* ACK packet
 *
 * Packet type is UET_PDS_TYPE_ACK
 *
 * Use uet_pds_common_{get,set}_next_hdr to get or set the next header field
 */
struct uet_pds_ack {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u16 type: 5; /* UET_PDS_TYPE_ACK */
	__u16 next_hdr: 4;
	__u16 rsvd1: 1;
	__u16 ecn_marked: 1;
	__u16 retrans: 1;
	__u16 probe: 1;
	__u16 request: 2; /* enum uet_pds_ack_request_type */
	__u16 rsvd2: 1;
#else
	__u16 next_hdr1: 3;
	__u16 type: 5; /* UET_PDS_TYPE_ACK */

	__u16 rsvd2: 1;
	__u16 request: 2; /* enum uet_pds_ack_request_type */
	__u16 probe: 1;
	__u16 retrans: 1;
	__u16 ecn_marked: 1;
	__u16 rsvd1: 1;
	__u16 next_hdr2: 1;
#endif
	union {
		__be16 ack_psn_offset;
		__be16 probe_opaque;
	};
	__be32 cack_psn;
	__be16 spdcid;
	__be16 dpdcid;
} __packed;

/* Congestion control types */
enum uet_pds_cc_type {
	UET_PDS_CC_TYPE_NSCC = 0,
	UET_PDS_CC_TYPE_CREDIT = 1,
};

/* Output a readable string for congestion control type */
static inline const char *uet_pds_cc_type_to_text(
		enum uet_pds_cc_type type)
{
	switch (type) {
	case UET_PDS_CC_TYPE_NSCC:
		return "NSCC";
	case UET_PDS_CC_TYPE_CREDIT:
		return "Credit";
	default:
		return "Unknown type";
	}
}

/* ACK packet with congestion control information
 *
 * Packet type is UET_PDS_TYPE_ACK_CC
 */
struct uet_pds_ack_cc {
	struct uet_pds_ack ack; /* ack.type is UET_PDS_TYPE_ACK_CC */
#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 cc_type: 4; /* enum uet_pds_cc_type */
	__u8 cc_flags: 4; /* Reserved */
#else
	__u8 cc_flags: 4; /* Reserved */
	__u8 cc_type: 4; /* enum uet_pds_cc_type */
#endif
	__u8 mpr;
	__be16 sack_psn_offset;
	__be64 sack_bitmap;

	union {
		struct {
#if defined(__BIG_ENDIAN_BITFIELD)
			__u64 service_time: 16;
			__u64 restore_cwnd: 1;
			__u64 rcv_cwnd_pend: 7;
			__u64 rcvd_bytes: 24;
			__u64 ooo_count: 16;
#else
			/* The following bitfield ordering seems to work.
			 * 64-bit bitfields are a little squirrely
			 */
			__u64 service_time: 16;
			__u64 rcv_cwnd_pend: 7;
			__u64 restore_cwnd: 1;
			__u64 rcvd_bytes: 24;
			__u64 ooo_count: 16;
#endif
		} nscc __packed;
		struct {
			/* No endian difference */
			__u32 credit: 24;
			__u32 rsvd1: 8;

			__u16 rsvd2;
			__u16 ooo_count;
		} credit;
	};
} __packed;

/* ACK packet with congestion control information extended
 *
 * Packet type is UET_PDS_TYPE_ACK_CCX
 */
struct uet_pds_ack_ccx {
	struct uet_pds_ack ack; /* ack.type is UET_PDS_TYPE_ACK_CCX */
#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 ccx_type: 4; /* Reserved, proprietary extensions 14-15 */
	__u8 cc_flags: 4; /* Reserved */
#else
	__u8 cc_flags: 4; /* Reserved */
	__u8 ccx_type: 4; /* Reserved, proprietary extensions 14-15 */
#endif
	__u8 mpr;
	__be16 sack_psn_offset;
	__be64 sack_bitmap;
	__u8 ack_cc_state[8];
};

/* Control packet types */
enum uet_pds_control_pkt_type {
	UET_PDS_CTL_TYPE_NOOP = 0,
	UET_PDS_CTL_TYPE_ACK_REQ = 1,
	UET_PDS_CTL_TYPE_CLEAR_CMD = 2,
	UET_PDS_CTL_TYPE_CLEAR_REQ = 3,
	UET_PDS_CTL_TYPE_CLOSE_CMD = 4,
	UET_PDS_CTL_TYPE_CLOSE_REQ = 5,
	UET_PDS_CTL_TYPE_PROBE = 6,
	UET_PDS_CTL_TYPE_CREDIT = 7,
	UET_PDS_CTL_TYPE_CREDIT_REQ = 8,
	UET_PDS_CTL_TYPE_NEGOTIATION = 9,
};

/* Output a readable string for control packet type */
static inline const char *uet_pds_control_pkt_type_to_text(
					enum uet_pds_control_pkt_type type)
{
	switch (type) {
	case UET_PDS_CTL_TYPE_NOOP:
		return "No operation";
	case UET_PDS_CTL_TYPE_ACK_REQ:
		return "ACK Request";
	case UET_PDS_CTL_TYPE_CLEAR_CMD:
		return "Clear Command";
	case UET_PDS_CTL_TYPE_CLEAR_REQ:
		return "Clear Request";
	case UET_PDS_CTL_TYPE_CLOSE_CMD:
		return "Close Command";
	case UET_PDS_CTL_TYPE_CLOSE_REQ:
		return "Close Request";
	case UET_PDS_CTL_TYPE_PROBE:
		return "Probe";
	case UET_PDS_CTL_TYPE_CREDIT:
		return "Credit";
	case UET_PDS_CTL_TYPE_CREDIT_REQ:
		return "Credit Request";
	case UET_PDS_CTL_TYPE_NEGOTIATION:
		return "Negotiation";
	default:
		return "Unknown type";
	}
}

/* Control packet
 *
 * Packet type is UET_PDS_TYPE_CONTROL
 *
 * Use uet_pds_common_{get,set}_next_hdr to get or set the ctl_type field
 * Use uet_pds_control_pkt_{get,set}_psn_offset to get or set the psn offset
 */
struct uet_pds_control_pkt {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u16 type: 5; /* UET_PDS_TYPE_CONTROL */
	__u16 ctl_type: 4; /* enum uet_pds_control_pkt_type */
	__u16 rsvd1: 1;
	__u16 rsvd_isrod: 1;
	__u16 retrans: 1;
	__u16 ackreq: 1;
	__u16 syn: 1;
	__u16 rsvd2: 2;
#else
	__u16 ctl_type1: 3; /* enum uet_pds_control_pkt_type */
	__u16 type: 5; /* UET_PDS_TYPE_CONTROL */

	__u16 rsvd2: 2;
	__u16 syn: 1;
	__u16 ackreq: 1;
	__u16 retrans: 1;
	__u16 rsvd_isrod: 1;
	__u16 rsvd1: 1;
	__u16 ctl_type2: 1;
#endif
	__u16 probe_opaque;
	__be32 psn;
	__be16 spdcid;
	union {
		__be16 dpdcid;
		struct {
#if defined(__BIG_ENDIAN_BITFIELD)
			__u16 use_rsv_pdc: 1;
			__u16 rsvd3: 3;
			__u16 psn_offset: 12;
#else
			__u16 psn_offset1: 4;
			__u16 rsvd3: 3;
			__u16 use_rsv_pdc: 1;
			__u16 psn_offset2: 8;
#endif
		};
	};
	__be32 payload;
} __packed;

/* Helper functions to get/set psn-offset from bitfields in control packet */
static inline __u16 uet_pds_control_pkt_get_psn_offset(
		const struct uet_pds_control_pkt *ctrl)
{
#if defined(__BIG_ENDIAN_BITFIELD)
	return ctrl->ntohs(psn_offset);
#else
	return (ctrl->psn_offset1 << 8) + ctrl->psn_offset2;
#endif
}

static inline void uet_pds_control_pkt_set_psn_offset(
		struct uet_pds_control_pkt *ctrl, __be16 val)
{
#if defined(__BIG_ENDIAN_BITFIELD)
	ctrl->psn_offset = htons(val);
#else
	ctrl->psn_offset1 = val >> 8;
	ctrl->psn_offset2 = val & 0xff;
#endif
}

/* Helper functions to get/set control type from bitfields in control packet */
static inline __u16 uet_pds_control_pkt_get_ctl_type(
		const struct uet_pds_control_pkt *ctrl)
{
#if defined(__BIG_ENDIAN_BITFIELD)
	return ctrl->ctl_type;
#else
	return (ctrl->ctl_type1 << 1) + ctrl->ctl_type2;
#endif
}

static inline void uet_pds_control_pkt_set_ctl_type(
		struct uet_pds_control_pkt *ctrl, __u8 ctl_type)
{
#if defined(__BIG_ENDIAN_BITFIELD)
	ctrl->ctl_type = ctl_type;
#else
	ctrl->ctl_type1 = ctl_type >> 1;
	ctrl->ctl_type2 = ctl_type & 0x1;
#endif
}

/* RUDI request or response
 *
 * Packet type is UET_PDS_TYPE_RUD_CC_REQ or UET_PDS_TYPE_ROD_CC_REQ
 *
 * Use uet_pds_common_{get,set}_next_hdr to get or set the next header field
 */
struct uet_pds_rudi_req_resp {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u16 type: 5; /* UET_PDS_TYPE_RUD_CC_REQ or UET_PDS_TYPE_ROD_CC_REQ */
	__u16 next_hdr: 4;
	__u16 rsvd1: 1;
	__u16 ecn_marked: 1; /* RUDI responses only */
	__u16 retrans: 1;
	__u16 rsvd2: 4;
#else
	__u16 next_hdr1: 3;
	__u16 type: 5; /* UET_PDS_TYPE_RUD_CC_REQ or UET_PDS_TYPE_ROD_CC_REQ */

	__u16 rsvd2: 4;
	__u16 retrans: 1;
	__u16 ecn_marked: 1; /* RUDI responses only */
	__u16 rsvd1: 1;
	__u16 next_hdr2: 1;
#endif
	__u16 rsvd3;
	__be32 pkt_id;
} __packed;

/* NACK packet types */
enum uet_nack_type {
	UET_PDS_NACK_TYPE_RUD_ROD = 0,
	UET_PDS_NACK_TYPE_RUDI = 1,
};

/* Output a readable string for NACK type */
static inline const char *uet_pds_nack_type_to_text(enum uet_nack_type code)
{
	switch (code) {
	case UET_PDS_NACK_TYPE_RUD_ROD:
		return "RUD-ROD";
	case UET_PDS_NACK_TYPE_RUDI:
		return "RUDI";
	default:
		return "Unknown type";
	}
}

/* NACK codes */
enum uet_pds_nack_code {
	UET_PDS_NACK_CODE_TRIMMED = 0x1,
	UET_PDS_TRIMMED_LASTHOP = 0x2,
	UET_PDS_NACK_CODE_TRIMMED_ACK = 0x3,
	UET_PDS_NACK_CODE_NO_PDC_AVAIL = 0x4,
	UET_PDS_NACK_CODE_NO_CCC_AVAIL = 0x5,
	UET_PDS_NACK_CODE_NO_BITMAP = 0x6,
	UET_PDS_NACK_CODE_NO_PKT_BUFFER = 0x7,
	UET_PDS_NACK_CODE_NO_GTD_DEL_AVAIL = 0x8,
	UET_PDS_NACK_CODE_NO_SES_MSG_AVAIL = 0x9,
	UET_PDS_NACK_CODE_NO_RESOURCE = 0xa,
	UET_PDS_NACK_CODE_PSN_OOR_WINDOW = 0xb,
	UET_PDS_NACK_CODE_ROD_OOO = 0xd,
	UET_PDS_NACK_CODE_INV_DPDCID = 0xe,
	UET_PDS_NACK_CODE_PDC_HDR_MISMATCH = 0xf,
	UET_PDS_NACK_CODE_CLOSING = 0x10,
	UET_PDS_NACK_CODE_CLOSING_IN_ERR = 0x11,
	UET_PDS_NACK_CODE_PKT_NOT_RCVD = 0x12,
	UET_PDS_NACK_CODE_GTD_RESP_UNAVAIL = 0x13,
	UET_PDS_NACK_CODE_ACK_WITH_DATA = 0x14,
	UET_PDS_NACK_CODE_INVALID_SYN = 0x15,
	UET_PDS_NACK_CODE_PDC_MODE_MISMATCH = 0x16,
	UET_PDS_NACK_CODE_NEW_START_PSN = 0x17,
	UET_PDS_NACK_CODE_RCVD_SES_PROCG = 0x18,
	UET_PDS_NACK_CODE_UNEXP_EVENT = 0x19,
	UET_PDS_NACK_CODE_RCVR_INFER_LOSS = 0x1a,

	UET_PDS_NACK_CODE_EXP_NACK_NORMAL = 0xfd,
	UET_PDS_NACK_CODE_EXP_NACK_ERR = 0xfe,
	UET_PDS_NACK_CODE_EXP_NACK_FATAL = 0xff,
};

/* Output a readable string for NACK code */
static inline const char *uet_pds_nack_code_to_text(
						enum uet_pds_nack_code code)
{
	switch (code) {
	case UET_PDS_NACK_CODE_TRIMMED:
		return "Packet was trimmed";
	case UET_PDS_TRIMMED_LASTHOP:
		return "Packet trimmed at last hop";
	case UET_PDS_NACK_CODE_TRIMMED_ACK:
		return "ACK with response data trimmed";
	case UET_PDS_NACK_CODE_NO_PDC_AVAIL:
		return "No PDC resource available";
	case UET_PDS_NACK_CODE_NO_CCC_AVAIL:
		return "No CCC resource available";
	case UET_PDS_NACK_CODE_NO_BITMAP:
		return "No bitmap available";
	case UET_PDS_NACK_CODE_NO_PKT_BUFFER:
		return "No packet buffer resource available";
	case UET_PDS_NACK_CODE_NO_GTD_DEL_AVAIL:
		return "No SES guarnateed delivery response available";
	case UET_PDS_NACK_CODE_NO_SES_MSG_AVAIL:
		return "No message tracking state available";
	case UET_PDS_NACK_CODE_NO_RESOURCE:
		return "General resource not available";
	case UET_PDS_NACK_CODE_PSN_OOR_WINDOW:
		return "PSN outside of tracking window";
	case UET_PDS_NACK_CODE_ROD_OOO:
		return "PSN arrived OOO on a ROD PDC";
	case UET_PDS_NACK_CODE_INV_DPDCID:
		return "DPDCID is not recognized and not a SYN";
	case UET_PDS_NACK_CODE_PDC_HDR_MISMATCH:
		return "COnnection state not matched";
	case UET_PDS_NACK_CODE_CLOSING:
		return "Target PDCID is closed or closing";
	case UET_PDS_NACK_CODE_CLOSING_IN_ERR:
		return "Timeour at target during close process";
	case UET_PDS_NACK_CODE_PKT_NOT_RCVD:
		return "Control packet with ACK req and PSN not received";
	case UET_PDS_NACK_CODE_GTD_RESP_UNAVAIL:
		return "Unable to handle duplicate PSN";
	case UET_PDS_NACK_CODE_ACK_WITH_DATA:
		return "Control packet with ACK request and read response data";
	case UET_PDS_NACK_CODE_INVALID_SYN:
		return "Invalid SYN received";
	case UET_PDS_NACK_CODE_PDC_MODE_MISMATCH:
		return "Delivery mode mismatch for a packet";
	case UET_PDS_NACK_CODE_NEW_START_PSN:
		return "Resend all packets with new start_psn";
	case UET_PDS_NACK_CODE_RCVD_SES_PROCG:
		return "Delayed packet received and retrans arrived at close";
	case UET_PDS_NACK_CODE_UNEXP_EVENT:
		return "Unexpected event";
	case UET_PDS_NACK_CODE_RCVR_INFER_LOSS:
		return "Destination inferred a PSN was loat";

	case UET_PDS_NACK_CODE_EXP_NACK_NORMAL:
		return "Experimental (0xFD)";
	case UET_PDS_NACK_CODE_EXP_NACK_ERR:
		return "Experimental (0xFE)";
	case UET_PDS_NACK_CODE_EXP_NACK_FATAL:
		return "Experimental (0xFF)";
	default:
		return "Unknown code";
	}
}

/* Negative acknowledgment
 *
 * Packet type is UET_PDS_TYPE_NACK
 *
 * Use uet_pds_common_{get,set}_next_hdr to get or set the next header field
 */
struct uet_pds_nack {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u16 type: 5; /* UET_PDS_TYPE_NACK */
	__u16 next_hdr: 4;
	__u16 rsvd1: 1;
	__u16 ecn_marked: 1;
	__u16 retrans: 1;
	__u16 nack_type: 1; /* enum uet_pds_nack_type */
	__u16 rsvd2: 3;
#else
	__u16 next_hdr1: 3;
	__u16 type: 5; /* UET_PDS_TYPE_NACK */

	__u16 rsvd2: 3;
	__u16 nack_type: 1; /* enum uet_pds_nack_type */
	__u16 retrans: 1;
	__u16 ecn_marked: 1;
	__u16 rsvd1: 1;
	__u16 next_hdr2: 1;
#endif
	__u8 nack_code;
	__u8 vendor_code;
	union {
		__be32 nack_psn;
		__be32 nack_pkt_id;
	};
	__be16 spdcid;
	__be16 dpdcid;
	__be32 payload;
} __packed;

/* Negative acknowledgment with congestion control extended
 *
 * Packet type is UET_PDS_TYPE_NACK_CCX
 */
struct uet_pds_nack_ccx {
	struct uet_pds_nack nack; /* nack.type = UET_PDS_TYPE_NACK_CCX */
#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 nccx_type: 4; /* Reserved, proprietary extensions 14-15 */
	__u8 nccx_ccx_state1: 4;
#else
	__u8 nccx_ccx_state1: 4;
	__u8 nccx_type: 4; /* Reserved, proprietary extensions 14-15 */
#endif
	__u8 nack_ccx_state2[7]; /* Reserved for future exensibility */
} __packed;

/* UUD request
 *
 * Packet type UET_PDS_TYPE_UUD_REQ
 *
 * Use uet_pds_common_{get,set}_next_hdr to get or set the next header field
 */
struct uet_pds_uud_req {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u16 type: 5; /* UET_PDS_TYPE_UUD_REQ */
	__u16 next_hdr: 4;
	__u16 flags: 7; /* Ignored, assumed to be zero */
#else
	__u16 next_hdr1: 3;
	__u16 type: 5; /* UET_PDS_TYPE_UUD_REQ */

	__u16 flags: 7; /* Ignored, assumed to be zero */
	__u16 next_hdr2: 1;
#endif
	__u16 rsvd;
} __packed;

#endif /* __UET_PDS_H__ */
