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
#ifndef __SUE_H__
#define __SUE_H__

#include "xdp2/utility.h"

/* Structure definitions and functions for Scale Up Ethernet (SUE)  protocol */

#ifndef SUE_UDP_PORT_NUM
#define SUE_UDP_PORT_NUM 3333
#endif

struct sue_version_switch_header {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 ver: 2;
	__u8 rsvd: 6;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
	__u8 rsvd: 6;
	__u8 ver: 2;
#else
#error  "Please fix bitfield endianness"
#endif
};

/* SUE opcodes. The spec does not formally define the values */
enum sue_opcode {
	SUE_OPCODE_ACK = 0,
	SUE_OPCODE_NACK,
	SUE_OPCODE_INVALID1,
	SUE_OPCODE_INVALID2,
};

struct sue_reliability_hdr {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u16 ver: 2;
	__u16 op: 2;
	__u16 rsvd1: 2;
	__u16 xpuid: 10;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
	__u16 xpuid1: 2;
	__u16 rsvd1: 2;
	__u16 op: 2;
	__u16 ver: 2;
	__u16 xpuid2: 8;
#endif

	__be16 npsn;

#if defined(__BIG_ENDIAN_BITFIELD)
	__u16 vc: 2;
	__u16 rsvd2: 4;
	__u16 partition: 10;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
	__u16 partition1: 2;
	__u16 rsvd2: 4;
	__u16 vc: 2;
	__u16 partition2: 8;
#endif

	__be16 apsn;
};

static inline const char *sue_opcodeto_text(enum sue_opcode opcode)
{
	switch (opcode) {
	case SUE_OPCODE_ACK:
		return "ACK";
	case SUE_OPCODE_NACK:
		return "NACK";
	case SUE_OPCODE_INVALID1:
		return "Invalid1";
	case SUE_OPCODE_INVALID2:
		return "Invalid2";
	default:
		return "Opcode";
	}
}

/* Helper functions to get/set XPUID from bitfields in SUE RH */
static inline __u16 sue_get_xpuid(const void *vhdr)
{
	const struct sue_reliability_hdr *rhdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return rhdr->xpuid;
#else
	return (rhdr->xpuid1 << 8) + rhdr->xpuid2;
#endif
}

static inline void sue_set_xpuid(void *vhdr, __u16 xpuid)
{
	struct sue_reliability_hdr *rhdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	rhdr->xpuid = xpuid;
#else
	rhdr->xpuid1 = xpuid >> 2;
	rhdr->xpuid2 = xpuid & 0x3;
#endif
}

/* Helper functions to get/set XPUID from bitfields in SUE RH */
static inline __u16 sue_get_partition(const void *vhdr)
{
	const struct sue_reliability_hdr *rhdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return rhdr->partition;
#else
	return (rhdr->partition1 << 8) + rhdr->partition2;
#endif
}

static inline void sue_set_partition(void *vhdr, __u16 partition)
{
	struct sue_reliability_hdr *rhdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	rhdr->partition = partition;
#else
	rhdr->partition1 = partition >> 2;
	rhdr->partition2 = partition & 0x3;
#endif
}

#endif /* __SUE_H__ */
