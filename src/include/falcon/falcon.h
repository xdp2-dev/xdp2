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
#ifndef __FALCON_H__
#define __FALCON_H__

#include "xdp2/utility.h"

/* Structure definitions and functions for Falcon protocol */

#ifndef FALCON_UDP_PORT_NUM
#define FALCON_UDP_PORT_NUM 7777
#endif

/* Falcon protocol version, currently set to 1 */
#define FALCON_PROTO_VERSION 1

/* Falcon packet types */
enum falcon_pkt_type {
	FALCON_PKT_TYPE_PULL_REQUEST = 0,
	FALCON_PKT_TYPE_PULL_DATA = 3,
	FALCON_PKT_TYPE_PUSH_DATA = 5,
	FALCON_PKT_TYPE_RESYNC = 6,
	FALCON_PKT_TYPE_NACK = 8,
	FALCON_PKT_TYPE_BACK = 9,
	FALCON_PKT_TYPE_EACK = 10,
};

static inline const char *falcon_pkt_type_to_text(enum falcon_pkt_type type)
{
	switch (type) {
	case FALCON_PKT_TYPE_PULL_REQUEST:
		return "Pull Request";
	case FALCON_PKT_TYPE_PULL_DATA:
		return "Pull Data";
	case FALCON_PKT_TYPE_PUSH_DATA:
		return "Push Data";
	case FALCON_PKT_TYPE_RESYNC:
		return "Resync";
	case FALCON_PKT_TYPE_NACK:
		return "Negative acknowledgment";
	case FALCON_PKT_TYPE_BACK:
		return "Acknowledgment";
	case FALCON_PKT_TYPE_EACK:
		return "Extended acknowledgment";
	default:
		return "Unknown type";
	}
}

/* Falcon protocol types */
enum falcon_protocol_type {
	FALCON_PROTO_TYPE_RDMA = 2,
	FALCON_PROTO_TYPE_NVME = 3,
};

static inline const char *falcon_proto_type_to_text(
					enum falcon_protocol_type type)
{
	switch (type) {
	case FALCON_PROTO_TYPE_RDMA:
		return "RDMA";
	case FALCON_PROTO_TYPE_NVME:
		return "NVMe";
	default:
		return "Unknown type";
	}
}

/* Resync codes */
enum falcon_resync_code {
	FALCON_RESYNC_CODE_TARG_ULP_COMPLETE_IN_ERROR = 1,
	FALCON_RESYNC_CODE_LOCAL_XLR_FLOW = 2,
	FALCON_RESYNC_CODE_PKT_RETRANS_ERROR = 3,
	FALCON_RESYNC_CODE_TRANSACTION_TIMEOUT = 4,
	FALCON_RESYNC_CODE_REMOTE_XLR_FLOW = 5,
	FALCON_RESYNC_CODE_TARG_ULP_NONRECOV_ERROR = 6,
	FALCON_RESYNC_CODE_TARG_ULP_INVALID_CID_ERROR = 7,
};

static inline const char *falcon_resync_code_to_text(
						enum falcon_resync_code code)
{
	switch (code) {
	case FALCON_RESYNC_CODE_TARG_ULP_COMPLETE_IN_ERROR:
		return "Target ULP complete-in-error";
	case FALCON_RESYNC_CODE_LOCAL_XLR_FLOW:
		return "Local xLR flow";
	case FALCON_RESYNC_CODE_PKT_RETRANS_ERROR:
		return "Packet (exhausting) Retransmission Error";
	case FALCON_RESYNC_CODE_TRANSACTION_TIMEOUT:
		return "Transaction timed out";
	case FALCON_RESYNC_CODE_REMOTE_XLR_FLOW:
		return "Remote xLR flow";
	case FALCON_RESYNC_CODE_TARG_ULP_NONRECOV_ERROR:
		return "Target ULP non-recoverable error";
	case FALCON_RESYNC_CODE_TARG_ULP_INVALID_CID_ERROR:
		return "Target ULP invalid CID error";
	default:
		return "Unknown code";
	}
}

/* NACK codes */
enum falcon_nack_code {
	FALCON_NACK_CODE_INSUFF_RESOURCES = 1,
	FALCON_NACK_CODE_ULP_NOT_READY = 2,
	FALCON_NACK_CODE_XLR_DROP = 4,
	FALCON_NACK_CODE_ULP_COMPLETE_IN_ERROR = 6,
	FALCON_NACK_CODE_ULP_NONRECOV_ERROR = 7,
	FALCON_NACK_CODE_INVAILD_CID = 8,
};

static inline const char *falcon_nack_code_to_text(enum falcon_nack_code code)
{
	switch (code) {
	case FALCON_NACK_CODE_INSUFF_RESOURCES:
		return "Insufficient resources";
	case FALCON_NACK_CODE_ULP_NOT_READY:
		return "ULP not ready";
	case FALCON_NACK_CODE_XLR_DROP:
		return "xLR drop";
	case FALCON_NACK_CODE_ULP_COMPLETE_IN_ERROR:
		return "ULP complete in error";
	case FALCON_NACK_CODE_ULP_NONRECOV_ERROR:
		return "ULP nonrecoverable error";
	case FALCON_NACK_CODE_INVAILD_CID:
		return "Invalid context ID";
	default:
		return "Unknown code";
	}
}

/* Convert the Receiver Not Ready timeout field value in a NACK packet to
 * time in units of 10's of microseconds (per table in the Falcon spec)
 */
static inline unsigned int falcon_rnr_nack_field_val_to_timeout(
						unsigned int field_val)
{
	switch (field_val) {
	case 0: return 65536;
	case 1: return 1;
	case 2: return 2;
	case 3: return 3;
	case 4: return 4;
	case 5: return 6;
	case 6: return 8;
	case 7: return 12;
	case 8: return 16;
	case 9: return 24;
	case 10: return 32;
	case 11: return 48;
	case 12: return 64;
	case 13: return 96;
	case 14: return 128;
	case 15: return 192;
	case 16: return 256;
	case 17: return 384;
	case 18: return 512;
	case 19: return 768;
	case 20: return 1024;
	case 21: return 1536;
	case 22: return 2048;
	case 23: return 3072;
	case 24: return 4096;
	case 25: return 6144;
	case 26: return 8192;
	case 27: return 12288;
	case 28: return 16384;
	case 29: return 24576;
	case 30: return 32768;
	case 31: return 49152;
	default:
		XDP2_ERR(1, "Invalid timeout value");
	}
}

/* Timestamp units in picseconds */
#define FALCON_TIMESTAMP_UNITS 131072

/* Header and packet definitions */

/* Pseudo header for extracting the Falcon version number */
struct falcon_version_switch_header {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 version: 4;
	__u8 rsvd: 4;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
	__u8 rsvd: 4;
	__u8 version: 4;
#else
#error  "Please fix bitfield endianness"
#endif
} __packed;

/* Pseudo header for extracting the Falcon protocol type */
struct falcon_packet_type_switch_header {
	__be32 rsvd1;
#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 rsvd2: 1;
	__u32 packet_type: 4;
	__u32 rsvd3: 27;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
	__u32 rsvd2: 25;
	__u32 packet_type: 4;
	__u32 rsvd3: 3;
#endif
} __packed;

/* Base header for several packet types */
struct falcon_base_hdr {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 version: 4;
	__u32 rsvd: 4;
#else
	__u32 rsvd: 4;
	__u32 version: 4;
#endif
	__u32 dest_cid: 24;

	__u32 dest_function: 24;
#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 protocol_type: 3;
	__u32 packet_type: 4;
	__u32 ack_req: 1;
#else
	__u32 ack_req: 1;
	__u32 packet_type: 4;
	__u32 protocol_type: 3;
#endif

	__be32 ack_data_psn;
	__be32 ack_req_psn;
	__be32 psn;
	__be32 req_seqno;
} __packed;

/* Pull request packet. Packet type is FALCON_PKT_TYPE_PULL_REQUEST */
struct falcon_pull_req_pkt {
	struct falcon_base_hdr base_hdr;
	__be16 rsvd1;
	__be16 req_length;
	__be32 rsvd2;
} __packed;

/* Pull data packet. Packet type is FALCON_PKT_TYPE_PULL_DATA */
struct falcon_pull_data_pkt {
	struct falcon_base_hdr base_hdr;
	__u8 payload[0];
} __packed;

/* Push data packet. Packet type is FALCON_PKT_TYPE_PUSH_DATA */
struct falcon_push_data_pkt {
	struct falcon_base_hdr base_hdr;
	__be16 rsvd;
	__be16 req_length;
	__u8 payload[0];
} __packed;

/* Resync packet. Packet type is FALCON_PKT_TYPE_RESYNC */
struct falcon_resync_pkt {
	struct falcon_base_hdr base_hdr;

	__u32 resync_code: 8;
#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 resync_pkt_type: 4;
	__u32 rsvd1: 4;
#else
	__u32 rsvd1: 4;
	__u32 resync_pkt_type: 4;
#endif
	__u32 rsvd2: 16;

	__be32 vendor_defined;
} __packed;

/* Base ACK packet. Packet type is FALCON_PKT_TYPE_BACK */
struct falcon_base_ack_pkt {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 version: 4;
	__u32 rsvd1: 4;
#else
	__u32 rsvd1: 4;
	__u32 version: 4;
#endif
	__u32 dest_cid: 24;

	__u32 rsvd2: 24;
#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 rsvd3: 3;
	__u32 pkt_type: 4;
	__u32 rsvd4: 1;
#else
	__u32 rsvd3: 1;
	__u32 pkt_type: 4;
	__u32 rsvd4: 3;
#endif

	__be32 ack_data_psn;
	__be32 ack_req_psn;
	__be32 timestamp_t1;
	__be32 timestamp_t2;

#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 hop_count: 4;
	__u32 rx_buffer_occ: 5;
	__u32 ecn_rx_pkt_cnt: 14;
	__u32 rsvd5: 1;
#else
	__u32 rx_buffer_occ1: 4;
	__u32 hop_count: 4;
	__u32 ecn_rx_pkt_cnt1: 7;
	__u32 rx_buffer_occ2: 1;
	__u32 rsvd5: 1;
	__u32 ecn_rx_pkt_cnt2: 7;
#endif
	__u32 rsvd6: 8;

	__u32 rsvd7: 8;
#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 rsvd8: 1;
	__u32 rue_info: 21
	__u32 oo_wind_notify: 2;
#else
	__u32 rue_info1: 7;
	__u32 rsvd8: 1;
	__u32 rue_info2: 8;
	__u32 oo_wind_notify: 2;
	__u32 rue_info3: 6;
#endif

} __packed;

/* Helper functions to get/set RUE info from bitfields in base ACK */
static inline __u32 falcon_base_ack_get_rue_info(const void *vhdr)
{
	const struct falcon_base_ack_pkt *bhdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return bhdr->rue_info;
#else
	return (bhdr->rue_info1 << 14) | (bhdr->rue_info2 << 6) |
		bhdr->rue_info3;
#endif
}

static inline void falcon_base_ack_set_rue_info(void *vhdr, __u32 rue_info)
{
	struct falcon_base_ack_pkt *bhdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	bhdr->rue_info = val;
#else
	bhdr->rue_info1 = rue_info >> 14;
	bhdr->rue_info2 = (rue_info >> 6) & 0xff;
	bhdr->rue_info3 = rue_info & 0x3f;
#endif
}

/* Helper functions to get/set buffer occupancy from bitfields in base ACK */
static inline __u8 falcon_base_ack_get_rx_buffer_occ(const void *vhdr)
{
	const struct falcon_base_ack_pkt *bhdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return bhdr->rx_buffer_occ1;
#else
	return (bhdr->rx_buffer_occ1 << 1) + bhdr->rx_buffer_occ2;
#endif
}

static inline void falcon_base_ack_set_rx_buffer_occ(
		void *vhdr, __u8 rx_buffer_occ)
{
	struct falcon_base_ack_pkt *bhdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	bhdr->rx_buffer_occ = val;
#else
	bhdr->rx_buffer_occ1 = rx_buffer_occ >> 1;
	bhdr->rx_buffer_occ2 = rx_buffer_occ & 0x1;
#endif
}

/* Helper functions to get/set ECN packet count from bitfields in base ACK */
static inline __u16 falcon_base_ack_get_ecn_rx_pkt_cnt(const void *vhdr)
{
	const struct falcon_base_ack_pkt *bhdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return bhdr->ecn_rx_pkt_cnt;
#else
	return (bhdr->ecn_rx_pkt_cnt1 << 7) + bhdr->ecn_rx_pkt_cnt2;
#endif
}

static inline void falcon_base_ack_set_ecn_rx_pkt_cnt(
		void *vhdr, __u16 ecn_rx_pkt_cnt)
{
	struct falcon_base_ack_pkt *bhdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	bhdr->ecn_rx_pkt_cnt = val;
#else
	bhdr->ecn_rx_pkt_cnt1 = ecn_rx_pkt_cnt >> 7;
	bhdr->ecn_rx_pkt_cnt2 = ecn_rx_pkt_cnt & 0x7f;
#endif
}

/* Extended ACK. Packet type is FALCON_PKT_TYPE_EACK */
struct falcon_ext_ack_pkt {
	struct falcon_base_ack_pkt base_ack;
	__be64 data_ack_bitmap[2];
	__be64 data_rx_bitmap[2];
	__be64 req_ack_bitmap;
} __packed;

/* Negative ACK. Packet type is FALCON_PKT_TYPE_NACK */
struct falcon_nack_pkt {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 version: 4;
	__u32 rsvd1: 4;
#else
	__u32 rsvd1: 4;
	__u32 version: 4;
#endif
	__u32 dest_cid: 24;

	__u32 rsvd2: 24;
#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 rsvd3: 3;
	__u32 pkt_type: 4;
	__u32 rsvd4: 1;
#else
	__u32 rsvd3: 1;
	__u32 pkt_type: 4;
	__u32 rsvd4: 3;
#endif

	__be32 ack_data_psn;
	__be32 ack_req_psn;
	__be32 timestamp_t1;
	__be32 timestamp_t2;

#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 hop_count: 4;
	__u32 rx_buffer_occ: 5;
	__u32 ecn_rx_pkt_cnt: 14;
	__u32 rsvd5: 1;
#else
	__u32 rx_buffer_occ1: 4;
	__u32 hop_count: 4;
	__u32 ecn_rx_pkt_cnt1: 7;
	__u32 rx_buffer_occ2: 1;
	__u32 rsvd5: 1;
	__u32 ecn_rx_pkt_cnt2: 7;
#endif
	__u32 rsvd6: 8;

	__u32 rsvd7: 8;
#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 rsvd8: 1;
	__u32 rue_info: 23;
#else
	__u32 rue_info1: 7;
	__u32 rsvd8: 1;
	__u32 rue_info2: 8;
	__u32 rsvd9: 2;
	__u32 rue_info3: 6;
#endif

	__be32 nack_psn;

	__u8 nack_code;

#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 rsvd10: 3;
	__u8 rnr_nack_timeout: 5;
#else
	__u8 rnr_nack_timeout: 5;
	__u8 rsvd10: 3;
#endif

#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 window: 1;
	__u8  rsvd11: 7;
#else
	__u8  rsvd11: 7;
	__u8 window: 1;
#endif

	__u8 ulp_nack_code;
} __packed;

/* Helper functions to get/set RUE info from bitfields in NACK */
static inline __u32 falcon_nack_get_rue_info(const void *vhdr)
{
	const struct falcon_nack_pkt *nack = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return nack->rue_info;
#else
	return (nack->rue_info1 << 14) | (nack->rue_info2 << 6) |
	       nack->rue_info3;
#endif
}

static inline void falcon_nack_set_rue_info(void *vhdr, __u32 rue_info)
{
	struct falcon_nack_pkt *nack = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	nack->rue_info = val;
#else
	nack->rue_info1 = rue_info >> 14;
	nack->rue_info2 = (rue_info >> 6) & 0xff;
	nack->rue_info3 = rue_info & 0x3f;
#endif
}

/* Helper functions to get/set buffer occupancy from bitfields in NACK */
static inline __u8 falcon_nack_get_rx_buffer_occ(const void *vhdr)
{
	const struct falcon_nack_pkt *nack = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return nack->rx_buffer_occ1;
#else
	return (nack->rx_buffer_occ1 << 1) + nack->rx_buffer_occ2;
#endif
}

static inline void falcon_nack_set_rx_buffer_occ(
		void *vhdr, __u8 rx_buffer_occ)
{
	struct falcon_nack_pkt *nack = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	nack->rx_buffer_occ = val;
#else
	nack->rx_buffer_occ1 = rx_buffer_occ >> 1;
	nack->rx_buffer_occ2 = rx_buffer_occ & 0x1;
#endif
}

/* Helper functions to get/set ECN packet count from bitfields in NACK */
static inline __u16 falcon_nack_get_ecn_rx_pkt_cnt(const void *vhdr)
{
	const struct falcon_nack_pkt *nack = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return nack->ecn_rx_pkt_cnt;
#else
	return (nack->ecn_rx_pkt_cnt1 << 7) + nack->ecn_rx_pkt_cnt2;
#endif
}

static inline void falcon_nack_set_ecn_rx_pkt_cnt(
		void *vhdr, __u16 ecn_rx_pkt_cnt)
{
	struct falcon_nack_pkt *nack = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	nack->ecn_rx_pkt_cnt = val;
#else
	nack->ecn_rx_pkt_cnt1 = ecn_rx_pkt_cnt >> 7;
	nack->ecn_rx_pkt_cnt2 = ecn_rx_pkt_cnt & 0x7f;
#endif
}

#endif /* __FALCON_H__ */
