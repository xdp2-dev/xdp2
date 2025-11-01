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
#ifndef __SUPERP_H__
#define __SUPERP_H__

#include "xdp2/utility.h"

/* Structure definitions and functions for Scale Up EtheRnet Protrocol
 * (SUPERp)
 */

#ifndef SUPERP_UDP_PORT_NUM
#define SUPERP_UDP_PORT_NUM 4444
#endif

#ifndef IPPROTO_SUPERP
#define IPPROTO_SUPERP 253
#endif

/* Packet Delivery Layer Header */
struct superp_pdl_hdr {
	__le16 dcid;
	__le16 rwin;
	__le32 psn;
	__le32 ack_psn;
	__le32 sack_bitmap;
} __packed;

/* Transaction Layer Header */
struct superp_tal_hdr {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 eom :1;
	__u8 rsvd: 3;
	__u8 num_ops: 4;
#else
	__u8 num_ops: 4;
	__u8 rsvd: 3;
	__u8 eom :1;
#endif
	__u8 opcode;

	__le16 seqno;

	__le16 xid;
	__le16 ack_xid;
} __packed;

struct superp_pdl_tal_hdr {
	struct superp_pdl_hdr pdl;
	struct superp_tal_hdr tal;
} __packed;

/* SUPERp opcodes */
enum superp_opcode {
	SUPERP_OP_NOOP = 0,
	SUPERP_OP_LAST_NULL = 1,
	SUPERP_OP_TRANSACT_ERR = 2,
	SUPERP_OP_READ = 8,
	SUPERP_OP_WRITE = 9,
	SUPERP_OP_READ_RESP = 10,
	SUPERP_OP_SEND = 11,
	SUPERP_OP_SEND_TO_QP = 12,
};

/* Trannsaction error operation */
struct superp_op_transact_err {
	__le16 seqno;
	__u8 op_num;
	__u8 rsvd;
	__le16 major_err_code;
	__le16 minor_err_code;
} __packed;

/* Read operation */
struct superp_op_read {
	__le64 addr;
	__le32 len;
} __packed;

/* Write operation */
struct superp_op_write {
	__le64 addr;
} __packed;

/* Read response */
struct superp_op_read_resp {
	__le16 seqno;
	__u8 op_num;
	__u8 rsvd1;
	__le32 offset;
} __packed;

/* Send operation */
struct superp_op_send {
	__le32 key;
	__le32 offset;
} __packed;

/* Send to queue pair operation */
struct superp_op_send_to_qp {
	__u32 queue_pair: 24;
	__u32 rsvd: 8;
} __packed;

static inline const char *superp_opcode_text(enum superp_opcode opcode)
{
	switch (opcode) {
	case SUPERP_OP_NOOP:
		return "No-Op";
	case SUPERP_OP_LAST_NULL:
		return "Last NULL";
	case SUPERP_OP_TRANSACT_ERR:
		return "Transaction error";
	case SUPERP_OP_READ:
		return "Read";
	case SUPERP_OP_WRITE:
		return "Write";
	case SUPERP_OP_READ_RESP:
		return "Read response";
	case SUPERP_OP_SEND:
		return "Send";
	case SUPERP_OP_SEND_TO_QP:
		return "Send to Queue Pair";
	default:
		return "Invalid opcode";
	}
}

#endif /* __SUPERP_H__ */
