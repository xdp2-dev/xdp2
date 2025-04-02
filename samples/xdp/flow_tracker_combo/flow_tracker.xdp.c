// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
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

/* Simple xdp flow tracker for TCP. Parse packets to identify TCP flows.
 * Mainttain a table of flows where the key is the canonical TCP 5-tuple
 * and the value is a counter for the number of packets that were received
 * for the flow
 */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

#include "xdp2/bpf.h"
#include "xdp2/parser_metadata.h"
#include "xdp2/xdp_tmpl.h"

#include "parser.xdp.h"

/* Local context for processing packet in xdp */
struct flow_tracker_ctx {
	struct xdp2_xdp_ctx ctx;
	struct xdp2_metadata_all frame[1];
};

/* Flow table key, This is the canonical 5-tuple for TCP/IPv4 packet */
struct flowtuple {
	__be32 saddr;
	__be32 daddr;
	__be16 sport;
	__be16 dport;
	__u8 protocol;
};

/* Create the flow table. Key is a five tuple, value is the number of hits
 * for the flow
 */
struct bpf_elf_map SEC("maps") flowtracker = {
	.type = BPF_MAP_TYPE_HASH,
	.size_key = sizeof(struct flowtuple),
	.size_value = sizeof(__u64),
	.max_elem = 32,
	.pinning = PIN_GLOBAL_NS,
};

static __always_inline int flow_track(struct xdp_md *xdp_ctx,
				      struct flow_tracker_ctx *ctx)
{
	struct xdp2_metadata_all *frame = ctx->frame;
	struct flowtuple ft = {};
	__u64 new_counter = 1;
	__u64 *counter;

	/* Only work with IPv4 right now */
	if (ctx->frame[0].addr_type != XDP2_ADDR_TYPE_IPV4)
		return XDP_PASS;

	/* Is packet TCP? */
	if (ctx->frame[0].ip_proto != 6)
		return XDP_PASS;

	/* Set up flow entry */
	ft.saddr = frame[0].addrs.v4.saddr;
	ft.daddr = frame[0].addrs.v4.daddr;
	ft.sport = frame[0].src_port;
	ft.dport = frame[0].dst_port;
	ft.protocol = frame[0].ip_proto;

	/* Add flow to table */
	counter = bpf_map_lookup_elem(&flowtracker, &ft);
	if (counter) {
		/* Flow is already in the table, update the counter for number
		 * of hits
		 */
		__sync_fetch_and_add(counter, 1);
	} else {
		/* Create a new flow entry. Add the element to the flow
		 * table with initial count of 1
		 */
		bpf_map_update_elem(&flowtracker, &ft, &new_counter,
				    BPF_ANY);
	}

	return XDP_PASS;
}

static __always_inline int parser_fail(int rc, struct xdp_md *xdp_ctx,
				       struct flow_tracker_ctx *ctx)
{
	/* No special action for now */
	return XDP_PASS;
}

/* Create a standard XDP program to invoke the parser and call processing
 * functions
 */
XDP2_XDP_MAKE_PARSER_PROGRAM(xdp2_parser_simple_tuple,
			      struct flow_tracker_ctx,
			      sizeof(struct xdp2_metadata_all),
			      flow_track, parser_fail)

char __license[] SEC("license") = "GPL";
