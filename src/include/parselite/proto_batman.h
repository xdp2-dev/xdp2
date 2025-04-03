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

#ifndef __PROTO_BATMAN_H__
#define __PROTO_BATMAN_H__

#include <linux/if_ether.h>

/* BATMAN definitions */

#define BATADV_COMPAT_VERSION 15

enum batadv_packettype {
	/* 0x00 - 0x3f: local packets or special rules for handling */
	BATADV_IV_OGM           = 0x00,
	BATADV_BCAST            = 0x01,
	BATADV_CODED            = 0x02,
	BATADV_ELP		= 0x03,
	BATADV_OGM2		= 0x04,
	/* 0x40 - 0x7f: unicast */
#define BATADV_UNICAST_MIN     0x40
	BATADV_UNICAST          = 0x40,
	BATADV_UNICAST_FRAG     = 0x41,
	BATADV_UNICAST_4ADDR    = 0x42,
	BATADV_ICMP             = 0x43,
	BATADV_UNICAST_TVLV     = 0x44,
#define BATADV_UNICAST_MAX     0x7f
	/* 0x80 - 0xff: reserved */
};

struct batadv_unicast_packet {
	__u8 packet_type;
	__u8 version;
	__u8 ttl;
	__u8 ttvn; /* destination translation table version number */
	__u8 dest[ETH_ALEN];
	/* "4 bytes boundary + 2 bytes" long to make the payload after the
	 * following ethernet header again 4 bytes boundary aligned
	 */
};

struct batadv_eth {
	struct batadv_unicast_packet batadv_unicast;
	struct ethhdr eth;
};

#endif /* __PROTO_BATMAN_H__ */
