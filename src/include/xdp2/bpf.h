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

#ifndef __XDP2_BPF_H__
#define __XDP2_BPF_H__

#include <stdlib.h>
#include <linux/bpf.h>

#include "xdp2/parser.h"

/* We use the tc-bpf map layout.
 * Maps are pinned in '/sys/fs/bpf/tc/globals'
 */

#define PIN_GLOBAL_NS 2

struct bpf_elf_map {
	__u32 type;
	__u32 size_key;
	__u32 size_value;
	__u32 max_elem;
	__u32 flags;
	__u32 id;
	__u32 pinning;
	__u32 inner_id;
	__u32 inner_idx;
};

/* Macros used by the compiler when targeting bpf */

#define xdp2_bpf_cast_ptr(ptr) ((unsigned char *)ptr)
#define xdp2_bpf_check_pkt(hdr, len, hdr_end)                                 \
	(xdp2_bpf_cast_ptr(hdr) + len > xdp2_bpf_cast_ptr(hdr_end))

/* Called by the compiler when generating code for TLVs
 * TCP TLVs encode the length as the size of the TLV header + size of the data
 */

__always_inline ssize_t xdp2_bpf_extract_tcpopt_sack(
	const struct xdp2_parse_tlv_node_ops *ops, const void *hdr,
	const void *hdr_end, void *frame, size_t tlv_len,
	struct xdp2_ctrl_data tlv_ctrl)
{
	if (ops->extract_metadata) {
		if (tlv_ctrl.hdr.hdr_len == 34) {
			if (xdp2_bpf_check_pkt(hdr, 34, hdr_end))
				return XDP2_STOP_TLV_LENGTH;
			ops->extract_metadata(hdr, frame, tlv_ctrl);
		} else if (tlv_ctrl.hdr.hdr_len == 26) {
			if (xdp2_bpf_check_pkt(hdr, 26, hdr_end))
				return XDP2_STOP_TLV_LENGTH;
			ops->extract_metadata(hdr, frame, tlv_ctrl);
		} else if (tlv_ctrl.hdr.hdr_len == 18) {
			if (xdp2_bpf_check_pkt(hdr, 18, hdr_end))
				return XDP2_STOP_TLV_LENGTH;
			ops->extract_metadata(hdr, frame, tlv_ctrl);
		} else if (tlv_ctrl.hdr.hdr_len == 10) {
			if (xdp2_bpf_check_pkt(hdr, 10, hdr_end))
				return XDP2_STOP_TLV_LENGTH;
			ops->extract_metadata(hdr, frame, tlv_ctrl);
		}
	}

	return XDP2_OKAY;
}

__always_inline ssize_t xdp2_bpf_extract_tcpopt_timestamp(
	const struct xdp2_parse_tlv_node_ops *ops, const void *hdr,
	const void *hdr_end, void *frame, struct xdp2_ctrl_data tlv_ctrl)
{
	if (ops->extract_metadata) {
		if (tlv_ctrl.hdr.hdr_len == 10) {
			if (xdp2_bpf_check_pkt(hdr, 10, hdr_end))
				return XDP2_STOP_TLV_LENGTH;
			ops->extract_metadata(hdr, frame, tlv_ctrl);
		}
	}

	return XDP2_OKAY;
}

__always_inline ssize_t xdp2_bpf_extract_tcpopt_window(
	const struct xdp2_parse_tlv_node_ops *ops, const void *hdr,
	const void *hdr_end, void *frame, struct xdp2_ctrl_data tlv_ctrl)
{
	if (ops->extract_metadata) {
		if (tlv_ctrl.hdr.hdr_len == 4) {
			if (xdp2_bpf_check_pkt(hdr, 4, hdr_end))
				return XDP2_STOP_TLV_LENGTH;
			ops->extract_metadata(hdr, frame, tlv_ctrl);
		}
	}

	return XDP2_OKAY;
}

__always_inline ssize_t xdp2_bpf_extract_tcpopt_mss(
	const struct xdp2_parse_tlv_node_ops *ops, const void *hdr,
	const void *hdr_end, void *frame, struct xdp2_ctrl_data tlv_ctrl)
{
	if (ops->extract_metadata) {
		if (tlv_ctrl.hdr.hdr_len == 3) {
			if (xdp2_bpf_check_pkt(hdr, 3, hdr_end))
				return XDP2_STOP_TLV_LENGTH;
			ops->extract_metadata(hdr, frame, tlv_ctrl);
		}
	}

	return XDP2_OKAY;
}

#endif /* __XDP2_BPF_H__ */
