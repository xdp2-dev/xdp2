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

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

#include "flow_tracker.h"
#include "xdp2/parser.h"
#include "parser.xdp.h"

#define PROG_MAP_ID 0xcafe

struct flow_tracker_ctx {
	struct xdp2_ctx ctx;
	struct xdp2_metadata_all frame[1];
};

struct bpf_elf_map SEC("maps") ctx_map = {
	.type = BPF_MAP_TYPE_PERCPU_ARRAY,
	.size_key = sizeof(__u32),
	.size_value = sizeof(struct flow_tracker_ctx),
	.max_elem = 2,
	.pinning = PIN_GLOBAL_NS,
};
struct bpf_elf_map SEC("maps") parsers = {
	.type = BPF_MAP_TYPE_PROG_ARRAY,
	.size_key = sizeof(__u32),
	.size_value = sizeof(__u32),
	.max_elem = 1,
	.pinning = PIN_GLOBAL_NS,
	.id = PROG_MAP_ID,
};

static __always_inline struct flow_tracker_ctx *xdp2_get_ctx(void)
{
	/* clang-10 has a bug if key == 0,
	 * it generates bogus bytecodes.
	 */
	__u32 key = 1;

	return bpf_map_lookup_elem(&ctx_map, &key);
}

SEC("0xcafe/0")
int parser_prog(struct xdp_md *ctx)
{
	struct flow_tracker_ctx *parser_ctx = xdp2_get_ctx();
	void *data_end = (void *)(long)ctx->data_end;
	void *data = (void *)(long)ctx->data;
	void *original = data;
	int rc = XDP2_OKAY;

	if (!parser_ctx)
		return XDP_ABORTED;

	/* >>> Invoke the specific xdp2 parser */
	rc = XDP2_PARSE_XDP(xdp2_parser_simple_tuple, &parser_ctx->ctx,
			     (const void **)&data, data_end, false);

	if (rc != XDP2_OKAY && rc != XDP2_STOP_OKAY) {
		bpf_xdp_adjust_head(ctx, -parser_ctx->ctx.offset);
		return XDP_PASS;
	}
	if (parser_ctx->ctx.next != CODE_IGNORE) {
		parser_ctx->ctx.offset += data - original;
		bpf_xdp_adjust_head(ctx, data - original);
		bpf_tail_call(ctx, &parsers, 0);
	}

	/* >>> Call processing user function here */
	flow_track(parser_ctx->frame);

	bpf_xdp_adjust_head(ctx, -parser_ctx->ctx.offset);
	return XDP_PASS;
}

SEC("prog")
int xdp_prog(struct xdp_md *ctx)
{
	struct flow_tracker_ctx *parser_ctx = xdp2_get_ctx();
	void *data_end = (void *)(long)ctx->data_end;
	void *data = (void *)(long)ctx->data;
	void *original = data;
	int rc = XDP2_OKAY;

	if (!parser_ctx)
		return XDP_ABORTED;

	parser_ctx->ctx.metadata.frame_size = sizeof(parser_ctx->frame[0]);
	parser_ctx->ctx.metadata.max_frame_num = 0;
	parser_ctx->ctx.frame_num = 0;
	parser_ctx->ctx.next = CODE_IGNORE;

	/* >>> Invoke the specific xdp2 parser */
	rc = XDP2_PARSE_XDP(xdp2_parser_simple_tuple, &parser_ctx->ctx,
			     (const void **)&data, data_end, false);

	if (rc != XDP2_OKAY && rc != XDP2_STOP_OKAY)
		return XDP_PASS;

	if (parser_ctx->ctx.next != CODE_IGNORE) {
		parser_ctx->ctx.offset = data - original;
		bpf_xdp_adjust_head(ctx, parser_ctx->ctx.offset);
		bpf_tail_call(ctx, &parsers, 0);
	}

	/* >>> Call processing user function here */
	flow_track(parser_ctx->frame);

	return XDP_PASS;
}

char __license[] SEC("license") = "GPL";
