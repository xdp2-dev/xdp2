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

#ifndef __XDP2_PARSER_HASH_H__
#define __XDP2_PARSER_HASH_H__

/* Hash parser definitions
 *
 * Hash parser parses to extract port information from an IPv4 or IPv6
 * packet in a plane Ethernet frame. Encapsulation is not supported in
 * this parser.
 */
#include <linux/types.h>
#include <unistd.h>

#include "xdp2/parser_metadata.h"
#include "xdp2/utility.h"

#define XDP2_PARSER_SIMPLE_NUM_FRAMES 1

/* Meta data structure for multiple frames (i.e. to retrieve metadata
 * for multiple levels of encapsulation)
 */
#define XDP2_SIMPLE_HASH_START_FIELD_HASH eth_proto
struct xdp2_parser_simple_hash_metadata {
	XDP2_METADATA_addr_type;

	XDP2_METADATA_eth_proto __aligned(8);
	XDP2_METADATA_ip_proto;
	XDP2_METADATA_flow_label;
	XDP2_METADATA_ports;

	XDP2_METADATA_addrs; /* Must be last */
};

#define XDP2_SIMPLE_HASH_OFFSET_HASH				\
	offsetof(struct xdp2_parser_simple_hash_metadata,	\
		 XDP2_SIMPLE_HASH_START_FIELD_HASH)

/* Externs for simple hash parser */
XDP2_PARSER_EXTERN(xdp2_parser_simple_hash_ether);

/* Externs for optimized simple hash parser */
XDP2_PARSER_EXTERN(xdp2_parser_simple_hash_ether_opt);

/* Function to get hash from Ethernet packet */
static inline __u32 xdp2_parser_hash_hash_ether(void *p, size_t len, void *arg)
{
	struct xdp2_parser_simple_hash_metadata mdata;
	struct xdp2_packet_data pdata;

	XDP2_SET_BASIC_PDATA(pdata, p, len);

	if (xdp2_parse(xdp2_parser_simple_hash_ether, &pdata,
			&mdata, 0) != XDP2_STOP_OKAY)
		return 0;

	XDP2_HASH_CONSISTENTIFY(&mdata);

	return XDP2_COMMON_COMPUTE_HASH(&mdata,
					 XDP2_SIMPLE_HASH_START_FIELD_HASH);
}

#endif /* __XDP2_PARSER_HASH_H__ */
