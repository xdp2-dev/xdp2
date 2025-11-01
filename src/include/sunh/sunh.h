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
#ifndef __SUNH_H__
#define __SUNH_H__

#include "xdp2/utility.h"

/* Scale Up Network header (SUNH) */

#ifndef ETH_P_SUNH
#define ETH_P_SUNH 0x885b
#endif

/* The flow label is a twelve bit value that is split over a byte bounday
 * so there are extra steps to set and get it. Helper functions for that are
 * below. An alterncative is to get and set it via the hoplim_flow_label
 * field. This a sixteen bit field that can be set in one shot. Combining the
 * Hoplimit with the Flow Label works since the Hop Limit can effectively
 * be treated as part of the Flow Label (it should be consistent over packets
 * for a flow
 *
 * To create a tuple has from the header, the traffic class can be or'ed
 * with 0xff and then the hash function can be run on the full SUNH header
 */
struct sunh_hdr {
	union {
		__u8 traffic_class;
		struct {
#if defined(__BIG_ENDIAN_BITFIELD)
			__u8 diff_serv: 5;
			__u8 ecn: 3;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
			__u8 ecn: 3;
			__u8 diff_serv: 5;
#else
#error  "Please fix bitfield endianness"
#endif
		};
	};

	__u8 next_header;

	union {
		__be16 hoplim_flow_label;
		struct {
#if defined(__BIG_ENDIAN_BITFIELD)
			__u16 hop_limit: 4;
			__u16 flow_label: 12;
#else
			__u16 flow_label_high: 4;
			__u16 hop_limit: 4;
			__u16 flow_label_low: 8;
#endif
		};
	};
	__be16 saddr;
	__be16 daddr;
} __packed;

/* Helper functions to get/set flow label from bitfields */
static inline __u16 sunh_get_flow_label(const void *vhdr)
{
	const struct sunh_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return hdr->flow_label;
#else
	return (hdr->flow_label_high << 8) + (hdr->flow_label_low & 0xff);
#endif
}

static inline void sunh_set_flow_label(void *vhdr, __u16 val)
{
	struct sunh_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	hdr->flow_label = val;
#else
	hdr->flow_label_high = val >> 8;
	hdr->flow_label_low = val & 0xff;
#endif
}

#endif /* __SUN_H__ */
