<!--(if 0)-->
// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2020, 2021 by Mojatatu Networks.
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
<!--(end)-->

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include "xdp2/proto_defs_def.h"
#include "xdp2/bpf.h"

#include "xdp2/parser.h"
#include "xdp2/parser_metadata.h"
#include "@!filename!@"
#ifndef XDP2_LOOP_COUNT
#define XDP2_LOOP_COUNT 8
#endif

#define XDP2_MAX_ENCAPS (XDP2_LOOP_COUNT + 32)
enum {
<!--(for node in graph)-->
CODE_@!node!@,
<!--(end)-->
CODE_IGNORE
};

static __always_inline int check_pkt_len(const void *hdr, const void *hdr_end,
					 const struct xdp2_proto_def *pnode,
					 ssize_t* hlen)
{
	*hlen = pnode->min_len;

	/* Protocol node length checks */
	if (xdp2_bpf_check_pkt(hdr, *hlen, hdr_end))
		return XDP2_STOP_LENGTH;

	if (pnode->ops.len) {
		*hlen = pnode->ops.len(hdr);
		if (*hlen < 0)
			return XDP2_STOP_LENGTH;
		if (*hlen < pnode->min_len)
			return XDP2_STOP_LENGTH;
		if (xdp2_bpf_check_pkt(hdr, *hlen, hdr_end))
			return XDP2_STOP_LENGTH;
	} else {
		*hlen = pnode->min_len;
	}

	return XDP2_OKAY;
}

static __always_inline bool xdp2_encap_layer(struct xdp2_metadata *metadata,
					      void** frame, __u32* frame_num)
{
	/* New encapsulation layer. Check against
	 * number of encap layers allowed and also
	 * if we need a new metadata frame.
	 */
	if (++metadata->encaps > XDP2_MAX_ENCAPS)
		return XDP2_STOP_ENCAP_DEPTH;

	if (metadata->max_frame_num > *frame_num) {
		*frame += metadata->frame_size;
		*frame_num = (*frame_num) + 1;
	}

	return XDP2_OKAY;
}

static inline __attribute__((always_inline)) int xdp2_parse_tlv(
		const struct xdp2_parse_tlvs_node *parse_node,
		const struct xdp2_parse_tlv_node *parse_tlv_node,
		const __u8 *cp, const void *hdr_end, void *frame,
		struct xdp2_ctrl_data tlv_ctrl) {
	const struct xdp2_parse_tlv_node_ops *ops = &parse_tlv_node->tlv_ops;
	const struct xdp2_proto_tlv_def *proto_tlv_def =
					parse_tlv_node->proto_tlv_def;

	if (proto_tlv_def &&
	    (cp + proto_tlv_def->min_len > (const __u8 *)hdr_end)) {
		/* Treat check length error as an unrecognized TLV */
		if (parse_node->tlv_wildcard_node)
			return xdp2_parse_tlv(parse_node,
					parse_node->tlv_wildcard_node,
					cp, hdr_end, frame, tlv_ctrl);
		else
			return parse_node->unknown_tlv_type_ret;
	}

	if (ops->extract_metadata)
		ops->extract_metadata(cp, frame, tlv_ctrl);

	if (ops->handle_tlv)
		ops->handle_tlv(cp, frame, tlv_ctrl);

	return XDP2_OKAY;
}

<!--(macro generate_entry_parse_function)-->
static __always_inline int @!parser_name!@_xdp2_parse_@!root_name!@(
		struct xdp2_ctx *ctx, const void **hdr,
		const void *hdr_end, bool tailcall)
{
	void *frame = ctx->metadata.frame_data;
	const void *start_hdr = *hdr; /* XXXTH for comupting ctrl.hdr_offset. I suspect this doesn't work across tail calls */
	int ret = XDP2_OKAY;

	if (!tailcall)
		ret = __@!root_name!@_xdp2_parse(ctx, hdr, hdr_end, 0, frame);

	#pragma unroll
	for (int i = 0; i < (tailcall ? 1 : XDP2_LOOP_COUNT); i++) {
		if (ctx->next == CODE_IGNORE || ret != XDP2_OKAY)
			break;
		<!--(for node in graph)-->
		else if (ctx->next == CODE_@!node!@)
			<!--(if len(graph[node]['tlv_nodes']) == 0)-->
			ret = __@!node!@_xdp2_parse(ctx, hdr, hdr_end,
						     *hdr - start_hdr, frame);
			<!--(else)-->
			if (tailcall)
				ret = __@!node!@_xdp2_parse(ctx, hdr, hdr_end,
							     *hdr - start_hdr,
							     frame);
			else
				return XDP2_OKAY;
			<!--(end)-->
		<!--(end)-->
		else
			return XDP2_STOP_UNKNOWN_PROTO;
	}
	return ret;
}

static __always_inline int xdp2_xdp_parser_@!parser_name!@(
		struct xdp2_ctx *ictx, const void **hdr,
		const void *hdr_end, bool tailcall)
{
	return @!parser_name!@_xdp2_parse_@!root_name!@(ictx,
							hdr, hdr_end, tailcall);
}
<!--(end)-->

<!--(macro generate_protocol_tlvs_parse_function)-->
static inline __attribute__((always_inline)) int __@!name!@_xdp2_parse_tlvs(
	const struct xdp2_parse_node *parse_node, const void *hdr,
		const void *hdr_end, void *frame, struct xdp2_ctrl_data ctrl)
{
	const struct xdp2_parse_tlv_node_ops *ops; (void)ops;
	const struct xdp2_parse_tlvs_node* parse_tlvs_node =
				(const struct xdp2_parse_tlvs_node*)&@!name!@;
	const struct xdp2_proto_tlvs_def *proto_tlvs_def =
		(const struct xdp2_proto_tlvs_def*)parse_node->proto_def;

	const struct xdp2_parse_tlv_node *parse_tlv_node;
	const __u8 *cp = hdr;
	size_t offset, len;
	ssize_t tlv_len;
	int type;

	offset = proto_tlvs_def->ops.start_offset(hdr);
	/* Assume hlen marks end of TLVs */
	len = ctrl.hdr_len - offset;
	cp += offset;
	ctrl.hdr_offset += offset;

#pragma unroll
	for (int i = 0; i < 2; i++) {
		if (xdp2_bpf_check_pkt(cp, 1, hdr_end))
			return XDP2_STOP_LENGTH;
		if (proto_tlvs_def->pad1_enable &&
			*cp == proto_tlvs_def->pad1_val) {
			/* One byte padding, just advance */
			cp++;
			ctrl.hdr_offset++;
			len--;
			if (xdp2_bpf_check_pkt(cp, 1, hdr_end))
				return XDP2_STOP_LENGTH;
		}
		if (proto_tlvs_def->eol_enable &&
			proto_tlvs_def->eol_val) {
			cp++;
			ctrl.hdr_offset++;
			len--;
			break;
		}
		if (len < proto_tlvs_def->min_len)
			return XDP2_STOP_TLV_LENGTH;

		if (xdp2_bpf_check_pkt(cp, proto_tlvs_def->min_len, hdr_end))
			return XDP2_STOP_LENGTH;

		if (proto_tlvs_def->ops.len) {
			tlv_len = proto_tlvs_def->ops.len(cp);
			if (!tlv_len || len < tlv_len)
				return XDP2_STOP_TLV_LENGTH;
			if (tlv_len < proto_tlvs_def->min_len)
				return tlv_len < 0 ? tlv_len :
							XDP2_STOP_TLV_LENGTH;
		} else {
			tlv_len = proto_tlvs_def->min_len;
		}

		type = proto_tlvs_def->ops.type (cp);
		switch (type) {
	<!--(for tlv in graph[name]['tlv_nodes'])-->
		case @!tlv['type']!@:
		{
			int ret;
			struct xdp2_ctrl_data tlv_ctrl = { tlv_len,
							    ctrl.hdr_offset };
			parse_tlv_node = &@!tlv['name']!@;
			const struct xdp2_parse_tlv_node_ops *ops =
						&parse_tlv_node->tlv_ops;
			const struct xdp2_proto_tlv_def *proto_tlv_def =
					parse_tlv_node->proto_tlv_def;

			if (proto_tlv_def &&
			    (tlv_ctrl.hdr_len < proto_tlv_def->min_len)) {

		<!--(if len(tlv['overlay_nodes']) != 0)-->
				ops = &parse_tlv_node->tlv_ops;
		<!--(end)-->

				if ((ret = xdp2_parse_tlv(parse_tlvs_node,
							   parse_tlv_node, cp,
							   hdr_end, frame,
							   tlv_ctrl)) !=
								XDP2_OKAY)
					return ret;
		<!--(if len(tlv['overlay_nodes']) != 0)-->
				if (ops->overlay_type) {
					type = ops->overlay_type(cp);
					switch (type) {
			<!--(for overlay in tlv['overlay_nodes'])-->
					case @!overlay['type']!@:
						parse_tlv_node = &@!overlay['name']!@;
						if ((ret = xdp2_parse_tlv(
							    parse_tlvs_node,
							    parse_tlv_node, cp,
							    hdr_end, frame,
							    tlv_ctrl)) !=
								XDP2_OKAY)
							return ret;
						break;
			<!--(end)-->
					default:
						break;
					}
			} else {
					switch (tlv_ctrl.hdr_len) {
			<!--(for overlay in tlv['overlay_nodes'])-->
					case @!overlay['type']!@:
						tlv_ctrl.hdr_len =
							@!overlay['type']!@;
						parse_tlv_node =
							&@!overlay['name']!@;
						if ((ret = xdp2_parse_tlv(
							    parse_tlvs_node,
							    parse_tlv_node, cp,
							    hdr_end, frame,
							    tlv_ctrl)) !=
								XDP2_OKAY)
							return ret;
						break;
			<!--(end)-->
					default:
						break;
					}
				}
		<!--(end)-->
			}
			break;
		}
	<!--(end)-->
		default:
		{
			struct xdp2_ctrl_data tlv_ctrl = { tlv_len,
							    ctrl.hdr_offset };
			if (parse_tlvs_node->tlv_wildcard_node)
				return xdp2_parse_tlv(parse_tlvs_node,
					parse_tlvs_node->tlv_wildcard_node,
					cp, hdr_end, frame, tlv_ctrl);
			else if (parse_tlvs_node->unknown_tlv_type_ret !=
				 XDP2_OKAY)
				return parse_tlvs_node->unknown_tlv_type_ret;
		}
		}

		/* Move over current header */
		cp += tlv_len;
		ctrl.hdr_offset += tlv_len;
		len -= tlv_len;
	}
	return XDP2_OKAY;
}
<!--(end)-->

<!--(macro generate_protocol_parse_function_decl)-->
static int __always_inline __@!name!@_xdp2_parse(struct xdp2_ctx *ctx,
		const void **hdr, const void *hdr_end, size_t offset,
		void *frame) __attribute__((unused));
<!--(end)-->

<!--(macro generate_protocol_parse_function)-->
	<!--(if len(graph[name]['tlv_nodes']) != 0)-->
@!generate_protocol_tlvs_parse_function(name=name)!@
	<!--(end)-->
	<!--(if len(graph[name]['flag_fields_nodes']) != 0)-->
@!generate_protocol_fields_parse_function(name=name)!@
	<!--(end)-->
static int __always_inline __@!name!@_xdp2_parse(
		struct xdp2_ctx *ctx, const void **hdr,
		const void *hdr_end, size_t offset, void *frame)
{
	const struct xdp2_parse_node* parse_node =
				(const struct xdp2_parse_node*)&@!name!@;
	const struct xdp2_proto_def* proto_def = parse_node->proto_def;
	struct xdp2_ctrl_data ctrl;
	int ret, type;
	ssize_t hlen;

	ret = check_pkt_len(*hdr, hdr_end, parse_node->proto_def, &hlen);
	if (ret != XDP2_OKAY)
		return ret;

	ctrl.hdr_len = hlen;
	ctrl.hdr_offset = offset;

	if (parse_node->ops.extract_metadata)
		parse_node->ops.extract_metadata(*hdr, frame, ctrl);

	<!--(if len(graph[name]['tlv_nodes']) != 0)-->
	ret = __@!name!@_xdp2_parse_tlvs(parse_node, *hdr, hdr_end,
					  frame, ctrl);
	if (ret != XDP2_OKAY)
		return ret;
	<!--(end)-->

	if (proto_def->encap) {
		ret = xdp2_encap_layer(&ctx->metadata, &frame,
					&ctx->frame_num);
		if (ret != XDP2_OKAY)
			return ret;
	}

	<!--(if len(graph[name]['out_edges']) != 0)-->
	type = proto_def->ops.next_proto (*hdr);
	if (type < 0)
		return type;
	if (!proto_def->overlay)
		*hdr += hlen;

	switch (type) {
		<!--(for edge_target in graph[name]['out_edges'])-->
			<!--(for e in graph[name]['out_edges'][edge_target])-->
	case @!e['macro_name']!@:
		ctx->next = CODE_@!edge_target!@;
		return XDP2_OKAY;
			<!--(end)-->
		<!--(end)-->
	}
	/* Unknown protocol */
		<!--(if len(graph[name]['wildcard_proto_def']) != 0)-->
	return __@!graph[name]['wildcard_proto_def']!@_xdp2_parse(
		parser, hdr, len, offset, metadata, flags, max_encaps,
		frame, frame_num);
		<!--(else)-->
	return XDP2_STOP_UNKNOWN_PROTO;
		<!--(end)-->
	<!--(else)-->
	ctx->next = CODE_IGNORE;
	return XDP2_STOP_OKAY;
	<!--(end)-->
}
<!--(end)-->

<!--(for node in graph)-->
@!generate_protocol_parse_function_decl(name=node)!@
<!--(end)-->

<!--(for node in graph)-->
@!generate_protocol_parse_function(name=node)!@
<!--(end)-->

<!--(for parser_name,root_name,parser_add,parser_ext in roots)-->
@!generate_entry_parse_function(parser_name=parser_name,root_name=root_name,parser_add=parser_add,parser_ext=parser_ext)!@
<!--(end)-->
