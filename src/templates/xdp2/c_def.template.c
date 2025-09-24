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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "xdp2/parser.h"
#include "xdp2/proto_defs_define.h"
#include "xdp2/compiler_helpers.h"
#include "@!filename!@"

/* Template for making a plain C parser */

static inline __unused() __attribute__((always_inline)) int
	check_pkt_len(const void *hdr,
		const struct xdp2_proto_def *pnode, size_t len, ssize_t *hlen)
{
	*hlen = pnode->min_len;

	/* Protocol node length checks */
	if (len < *hlen)
		return XDP2_STOP_LENGTH;

	if (pnode->ops.len || pnode->ops.len_maxlen) {
		*hlen = pnode->ops.len_maxlen ?
				pnode->ops.len_maxlen(hdr, len) :
				pnode->ops.len(hdr);
		if (len < *hlen)
			return XDP2_STOP_LENGTH;
		if (*hlen < pnode->min_len)
			return *hlen < 0 ? *hlen : XDP2_STOP_LENGTH;
	} else {
		*hlen = pnode->min_len;
	}

	return XDP2_OKAY;
}

<!--(macro generate_xdp2_encap_layer)-->
static inline __unused() __attribute__((always_inline)) int
	@!parser_name!@_xdp2_encap_layer(
		struct xdp2_metadata_all *metadata,
		void **frame, unsigned *frame_num)
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

@!generate_xdp2_parse_tlv_function!@
<!--(for root in roots)-->
@!generate_xdp2_encap_layer(parser_name=root['parser_name'],frame_size=root['frame_size'],max_encaps=root['max_encaps'])!@
	<!--(for node in graph)-->
@!generate_protocol_parse_function_decl(parser_name=root['parser_name'],name=node)!@
	<!--(end)-->
	<!--(for node in graph)-->
@!generate_protocol_parse_function(parser_name=root['parser_name'],name=node)!@
	<!--(end)-->
@!generate_entry_parse_function(parser_name=root['parser_name'],root_name=root['node_name'],parser_add=root['parser_add'],parser_ext=root['parser_ext'],max_nodes=root['max_nodes'],max_frames=root['max_frames'],max_encaps=root['max_encaps'],metameta_size=root['metameta_size'],frame_size=root['frame_size'],num_counters=root['num_counters'],num_keys=root['num_keys'],okay_node=root['okay_node'],fail_node=root['fail_node'],atencap_node=root['atencap_node'])!@
<!--(end)-->
