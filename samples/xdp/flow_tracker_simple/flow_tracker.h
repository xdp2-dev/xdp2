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

/* Flow tracker definitions */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

#include "xdp2/bpf.h"
#include "xdp2/parser_metadata.h"

struct flowtuple {
	__be32 saddr;
	__be32 daddr;
	__be16 sport;
	__be16 dport;
	__u8 protocol;
};

struct bpf_elf_map SEC("maps") flowtracker = {
	.type = BPF_MAP_TYPE_HASH,
	.size_key = sizeof(struct flowtuple),
	.size_value = sizeof(__u64),
	.max_elem = 32,
	.pinning = PIN_GLOBAL_NS,
};

/* Lookup a flow in the flow tracker map and increment counter on a hit */
static __always_inline void flow_track(struct xdp2_metadata_all *frame)
{
	struct flowtuple ft = {};
	__u64 new_counter = 1;
	__u64 *counter;

	/* is packet TCP? */
	if (frame->ip_proto != 6)
		return;

	ft.saddr = frame->addrs.v4.saddr;
	ft.daddr = frame->addrs.v4.daddr;
	ft.sport = frame->src_port;
	ft.dport = frame->dst_port;
	ft.protocol = frame->ip_proto;

	/* Looking for packets from source port 22 */
	if (ft.sport != htons(22))
		return;

	counter = bpf_map_lookup_elem(&flowtracker, &ft);
	if (counter) {
		__sync_fetch_and_add(counter, 1);
	} else {
		/* New flow entry */
		bpf_map_update_elem(&flowtracker, &ft, &new_counter,
				    BPF_ANY);
	}
}
