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

#ifndef __XDP2_PROTO_PROTOBUF_H__
#define __XDP2_PROTO_PROTOBUF_H__

#include <linux/udp.h>

/* Generic UDP protocol definitions */

enum {
	XDP2_PROTOBUF_TYPE_VARINT,
	XDP2_PROTOBUF_TYPE_I64,
	XDP2_PROTOBUF_TYPE_LEN,
	XDP2_PROTOBUF_TYPE_I32,
};

static inline ssize_t parse_varint(__u64 *value, const void *v,
				   size_t max_len)
{
	const __u8 *ch = v;
	unsigned int shift;
	__u64 result = 0;
	int i;

	for (shift = 0, i = 0; i < 11; i++, shift += 7) {
		if (max_len && i >= max_len)
			return XDP2_STOP_TLV_LENGTH;

		result += (ch[i] & ~0x80) << shift;

		if (!(ch[i] & 0x80)) {
			*value = result;
			return i + 1;
		}
	}
	*value = 0;
	return XDP2_STOP_TLV_LENGTH;
}

static inline ssize_t parse_string(const void *v, const void **ptr,
				   size_t *str_len, size_t max_len)
{
	__u64 value;
	ssize_t len;

	len = parse_varint(&value, v, max_len);
	if (len < 0)
		return len;

	if (ptr)
		*ptr = v + len;

	if (str_len)
		*str_len = value;

	len += value;

	return (!max_len || len <= max_len) ? len : XDP2_STOP_TLV_LENGTH;
}
static inline ssize_t protobuf_len(const void *v, size_t max_len)
{
	ssize_t tl_len, v_len;
	__u64 value;
	__u8 type;

	tl_len = parse_varint(&value, v, max_len);
	if (tl_len < 0)
		return tl_len;

	type = value & 0x7;
	v += tl_len;
	max_len -= tl_len;

	switch (type) {
	case XDP2_PROTOBUF_TYPE_VARINT:
		v_len = parse_varint(&value, v, max_len);
		if (v_len < 0)
			return v_len;
		break;
	case XDP2_PROTOBUF_TYPE_LEN:
		v_len = parse_string(v, NULL, NULL, max_len);
		if (v_len < 0)
			return v_len;
		break;
	default:
		return XDP2_STOP_UNKNOWN_TLV;
	}

	return tl_len + v_len;
}

static inline ssize_t protobuf_top_len(const void *v, size_t max_len)
{
	size_t omax_len = max_len;
	ssize_t len;

	while (max_len > 0) {
		len = protobuf_len(v, max_len);
		if (len < 0)
			return len;

		v += len;
		max_len -= len;
	}

	return omax_len;
}

static inline int protobuf_type(const void *v)
{
	__u64 value;

	parse_varint(&value, v, 0);

	return value >> 3;
}

static inline size_t protobuf_start_offset(const void *hdr)
{
	return 0;
}

static inline size_t protobuf_nested_offset(const void *hdr,
					    size_t len)
{
	__u64 value;
	size_t tl_len, v_len;

	tl_len = parse_varint(&value, hdr, 0);
	v_len = parse_varint(&value, hdr + tl_len, 0);

	return tl_len + v_len;
}

#endif /* __XDP2_PROTO_PROTOBUF_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* xdp2_parse_udp protocol definition
 *
 * Parse UDP header
 */
static const struct xdp2_proto_tlvs_def xdp2_parse_protobufs __unused() = {
	.proto_def.node_type = XDP2_NODE_TYPE_TLVS,
	.proto_def.name = "Parse protobufs",
	.proto_def.ops.len = protobuf_top_len,
	.ops.start_offset = protobuf_start_offset,
	.ops.len = protobuf_len,
	.ops.type = protobuf_type,
};

static const struct xdp2_proto_tlv_def xdp2_parse_nested_protobuf
								__unused() = {
	.ops.nested_offset = protobuf_nested_offset,
};
#endif /* XDP2_DEFINE_PARSE_NODE */
