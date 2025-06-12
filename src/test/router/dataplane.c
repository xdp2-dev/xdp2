// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2020 Tom Herbert
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

#include <linux/if_ether.h>
#include <linux/types.h>
#include <sys/types.h>

#include "xdp2/parser.h"
#include "xdp2/pkt_action.h"
#include "xdp2/proto_defs_define.h"
#include "xdp2/checksum.h"

#include "router.h"

/* Extract EtherType from Ethernet header */
static void extract_ether(const void *hdr, void *_frame,
			  const struct xdp2_ctrl_data ctrl)
{
	((struct router_metadata *)_frame)->ether_offset = ctrl.hdr.hdr_offset;
}

/* Extract EtherType from Ethernet header */
static void extract_ipv4(const void *hdr, void *_frame,
			 const struct xdp2_ctrl_data ctrl)
{
	struct router_metadata *frame = _frame;
	const struct iphdr *iph = hdr;

	frame->ip_offset = ctrl.hdr.hdr_offset;
	frame->ip_src_addr = iph->saddr;
}

static int handler_ipv4(const void *hdr, void *_frame,
			const struct xdp2_ctrl_data ctrl)
{
	const struct iphdr *iph = hdr;

	if (iph->version != 4)
		return XDP2_STOP_FAIL;

	if (xdp2_checksum_compute(hdr, ctrl.hdr.hdr_len) != 0xffff)
		return XDP2_STOP_FAIL;

	return XDP2_OKAY;
}

static int handler_okay(const void *hdr, void *_frame,
			const struct xdp2_ctrl_data ctrl)
{
	struct router_metadata *frame = _frame;
	struct ethhdr *eth = (struct ethhdr *)(ctrl.pkt.packet + frame->ether_offset);
	const struct next_hop *nh;

	nh = router_lpm_table_lookup(frame);

	if (nh) {
		memcpy(eth->h_dest, nh->edest, sizeof(eth->h_dest));
		xdp2_pkt_send(ctrl.pkt.packet, nh->port);
		printf("HIT port %u\n", nh->port);
		return XDP2_OKAY;
	}

	printf("MISS\n");

#if 0
	nh = router_lpm_skey_table_lookup_by_key(&frame->ip_src_addr);

	if (nh)
		printf("HIT2\n");
	else
		printf("MISS2\n");
#endif

	return XDP2_OKAY;
}

XDP2_MAKE_PARSE_NODE(ether_node_root, xdp2_parse_ether, ether_table,
		     (.ops.extract_metadata = extract_ether));

XDP2_MAKE_LEAF_PARSE_NODE(ipv4_node, xdp2_parse_ipv4,
		     (.ops.extract_metadata = extract_ipv4,
		      .ops.handler = handler_ipv4)
		    );

XDP2_MAKE_LEAF_PARSE_NODE(okay_node, xdp2_parse_null,
			  (.ops.handler = handler_okay));

XDP2_PARSER(simple_router, "Simple IPv4 router", ether_node_root,
	(.okay_node = &okay_node.pn,
	 .max_frames = 1,
	 .metameta_size = 0,
	 .frame_size = sizeof(struct router_metadata),
	)
);

XDP2_MAKE_PROTO_TABLE(ether_table,
	(__cpu_to_be16(ETH_P_IP), ipv4_node)
);

