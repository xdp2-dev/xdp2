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

#ifndef __XDP2_UET_CONFIG_H__
#define __XDP2_UET_CONFIG_H__

/* UET configuration parameters */

#include "uet/pds.h"

#include "xdp2/config.h"
#include "xdp2/utility.h"

XDP2_CONFIG_MAKE_ALL_CONFIGS(
	(UET, uet_config, NULL, false),
	(
		(MAX_NUMBER_OF_PDCS, 1000, __u16, 1, 64535,
		 "Maximum number of PDCs",),

		/* PDS configuration parameters from the spec */
		(UET_OVER_UDP, 1, __u8, 0, 1, "Encapsulate UET in UDP",),
		(UDP_DEST_PORT, UET_UDP_PORT_NUM, __u16, 0, (1 << 16) - 1,
		 "UDP port number for UET",),
		(IP_PROTO_NXT_HDR, 0, __u8, 0, 255,
		 "IP protocol number when UET is encapsulated in IP",),
		(UET_DATA_PROTECT, 0, __u8, 0, 2,
		 "0=>no CRC or TSS, 1=>CRC enabled, 2=>TSS enabled",),
		(LIMIT_PSN_RANGE, 1, __u8, 0, 1,
		 "When set close PDC when PSN reaches start_psn + 2^31",),
		(DEFAULT_MPR, 8, __u8, 1, 255,
		 "Default MPR assumed when creating a PDC",),
		(MAX_ACK_DATA_SIZE, 16, __u16, 0, 8192,
		 "Maximum amount of return data that can be carried with a "
		 "PDS ACK sent in response to a PDS request",),
		(TRIMMABLE_ACK_SIZE, 10 * 1024, __u16, 0, 10 * 1024,
		 "ACK packets carrying read response data which are larger "
		 "than this size use a trimmable DSCP. Must be units of "
		 "sixteen bytes",),
		(ACK_ON_ECN, 1, __u8, 0, 1,
		 "If set the reception of a packet that is ECN marked "
		 "triggers an ACK generation",),
		(ENB_ACK_PER_PKT, 0, __u8, 0, 1,
		 "If set send an ACK per packet, else use coalesced ACKs",),
		(ACK_GEN_TRIGGER, 16 * 1024, __u16, 0, 32 * 1024,
		 "When ack_gen_count reaches this threshold, an ACK is "
		 "generated",),
		(ACK_GEN_MIN_PKT_ADD, 1024, __u16, 0, 2 * 1024,
		 "Minimum number of bytes added to ack_gen_count when a "
		 "packet is received at a PDC",),
		(RTO_INIT_TIMER, 1000000, __u64, 0, 8 * 1000000000ULL,
		 "Retransmit packet after this amount of time in nanoseconds "
		 "if no ACK or NACK is received",),
		(MAX_RTO_RETX_CNT, 5, __u8, 0, 15,
		 "Max number of retransmissions for a single packet before "
		 "declaring a failure event",),
		(NACK_RETX_TIME, 1000000, __u64, 0, 8ULL * 1000000000ULL,
		 "How long to delay the retransmission of a NACK’d packet",),
		(MAX_NACK_RETX_CNT, 5, __u8, 0, 31,
		 "Set a separate, possibly higher, threshold for "
		 "retransmissions based on NACK packets. Maximum setting "
		 "indicates infinite",),
		(NEW_PDC_TIME, 5000, __u32, 0, 100 * 1000,
		 "The time allowed in microseconds for a PDC initiator to "
		 "establish a PDC when TSS is enabled",),
		(PDS_CLEAR_TIME, 1000000, __u32, 0, 100 * 1000000,
		 "The time in nanoseconds allowed for a PDC initiator to "
		 "establish a PDC when TSS is enabled",),
		(CLOSE_REQ_TIME, 1000000, __u32, 0, 100 * 100000,
		 "The time in nanoseconds that an initiator is allowed to "
		 "take to respond to a request to close from the target",),
		(TAIL_LOSS_TIME, 1000000, __u32, 0, 100 * 100000,
		 "The time nanoseconds to detect tail loss",),
		(MAX_TAIL_LOSS_RETX, 5, __u8, 0, 15,
		 "Number of tail loss retransmits before connection is "
		 "closed",)
	)
)

#endif /* __XDP2_UET_CONFIG_H__ */
