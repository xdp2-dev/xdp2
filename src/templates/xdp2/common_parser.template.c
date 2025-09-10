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

/* Common parser template. This combined with another use case specific
 * template to make a complete template
 */

<!--(macro generate_entry_parse_function)-->

/* Parser entry function to commence parsing at the root */
static inline int @!parser_name!@_xdp2_parse_@!root_name!@(
		const struct xdp2_parser *parser,
		const struct xdp2_packet_data *pdata,
		void *metadata, unsigned int flags)
{
	struct xdp2_ctrl_data ctrl = { .var.ret_code = XDP2_STOP_OKAY };
	void *frame = metadata + parser->config.metameta_size;
	const struct xdp2_parse_node *parse_node;
	int ret;

	__XDP2_PARSE_INIT_CTRL(parser, ctrl);

	ctrl.pkt = *pdata;
	ctrl.var.metadata = metadata;

	<!--(for node in graph)-->
	(void)&__@!parser_name!@_@!node!@_xdp2_parse;
	<!--(end)-->

	ret = __@!parser_name!@_@!root_name!@_xdp2_parse(
			parser, pdata->packet, pdata->hdrs,
			pdata->hdrs_len, metadata, &frame, 0, &ctrl, flags);

	ctrl.var.ret_code = ret;

	parse_node = XDP2_CODE_IS_OKAY(ret) ?
			parser->config.okay_node : parser->config.fail_node;
	if (parse_node)
		__xdp2_parse_run_exit_node(parser, parse_node, metadata, frame,
					   &ctrl, flags);
	return ret;
}

/* Define optimized parser */
XDP2_PARSER_OPT(
	@!parser_name!@_opt,
	"",
	@!root_name!@,
	@!parser_name!@_xdp2_parse_@!root_name!@,
	(
		.max_nodes = @!max_nodes!@,
		.max_encaps = @!max_encaps!@,
		.max_frames = @!max_frames!@,
		.metameta_size = @!metameta_size!@,
		.frame_size = @!frame_size!@,
		.num_counters = @!num_counters!@,
		.num_keys = @!num_keys!@,
		<!--(if okay_node != '')-->
		.okay_node = &@!okay_node!@.pn,
		<!--(end)-->
		<!--(if fail_node != '')-->
		.fail_node = &@!fail_node!@.pn,
		<!--(end)-->
		<!--(if atencap_node != '')-->
		.atencap_node = &@!atencap_node!@.pn,
		<!--(end)-->
	)
    );
<!--(end)-->

<!--(macro generate_protocol_fields_parse_function)-->

/* Template for parsing flag fields */
static inline __attribute__((always_inline)) int
	__@!parser_name!@_@!name!@_xdp2_parse_flag_fields(
		const struct xdp2_parse_node *parse_node,
		void *_obj_ref, void *hdr, size_t hdr_len, void *_metadata,
		void *frame, struct xdp2_ctrl_data *ctrl,
		unsigned int flags)
{
	const struct xdp2_proto_flag_fields_def *proto_flag_fields_def;
	const struct xdp2_flag_field *flag_fields;
	const struct xdp2_flag_field *flag_field;
	struct xdp2_ctrl_data flags_ctrl = *ctrl;
	__u32 fflags, mask;
	size_t ioff;

	proto_flag_fields_def =
		(struct xdp2_proto_flag_fields_def *)parse_node->proto_def;
	flag_fields = proto_flag_fields_def->flag_fields->fields;
	fflags = proto_flag_fields_def->ops.get_flags(hdr);

	/* Position at start of field data */
	ioff = proto_flag_fields_def->ops.start_fields_offset(hdr);
	hdr += ioff;

	if (fflags) {
	<!--(for flag in graph[name]['flag_fields_nodes'])-->
		flag_field = &flag_fields[@!flag['index']!@];
		mask = flag_field->mask ? flag_field->mask : flag_field->flag;
		if ((fflags & mask) == flag_field->flag) {
			flags_ctrl.hdr.hdr_len = flag_fields->size;
                        flags_ctrl.hdr.hdr_offset = ioff;

			if (@!flag['name']!@.ops.extract_metadata)
				@!flag['name']!@.ops.extract_metadata(
					hdr, frame, flags_ctrl);
			if(@!flag['name']!@.ops.handler)
				@!flag['name']!@.ops.handler(
					hdr, frame, flags_ctrl);
			hdr += flag_fields->size;
			ioff += flag_fields->size;
		}
	<!--(end)-->
	}
	return XDP2_OKAY;
}
<!--(end)-->

<!--(macro generate_protocol_tlvs_parse_function)-->

/* Template for parsing TLVs */
static inline __attribute__((always_inline)) int
	__@!parser_name!@_@!name!@_xdp2_parse_tlvs(
		const struct xdp2_parse_node *parse_node,
		void* _obj_ref, void *hdr, size_t hdr_len,
		size_t pkt_len, void *_metadata, void *frame,
		struct xdp2_ctrl_data *ctrl, unsigned int flags)
{
	const struct xdp2_proto_tlvs_def *proto_tlvs_def =
		(const struct xdp2_proto_tlvs_def *)parse_node->proto_def;
	const struct xdp2_parse_tlvs_node *parse_tlvs_node =
		(const struct xdp2_parse_tlvs_node*)&@!name!@;
	const struct xdp2_parse_tlv_node *parse_tlv_node;
	const struct xdp2_parse_tlv_node_ops *ops;
	struct xdp2_ctrl_data tlv_ctrl = *ctrl;
	size_t hdr_offset = 0, len = hdr_len;
	ssize_t tlv_len;
	__u8 *cp = hdr;
	int type;

	(void)ops;

	hdr_offset = proto_tlvs_def->ops.start_offset(hdr);
	/* Assume hdr_len marks end of TLVs */
	len -= hdr_offset;
	cp += hdr_offset;

	while (len > 0) {
		if (proto_tlvs_def->pad1_enable &&
		    *cp == proto_tlvs_def->pad1_val) {
			/* One byte padding, just advance */
			cp++;
			hdr_offset++;
			len--;
			continue;
		}

		if (proto_tlvs_def->eol_enable &&
		    *cp == proto_tlvs_def->eol_val) {
			cp++;
			hdr_offset++;
			len--;
			break;
		}

		if (len < proto_tlvs_def->min_len)
			return XDP2_STOP_TLV_LENGTH;

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

		type = proto_tlvs_def->ops.type(cp);

		tlv_ctrl.hdr.hdr_offset = hdr_offset;
		tlv_ctrl.hdr.hdr_len = hdr_len;

		switch (type) {
	<!--(for tlv in graph[name]['tlv_nodes'])-->
		case @!tlv['type']!@:
		{
			int ret;

			parse_tlv_node = &@!tlv['name']!@;
		<!--(if len(tlv['overlay_nodes']) != 0)-->
			ops = &parse_tlv_node->tlv_ops;
		<!--(end)-->
			ret = xdp2_parse_tlv(parse_tlvs_node, parse_tlv_node,
					     _obj_ref, cp, _metadata, frame,
					     hdr_len, &tlv_ctrl, flags);
			if (ret != XDP2_OKAY)
				return ret;

			break;
		<!--(if len(tlv['overlay_nodes']) != 0)-->
			if (parse_tlv_node->proto_tlv_def->ops.overlay_type)
				type = parse_tlv_node->proto_tlv_def->
							ops.overlay_type(cp);
			else
				type = hdr_len;

			switch (type) {
			<!--(for overlay in tlv['overlay_nodes'])-->
			case @!overlay['type']!@:
				parse_tlv_node = &@!overlay['name']!@;
				ret = xdp2_parse_tlv(parse_tlvs_node,
						parse_tlv_node, cp, _obj_ref,
						_metadata, frame, hdr_len,
						&tlv_ctrl, flags);
				if (ret != XDP2_OKAY)
					return ret;
				break;
			<!--(end)-->
			default:
				break;
			 }

			break;
		<!--(end)-->
		}
	<!--(end)-->
		default:
		{
			/* struct xdp2_ctrl_data tlv_ctrl = */
			/* 			{ tlv_len, hdr_offset }; */

			if (parse_tlvs_node->tlv_wildcard_node)
				return xdp2_parse_tlv(
					parse_tlvs_node,
					parse_tlvs_node->tlv_wildcard_node,
					cp, _obj_ref, _metadata, frame,
					hdr_len, &tlv_ctrl, flags);
			else if (parse_tlvs_node->unknown_tlv_type_ret !=
							XDP2_OKAY)
				return parse_tlvs_node->unknown_tlv_type_ret;
		}
		}

		/* Move over current header */
		cp += tlv_len;
		hdr_offset += tlv_len;
		len -= tlv_len;
	}
	return XDP2_OKAY;
}
<!--(end)-->

<!--(macro generate_protocol_parse_function_decl)-->

/* Prototype for parse functions */
static inline int __@!parser_name!@_@!name!@_xdp2_parse(
		const struct xdp2_parser *parser,
		void *_obj_ref, void *hdr, size_t hdr_len,
		void *metadata, void **frame, unsigned int frame_num,
		struct xdp2_ctrl_data *ctrl, unsigned int flags);
<!--(end)-->

<!--(macro generate_protocol_parse_function)-->
	<!--(if len(graph[name]['tlv_nodes']) != 0)-->
@!generate_protocol_tlvs_parse_function(parser_name=parser_name,name=name)!@
	<!--(end)-->
	<!--(if len(graph[name]['flag_fields_nodes']) != 0)-->
@!generate_protocol_fields_parse_function(parser_name=parser_name,name=name)!@
	<!--(end)-->

/* Parse function */
static inline int __@!parser_name!@_@!name!@_xdp2_parse(
		const struct xdp2_parser *parser, void *_obj_ref,
		void *hdr, size_t hdr_len, void *metadata,
		void **frame, unsigned int frame_num,
		struct xdp2_ctrl_data *ctrl, unsigned int flags)
{
	const struct xdp2_parse_node *parse_node =
		(const struct xdp2_parse_node*)&@!name!@;
	const struct xdp2_proto_def *proto_def = parse_node->proto_def;
	ssize_t hlen;
	int ret;

	ctrl->var.last_node = parse_node;

	ret = check_pkt_len(hdr, parse_node->proto_def, hdr_len, &hlen);
	if (ret != XDP2_OKAY)
		return ret;

	ctrl->hdr.hdr_len = hdr_len;

	if (parse_node->ops.extract_metadata)
		parse_node->ops.extract_metadata(hdr, *frame, *ctrl);

	if (parse_node->ops.handler)
		parse_node->ops.handler(hdr, *frame, *ctrl);

	<!--(if len(graph[name]['tlv_nodes']) != 0)-->
	ret = __@!parser_name!@_@!name!@_xdp2_parse_tlvs(
			parse_node, _obj_ref, hdr, hlen, hdr_len,
			metadata, *frame, ctrl, flags);
	if (ret != XDP2_OKAY)
		return ret;
	<!--(end)-->

	<!--(if len(graph[name]['flag_fields_nodes']) != 0)-->
	ret = __@!parser_name!@_@!name!@_xdp2_parse_flag_fields(
			parse_node, _obj_ref, hdr, hdr_len, metadata,
			*frame, ctrl, flags);
	if (ret != XDP2_OKAY)
		return ret;
	<!--(end)-->

	if (proto_def->encap) {
		if (parser->config.atencap_node) {
			ret = __xdp2_parse_run_exit_node(parser,
				parser->config.atencap_node, metadata,
				*frame, ctrl, flags);
			if (ret != XDP2_OKAY)
				return ret;
		}

		/* New encapsulation leyer. Check against
		 * number of encap layers allowed and also
		 * if we need a new metadata frame.
		 */
		if (++ctrl->var.encaps > parser->config.max_encaps)
			return XDP2_STOP_ENCAP_DEPTH;

		if (parser->config.max_frames > frame_num) {
			(*frame) += parser->config.frame_size;
			frame_num++;
		}
	}

	<!--(if len(graph[name]['out_edges']) != 0)-->
	{
	int type = proto_def->ops.next_proto_keyin ?
		proto_def->ops.next_proto_keyin(hdr,
					ctrl->var.keys[parse_node->key_sel]) :
		proto_def->ops.next_proto(hdr);

	if (type < 0)
		return type;

	if (!proto_def->overlay) {
		hdr += hlen;
		hdr_len -= hlen;
		ctrl->hdr.hdr_offset += hlen;
	}

	switch (type) {
		<!--(for edge_target in graph[name]['out_edges'])-->
	case @!edge_target['macro_name']!@:
		return __@!parser_name!@_@!edge_target['target']!@_xdp2_parse(
			parser, _obj_ref, hdr, hdr_len, metadata,
			frame, frame_num, ctrl, flags);
		<!--(end)-->
	}
		<!--(if len(graph[name]['wildcard_proto_node']) != 0)-->
	return __@!parser_name!@_@!graph[name]['wildcard_proto_node']!@_xdp2_parse(
		parser, _obj_ref, hdr, hdr_len, metadata,
		frame, frame_num, ctrl, flags);
		<!--(else)-->
	return parse_node->unknown_ret;
		<!--(end)-->
	}
	<!--(else)-->

		<!--(if len(graph[name]['wildcard_proto_node']) != 0)-->

	if (!proto_def->overlay) {
		hdr += hlen;
		hdr_len -= hlen;
		ctrl->hdr.hdr_offset += hlen;
	}

	return __@!parser_name!@_@!graph[name]['wildcard_proto_node']!@_xdp2_parse(
		parser, _obj_ref, hdr, hdr_len, metadata,
		frame, frame_num, ctrl, flags);
		<!--(else)-->
	return XDP2_STOP_OKAY;
		<!--(end)-->
	<!--(end)-->
}
<!--(end)-->

<!--(macro generate_xdp2_parse_tlv_function)-->

/* Parse wildcard TLV */
static inline __attribute__((unused)) __attribute__((always_inline)) int
	xdp2_parse_wildcard_tlv(
		const struct xdp2_parse_tlvs_node *parse_node,
		const struct xdp2_parse_tlv_node *wildcard_parse_tlv_node,
		void *_obj_ref, void *hdr,
		void *_metadata, void *frame, size_t hdr_len,
		struct xdp2_ctrl_data *tlv_ctrl, unsigned int flags)
{
	const struct xdp2_parse_tlv_node_ops *ops =
					&wildcard_parse_tlv_node->tlv_ops;
	const struct xdp2_proto_tlv_def *proto_tlv_node =
					wildcard_parse_tlv_node->proto_tlv_def;

	if (proto_tlv_node && (hdr_len < proto_tlv_node->min_len))
		return parse_node->unknown_tlv_type_ret;

	if (ops->extract_metadata)
		ops->extract_metadata(hdr, frame, *tlv_ctrl);

	if (ops->handler)
		ops->handler(hdr, frame, *tlv_ctrl);

	return XDP2_OKAY;
}

/* Parse one TLV */
static inline __attribute__((unused)) __attribute__((always_inline))
	int xdp2_parse_tlv(
		const struct xdp2_parse_tlvs_node *parse_node,
		const struct xdp2_parse_tlv_node *parse_tlv_node,
		void *_obj_ref, void *hdr, void *_metadata, void *frame,
		size_t hdr_len, struct xdp2_ctrl_data *tlv_ctrl,
		unsigned int flags)
{
	const struct xdp2_parse_tlv_node_ops *ops = &parse_tlv_node->tlv_ops;
	const struct xdp2_proto_tlv_def *proto_tlv_node =
					parse_tlv_node->proto_tlv_def;

	if (proto_tlv_node && (hdr_len < proto_tlv_node->min_len)) {
		/* Treat check length error as an unrecognized TLV */
		if (parse_node->tlv_wildcard_node)
			return xdp2_parse_wildcard_tlv(parse_node,
					parse_node->tlv_wildcard_node,
					_obj_ref, hdr, _metadata, frame,
					hdr_len, tlv_ctrl, flags);
		else
			return parse_node->unknown_tlv_type_ret;
	}

	if (ops->extract_metadata)
		ops->extract_metadata(hdr, frame, *tlv_ctrl);

	if (ops->handler)
		ops->handler(hdr, frame, *tlv_ctrl);

	return XDP2_OKAY;
}

<!--(end)-->
