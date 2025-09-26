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

#ifndef __SUE_PARSER_TEST_H__
#define __SUE_PARSER_TEST_H__

/* Defines Scale Up Ethernet parser nodes for common parser test framework */

#include "sue/sue.h"

#define XDP2_DEFINE_PARSE_NODE
#include "sue/proto_sue.h"
#undef XDP2_DEFINE_PARSE_NODE

#include "xdp2/parser_test_helpers.h"

/* Parser definitions in parse_dump for scale up Ethernet */

XDP2_PTH_MAKE_SIMPLE_HANDLER(sue_base_node, "SUE base node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(sue_v0_node, "SUE version 0")

static void print_sue_reliability_header(const void *vhdr,
					 const struct xdp2_ctrl_data *ctrl)
{
	const struct sue_reliability_hdr *rhdr = vhdr;
	__u16 xpuid = sue_get_xpuid(rhdr);
	__u16 partition = sue_get_partition(rhdr);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tVersion: %u\n", rhdr->ver);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tOpcode: %s (%u)\n",
			     sue_opcodeto_text(rhdr->op), rhdr->op);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tXPUID: %u (0x%x)\n", xpuid, xpuid);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPSN: %u (%x)\n",
			     ntohs(rhdr->npsn), ntohs(rhdr->npsn));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tVirtual channel: %u (%x)\n",
			     rhdr->vc, rhdr->vc);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPartition: %u (%x)\n",
			     partition, partition);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tACK PSN: %u (%x)\n",
			     ntohs(rhdr->apsn), ntohs(rhdr->apsn));
}

static int handler_sue_rh(const void *hdr, void *frame,
			  const struct xdp2_ctrl_data *ctrl,
			  char *text)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSUE APSN is %s\n", text);

	if (verbose < 10)
		return 0;

	print_sue_reliability_header(hdr, ctrl);

	return 0;
}

XDP2_MAKE_PARSE_NODE(sue_base_node, sue_parse_version_ov,
		     sue_version_table,
		     (.ops.handler = handler_sue_base_node));

XDP2_MAKE_PARSE_NODE(sue_v0_node, sue_parse_opcode_ov, sue_opcode_table,
		     (.ops.handler = handler_sue_v0_node));

#define MAKE_SUE_OPCODE_NODE(NAME, TEXT)				\
static int handler_sue_rh_##NAME(const void *hdr, size_t hdr_len,	\
				 size_t hdr_off, void *metadata,	\
				 void *frame,				\
				 const struct xdp2_ctrl_data *ctrl)	\
{									\
	return handler_sue_rh(hdr, frame, ctrl, TEXT);			\
}									\
									\
XDP2_MAKE_LEAF_PARSE_NODE(sue_rh_##NAME, sue_parse_rh,			\
		(.ops.handler = handler_sue_rh_##NAME))

MAKE_SUE_OPCODE_NODE(ack, "ACK");
MAKE_SUE_OPCODE_NODE(nack, "NACK");
MAKE_SUE_OPCODE_NODE(invalid1, "Invalid1");
MAKE_SUE_OPCODE_NODE(invalid2, "Invalid2");

XDP2_MAKE_PROTO_TABLE(sue_version_table,
	( 0, sue_v0_node )
);

XDP2_MAKE_PROTO_TABLE(sue_opcode_table,
	( SUE_OPCODE_ACK, sue_rh_ack ),
	( SUE_OPCODE_NACK, sue_rh_nack ),
	( SUE_OPCODE_INVALID1, sue_rh_invalid1 ),
	( SUE_OPCODE_INVALID2, sue_rh_invalid2 )
);

#endif /* __SUE_PARSER_TEST_H__ */
