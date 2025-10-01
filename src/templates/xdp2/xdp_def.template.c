<!--(if 0)-->
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
<!--(end)-->

/* Template for XDP parser */

#include <bpf/bpf_helpers.h>
#include <linux/bpf.h>

#include "xdp2/bpf.h"
#include "xdp2/compiler_helpers.h"
#include "xdp2/parser.h"
#include "xdp2/parser_metadata.h"
#include "xdp2/proto_defs_define.h"

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

static __attribute__((unused)) __always_inline int
	check_pkt_len(const void *hdr, const void *hdr_end,
					 const struct xdp2_proto_def *pnode,
					 ssize_t *hlen)
{
	size_t len = (uintptr_t)(hdr_end - hdr);
	*hlen = pnode->min_len;

	/* Protocol node length checks */
	if (xdp2_bpf_check_pkt(hdr, *hlen, hdr_end))
		return XDP2_STOP_LENGTH;

	if (pnode->ops.len) {
		*hlen = pnode->ops.len(hdr, len);
		if (*hlen < 0)
			return XDP2_STOP_LENGTH;
		if (*hlen < pnode->min_len)
			return XDP2_STOP_LENGTH;
		if (xdp2_bpf_check_pkt(hdr, *hlen, hdr_end))
			return XDP2_STOP_LENGTH;
	}

	return XDP2_OKAY;
}

<!--(macro generate_xdp2_encap_layer)-->
static inline __attribute__((unused)) __attribute__((always_inline)) int
	@!parser_name!@_xdp2_encap_layer(struct xdp2_metadata_all *metadata,
					 void **frame, unsigned int *frame_num,
					 unsigned int flags)
{
	/* New encapsulation layer. Check against number of encap layers
	 * allowed and also if we need a new metadata frame.
	 */
	/* if (++metadata->num_encaps > @!max_encaps!@) */
	/*	return XDP2_STOP_ENCAP_DEPTH; */

	/* if (metadata->num_encaps > *frame_num) { */
	/*	*frame += @!frame_size!@; */
	/*	*frame_num = (*frame_num) + 1; */
	/* } */

	return XDP2_OKAY;
}
<!--(end)-->

/* Parse one TLV */
static inline __attribute__((unused)) __attribute__((always_inline)) int
	xdp2_parse_tlv(
		const struct xdp2_parse_tlvs_node *parse_node,
		const struct xdp2_parse_tlv_node *parse_tlv_node,
		const __u8 *cp, const void *hdr_end, size_t offset,
		void *_metadata, void *frame, struct xdp2_ctrl_data tlv_ctrl,
		unsigned int flags) {
	const struct xdp2_parse_tlv_node_ops *ops = &parse_tlv_node->tlv_ops;
	const struct xdp2_proto_tlv_def *proto_tlv_node =
					parse_tlv_node->proto_tlv_def;

	if (proto_tlv_node &&
	    (cp + proto_tlv_node->min_len > (const __u8 *)hdr_end)) {
		/* Treat check length error as an unrecognized TLV */
		if (parse_node->tlv_wildcard_node)
			return xdp2_parse_tlv(parse_node,
					      parse_node->tlv_wildcard_node,
					      cp, hdr_end, offset, _metadata,
					      frame, tlv_ctrl, flags);
		else
			return parse_node->unknown_tlv_type_ret;
	}

	ssize_t hlen;
	int ret = check_pkt_len(cp, hdr_end,
		(const struct xdp2_proto_def *)
			parse_tlv_node->proto_tlv_def, &hlen);
	if (ret != XDP2_OKAY)
		return ret;

	if (ops->extract_metadata)
		ops->extract_metadata(cp, hlen, offset, _metadata,
				      frame, &tlv_ctrl);

	if (ops->handler)
		ops->handler(cp, hlen, offset, _metadata, frame, &tlv_ctrl);

	return XDP2_OKAY;
}

<!--(macro generate_entry_parse_function)-->
/* Parse root function. Header passed as a ** since parse functions may
 * advance it
 */
static __attribute__((unused)) __always_inline int
	@!parser_name!@_xdp2_parse_@!root_name!@(
		struct xdp2_xdp_ctx *ctx, const void **hdr,
		const void *hdr_end, void *_metadata, bool tailcall,
		unsigned int flags)
{
	void *frame = (void *)_metadata;
	const void *start_hdr = *hdr;
	/* XXXTH for computing ctrl.hdr.hdr_offset. I suspect this doesn't
	 * work across tail calls
	 */
	int ret = XDP2_OKAY;

	if (!tailcall)
		ret = __@!root_name!@_xdp2_parse(ctx, hdr, hdr_end, _metadata,
						 0, frame, flags);

	#pragma unroll
	for (int i = 0; i < (tailcall ? 1 : XDP2_LOOP_COUNT); i++) {
		if (ctx->next == CODE_IGNORE || ret != XDP2_OKAY)
			break;
		<!--(for node in graph)-->
		else if (ctx->next == CODE_@!node!@)
			<!--(if len(graph[node]['tlv_nodes']) == 0)-->
			ret = __@!node!@_xdp2_parse(ctx, hdr, hdr_end,
					_metadata, *hdr - start_hdr, frame,
					flags);
			<!--(else)-->
			if (tailcall)
				ret = __@!node!@_xdp2_parse(ctx, hdr, hdr_end,
					_metadata, *hdr - start_hdr, frame,
					flags);
			else
				return XDP2_OKAY;
			<!--(end)-->
		<!--(end)-->
		else
			return XDP2_STOP_UNKNOWN_PROTO;
	}
	return ret;
}

/* Parse node function. Header passed as a ** since parse functions may
 * advance it
 */
static __attribute__((unused)) __always_inline int
	xdp2_xdp_parser_@!parser_name!@(
		struct xdp2_xdp_ctx *ictx, const void **hdr,
		const void *hdr_end, bool tailcall, unsigned int flags)
{
	return @!parser_name!@_xdp2_parse_@!root_name!@(
			ictx, hdr, hdr_end, ictx->metadata,
			tailcall, flags);
}
<!--(end)-->

<!--(macro generate_protocol_tlvs_parse_function)-->
/* Parse TLVs function */
static inline __attribute__((unused)) __attribute__((always_inline)) int
	__@!name!@_xdp2_parse_tlvs(
		const struct xdp2_parse_node *parse_node, const void *hdr,
		const void *hdr_end, size_t offset, void *_metadata,
		void *frame,
		struct xdp2_ctrl_data ctrl, unsigned int flags)
{
	const struct xdp2_parse_tlvs_node *parse_tlvs_node =
				(const struct xdp2_parse_tlvs_node*)&@!name!@;
	const struct xdp2_proto_tlvs_def *proto_tlvs_node =
		(const struct xdp2_proto_tlvs_def *)parse_node->proto_def;
	const struct xdp2_parse_tlv_node *parse_tlv_node;
	const struct xdp2_parse_tlv_node_ops *ops;
	const __u8 *cp = hdr;
	size_t offset, len;
	ssize_t tlv_len;
	int type;

	offset = proto_tlvs_node->ops.start_offset(hdr);
	/* Assume hlen marks end of TLVs */
	len = ctrl.hdr.hdr_len - offset;
	cp += offset;
	ctrl.hdr.hdr_offset += offset;

#pragma unroll
	for (int i = 0; i < 2; i++) {
		if (xdp2_bpf_check_pkt(cp, 1, hdr_end))
			return XDP2_STOP_LENGTH;
		if (proto_tlvs_node->pad1_enable &&
			*cp == proto_tlvs_node->pad1_val) {
			/* One byte padding, just advance */
			cp++;
			ctrl.hdr.hdr_offset++;
			len--;
			if (xdp2_bpf_check_pkt(cp, 1, hdr_end))
				return XDP2_STOP_LENGTH;
		}
		if (proto_tlvs_node->eol_enable &&
			proto_tlvs_node->eol_val) {
			cp++;
			ctrl.hdr.hdr_offset++;
			len--;
			break;
		}
		if (len < proto_tlvs_node->min_len)
			return XDP2_STOP_TLV_LENGTH;

		if (xdp2_bpf_check_pkt(cp, proto_tlvs_node->min_len, hdr_end))
			return XDP2_STOP_LENGTH;

		if (proto_tlvs_node->ops.len) {
			tlv_len = proto_tlvs_node->ops.len(cp);
			if (!tlv_len || len < tlv_len)
				return XDP2_STOP_TLV_LENGTH;
			if (tlv_len < proto_tlvs_node->min_len)
				return tlv_len < 0 ? tlv_len :
							XDP2_STOP_TLV_LENGTH;
		} else {
			tlv_len = proto_tlvs_node->min_len;
		}

		type = proto_tlvs_node->ops.type (cp);
		switch (type) {
	<!--(for tlv in graph[name]['tlv_nodes'])-->
		case @!tlv['type']!@:
		{
			int ret;

			parse_tlv_node = &@!tlv['name']!@;
			const struct xdp2_proto_tlv_def_ops *ops;
			const struct xdp2_proto_tlv_def *proto_tlv_node =
					parse_tlv_node->proto_tlv_def;

			if (proto_tlv_node &&
			    (tlv_ctrl.hdr.hdr_len < proto_tlv_node->min_len)) {

		<!--(if len(tlv['overlay_nodes']) != 0)-->
				ops = &proto_tlv_node->ops;
		<!--(end)-->

				if ((ret = xdp2_parse_tlv(parse_tlvs_node,
						parse_tlv_node, cp,
						hdr_end, offset, _metadata, frame,
						tlv_ctrl, flags)) != XDP2_OKAY)
					return ret;
		<!--(if len(tlv['overlay_nodes']) != 0)-->
				if (ops->overlay_type) {
					type = ops->overlay_type(cp);
					switch (type) {
			<!--(for overlay in tlv['overlay_nodes'])-->
					case @!overlay['type']!@:
						parse_tlv_node =
							&@!overlay['name']!@;
						if ((ret = xdp2_parse_tlv(
							    parse_tlvs_node,
							    parse_tlv_node, cp,
							    hdr_end, _metadata,
							    frame, tlv_ctrl,
							    flags)) !=
								XDP2_OKAY)
							return ret;
						break;
			<!--(end)-->
					default:
						break;
					}
			} else {
					switch (tlv_ctrl.hdr.hdr_len) {
			<!--(for overlay in tlv['overlay_nodes'])-->
					case @!overlay['type']!@:
						tlv_ctrl.hdr.hdr_len =
							@!overlay['type']!@;
						parse_tlv_node =
							&@!overlay['name']!@;
						if ((ret = xdp2_parse_tlv(
							    parse_tlvs_node,
							    parse_tlv_node, cp,
							    hdr_end,
							    _metadata, frame,
							    tlv_ctrl,
							    flags)) !=
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
			if (parse_tlvs_node->tlv_wildcard_node)
				return xdp2_parse_tlv(parse_tlvs_node,
					parse_tlvs_node->tlv_wildcard_node,
					cp, hdr_end, offset, _metadata, frame,
					tlv_ctrl, flags);
			else if (parse_tlvs_node->unknown_tlv_type_ret !=
				 XDP2_OKAY)
				return parse_tlvs_node->unknown_tlv_type_ret;
		}
		}

		/* Move over current header */
		cp += tlv_len;
		len -= tlv_len;
	}
	return XDP2_OKAY;
}
<!--(end)-->

<!--(macro generate_protocol_parse_function_decl)-->
static __attribute__((unused)) __always_inline int
	__@!name!@_xdp2_parse(struct xdp2_xdp_ctx *ctx,
		const void **hdr, const void *hdr_end, void *_metadata,
		size_t offset, void *frame, unsigned int flags)
						__attribute__((unused));
<!--(end)-->

<!--(macro generate_protocol_parse_function)-->
	<!--(if len(graph[name]['tlv_nodes']) != 0)-->
@!generate_protocol_tlvs_parse_function(name=name)!@
	<!--(end)-->
	<!--(if len(graph[name]['flag_fields_nodes']) != 0)-->
@!generate_protocol_fields_parse_function(name=name)!@
	<!--(end)-->
/*  Main parse function. Header passed as a ** since parse functions may
 * advance it
 */
static __attribute__((unused)) __always_inline int __@!name!@_xdp2_parse(
		struct xdp2_xdp_ctx *ctx, const void **hdr,
		const void *hdr_end, void *_metadata, size_t offset,
		void *frame, unsigned int flags)
{
	const struct xdp2_parse_node *parse_node =
				(const struct xdp2_parse_node*)&@!name!@;
	const struct xdp2_proto_def *proto_def = parse_node->proto_def;
	struct xdp2_ctrl_data ctrl;
	int ret, type;
	ssize_t hlen;

	ret = check_pkt_len(*hdr, hdr_end, parse_node->proto_def, &hlen);
	if (ret != XDP2_OKAY)
		return ret;

	if (parse_node->ops.extract_metadata)
		parse_node->ops.extract_metadata(*hdr, hlen, offset,
						 _metadata, frame, &ctrl);

	<!--(if len(graph[name]['tlv_nodes']) != 0)-->
	ret = __@!name!@_xdp2_parse_tlvs(parse_node, *hdr, hdr_end,
					  _metadata, frame, ctrl, flags);
	if (ret != XDP2_OKAY)
		return ret;
	<!--(end)-->

	if (proto_def->encap) {
		ret = @!parser_name!@_xdp2_encap_layer(_metadata, &frame,
					&ctx->frame_num, flags);
		if (ret != XDP2_OKAY)
			return ret;
	}

	<!--(if len(graph[name]['out_edges']) != 0)-->
	type = proto_def->ops.next_proto(*hdr);
	if (type < 0)
		return type;
	if (!proto_def->overlay)
		*hdr += hlen;

	switch (type) {
		<!--(for edge_target in graph[name]['out_edges'])-->
	case @!edge_target['macro_name']!@:
		ctx->next = CODE_@!edge_target['target']!@;
		return XDP2_OKAY;
		<!--(end)-->
	}
	/* Unknown protocol */
		<!--(if len(graph[name]['wildcard_proto_node']) != 0)-->
	return __@!graph[name]['wildcard_proto_node']!@_xdp2_parse(
		parser, hdr, len, offset, _metadata, flags, max_encaps,
		frame, frame_num, flags);
		<!--(else)-->
	return XDP2_STOP_UNKNOWN_PROTO;
		<!--(end)-->
	<!--(else)-->
	ctx->next = CODE_IGNORE;
	return XDP2_STOP_OKAY;
	<!--(end)-->
}
<!--(end)-->

<!--(for root in roots)-->
@!generate_xdp2_encap_layer(parser_name=root['parser_name'],frame_size=root['frame_size'],max_encaps=root['max_encaps'])!@
  <!--(for node in graph)-->
@!generate_protocol_parse_function_decl(name=node)!@
  <!--(end)-->

  <!--(for node in graph)-->
@!generate_protocol_parse_function(name=node,parser_name=root['parser_name'])!@
  <!--(end)-->

@!generate_entry_parse_function(parser_name=root['parser_name'],root_name=root['node_name'],parser_add=root['parser_add'],parser_ext=root['parser_ext'],max_nodes=root['max_nodes'],max_encaps=root['max_encaps'],metameta_size=root['metameta_size'],frame_size=root['frame_size'])!@
<!--(end)-->
