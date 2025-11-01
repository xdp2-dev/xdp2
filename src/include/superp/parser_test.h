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

#ifndef __SUPERP_PARSER_TEST_H__
#define __SUPERP_PARSER_TEST_H__

/* Defines Scale Up EtheRnet protocol parser nodes for common parser test
 * framework
 */

#include "superp/superp.h"

#define XDP2_DEFINE_PARSE_NODE
#include "superp/proto_superp.h"
#undef XDP2_DEFINE_PARSE_NODE

#include "xdp2/parser_test_helpers.h"

/* Parser definitions in parse_dump for SUPERp */

static int handler_pdl(const void *hdr, size_t hdr_len,
		       size_t hdr_off, void *metadata,
		       void *frame,
		       const struct xdp2_ctrl_data *ctrl)
{
	const struct superp_pdl_hdr *pdl = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tPacket Delivery Layer (PDL)\n");

	if (verbose < 10)
		return 0;

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tDestination CID: "
				   "%u (0x%x)\n", pdl->dcid, pdl->dcid);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tReceive window: "
				   "%u (0x%x)\n", pdl->rwin, pdl->rwin);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPacket Sequence Number (PSN): "
				   "%u (0x%x)\n", pdl->psn, pdl->psn);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tACK Packet Sequence Number (ACK PSN): "
				   "%u (0x%x)\n", pdl->ack_psn, pdl->ack_psn);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSACK bitmap (SACK bitmap): "
				   "%u (0x%x)\n", pdl->sack_bitmap,
				  pdl->sack_bitmap);
	return 0;
}

static void extract_tal(const void *hdr, size_t hdr_len, size_t hdr_off,
			void *metadata, void *_frame,
			const struct xdp2_ctrl_data *ctrl)
{
	const struct superp_tal_hdr *tal = hdr;

	if (tal->num_ops) {
		/* Preferably we'd put the block size and blocks offset in
		 * metadata but we don't have access to the include file
		 * for the parser test that would have that. Just use
		 * keys for now, but in a real protocol implementation these
		 * could be in metadata
		 */
#if 0
		meta->superp_blocks_offset = hdr_off + hdr_len;
		meta->superp_block_size =
			(ctrl->pkt.pkt_len - (meta->superp_blocks_offset)) /
#else
		ctrl->key.keys[3] = hdr_off + hdr_len;
		ctrl->key.keys[4] =
			(ctrl->pkt.pkt_len - ctrl->key.keys[3]) / tal->num_ops;

#endif
	}
}

static int handler_tal(const void *hdr, size_t hdr_len,
		       size_t hdr_off, void *metadata,
		       void *frame,
		       const struct xdp2_ctrl_data *ctrl)
{
	const struct superp_tal_hdr *tal = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tTransaction Layer (TAL)\n");

	if (verbose < 10)
		return 0;

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tEnd of message: %s\n",
			     tal->eom ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tNum operations: "
				   "%u (0x%x)\n", tal->num_ops, tal->num_ops);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tOpcode: %s (%u)\n",
			     superp_opcode_text(tal->opcode), tal->opcode);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSequence Number (PSN): "
				   "%u (0x%x)\n", tal->seqno, tal->seqno);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tTransaction ID (XID): "
				   "%u (0x%x)\n", tal->xid, tal->xid);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tACK Transaction ID (ACK-XID): "
				   "%u (0x%x)\n", tal->ack_xid, tal->ack_xid);

	return 0;
}

static int handler_superp_read_op(const void *hdr, size_t hdr_len,
				  size_t hdr_off, void *metadata,
				  void *frame,
				  const struct xdp2_ctrl_data *ctrl)
{
	const struct superp_op_read *op = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tOperation #%u: READ\n",
				     ctrl->key.counters[3]++);

	if (verbose < 10)
		return 0;

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tAddress: %llx\n", op->addr);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tLength %u (0x%x)\n", op->len, op->len);

	return 0;
}

static int handler_superp_write_op(const void *hdr, size_t hdr_len,
				   size_t hdr_off, void *metadata,
				   void *frame,
				   const struct xdp2_ctrl_data *ctrl)
{
	const struct superp_op_read *op = hdr;
	size_t block_size = ctrl->key.keys[4];
	__u32 block_offset = ctrl->key.keys[3];
	__u8 index;

	index = ctrl->key.counters[3]++;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tOperation #%u: WRITE\n", index);

	if (verbose < 10)
		return 0;

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tAddress: %llx\n", op->addr);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tBlock size: %lu, Block offset: %lu\n",
			     block_size, block_offset + index * block_size);

	if (block_size) {
		__u8 first_data = *((__u8 *)(ctrl->pkt.packet + block_offset +
			(index * block_size)));

		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tFirst data %u (0x%x)\n",
				     first_data, first_data);
	}

	return 0;
}

static int handler_superp_read_resp(const void *hdr, size_t hdr_len,
				    size_t hdr_off, void *metadata,
				    void *frame,
				    const struct xdp2_ctrl_data *ctrl)
{
	const struct superp_op_read_resp *op = hdr;
	size_t block_size = ctrl->key.keys[4];
	__u32 block_offset = ctrl->key.keys[3];
	__u8 index;

	index = ctrl->key.counters[3]++;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tOperation #%u: READ RESPONSE\n",
				     index);

	if (verbose < 10)
		return 0;

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSeqno: %u (0x%x)\n",
			     op->seqno, op->seqno);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tOperation num: %u (0x%x)\n",
			     op->op_num, op->op_num);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tOffset: %u (0x%x)\n",
			     op->offset, op->offset);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tBlock size: %lu, Block offset: %lu\n",
			     block_size, block_offset + index * block_size);

	if (block_size) {
		__u8 first_data = *((__u8 *)(ctrl->pkt.packet + block_offset +
			(index * block_size)));

		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tFirst data %u (0x%x)\n",
				     first_data, first_data);
	}

	return 0;
}

static int handler_superp_send_op(const void *hdr, size_t hdr_len,
				  size_t hdr_off, void *metadata,
				  void *frame,
				  const struct xdp2_ctrl_data *ctrl)
{
	const struct superp_op_send *op = hdr;
	size_t block_size = ctrl->key.keys[4];
	__u32 block_offset = ctrl->key.keys[3];
	__u8 index;

	index = ctrl->key.counters[3]++;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tOperation #%u: SEND\n",
				     index);

	if (verbose < 10)
		return 0;

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tKey: %u (0x%x)\n", op->key, op->key);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tOffset: %u (0x%x)\n",
			     op->offset, op->offset);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tBlock size: %lu, Block offset: %lu\n",
			     block_size, block_offset + index * block_size);

	if (block_size) {
		__u8 first_data = *((__u8 *)(ctrl->pkt.packet + block_offset +
			(index * block_size)));

		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tFirst data %u (0x%x)\n",
				     first_data, first_data);
	}

	return 0;
}

static int handler_superp_send_to_qp_op(const void *hdr, size_t hdr_len,
					size_t hdr_off, void *metadata,
					void *frame,
					const struct xdp2_ctrl_data *ctrl)
{
	const struct superp_op_send_to_qp *op = hdr;
	size_t block_size = ctrl->key.keys[4];
	__u32 block_offset = ctrl->key.keys[3];
	__u8 index;

	index = ctrl->key.counters[3]++;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tOperation #%u: SEND TO QP\n",
				     index);

	if (verbose < 10)
		return 0;

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tQueue pair %u (0x%x)\n",
			     op->queue_pair, op->queue_pair);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tBlock size: %lu, Block offset: %lu\n",
			     block_size, block_offset + index * block_size);

	if (block_size) {
		__u8 first_data = *((__u8 *)(ctrl->pkt.packet + block_offset +
			(index * block_size)));

		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tFirst data %u (0x%x)\n",
				     first_data, first_data);
	}

	return 0;
}

static int handler_superp_transact_err(const void *hdr, size_t hdr_len,
				       size_t hdr_off, void *metadata,
				       void *frame,
				       const struct xdp2_ctrl_data *ctrl)
{
	const struct superp_op_transact_err *op = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tOperation #%u: "
					   "TRANSACTION ERROR\n",
				     ctrl->key.counters[3]++);

	if (verbose < 10)
		return 0;

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSeqno: %u (0x%x)\n",
			     op->seqno, op->seqno);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tOperation num: %u (0x%x)\n",
			     op->op_num, op->op_num);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMajor error code: %u (0x%x)\n",
			     op->major_err_code, op->major_err_code);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMinor error code: %u (0x%x)\n",
			     op->minor_err_code, op->minor_err_code);

	return 0;
}

XDP2_MAKE_LEAF_PARSE_NODE(superp_no_op_node, xdp2_parse_null, ());

XDP2_MAKE_ARREL_PARSE_NODE(superp_op_arrel_read_node,
			   (.ops.handler = handler_superp_read_op));
XDP2_MAKE_LEAF_ARRAY_PARSE_NODE_NOTAB(superp_tal_read_node,
		superp_parse_tal_with_read_ops,
		(.ops.extract_metadata = extract_tal),
		(.array_wildcard_node =
		 XDP2_ARREL_NODE(superp_op_arrel_read_node)));

XDP2_MAKE_ARREL_PARSE_NODE(superp_op_arrel_write_node,
			   (.ops.handler = handler_superp_write_op));
XDP2_MAKE_LEAF_ARRAY_PARSE_NODE_NOTAB(superp_tal_write_node,
		superp_parse_tal_with_write_ops,
		(.ops.extract_metadata = extract_tal),
		(.array_wildcard_node =
		 XDP2_ARREL_NODE(superp_op_arrel_write_node)));

XDP2_MAKE_ARREL_PARSE_NODE(superp_op_arrel_read_resp_node,
			   (.ops.handler = handler_superp_read_resp));
XDP2_MAKE_LEAF_ARRAY_PARSE_NODE_NOTAB(superp_tal_read_resp_node,
		superp_parse_tal_with_read_resp_ops,
		(.ops.extract_metadata = extract_tal),
		(.array_wildcard_node =
		 XDP2_ARREL_NODE(superp_op_arrel_read_resp_node)));

XDP2_MAKE_ARREL_PARSE_NODE(superp_op_arrel_send_node,
			   (.ops.handler = handler_superp_send_op));
XDP2_MAKE_LEAF_ARRAY_PARSE_NODE_NOTAB(superp_tal_send_node,
		superp_parse_tal_with_send_ops,
		(.ops.extract_metadata = extract_tal),
		(.array_wildcard_node =
		 XDP2_ARREL_NODE(superp_op_arrel_send_node)));

XDP2_MAKE_ARREL_PARSE_NODE(superp_op_arrel_send_to_qp_node,
			   (.ops.handler = handler_superp_send_to_qp_op));
XDP2_MAKE_LEAF_ARRAY_PARSE_NODE_NOTAB(superp_tal_send_to_qp_node,
		superp_parse_tal_with_send_to_qp_ops,
		(.ops.extract_metadata = extract_tal),
		(.array_wildcard_node =
		 XDP2_ARREL_NODE(superp_op_arrel_send_to_qp_node)));

XDP2_MAKE_ARREL_PARSE_NODE(superp_op_arrel_transact_err_node,
			   (.ops.handler = handler_superp_transact_err));
XDP2_MAKE_LEAF_ARRAY_PARSE_NODE_NOTAB(superp_tal_transact_err_node,
		superp_parse_tal_with_transact_err_ops,
		(.ops.extract_metadata = extract_tal),
		(.array_wildcard_node =
		 XDP2_ARREL_NODE(superp_op_arrel_transact_err_node)));

XDP2_MAKE_PARSE_NODE(superp_tal_node, superp_parse_tal, superp_tal_op_table,
		     (.ops.handler = handler_tal));

XDP2_MAKE_AUTONEXT_PARSE_NODE(superp_pdl_node, superp_parse_pdl,
			      superp_tal_node,
			      (.ops.handler = handler_pdl));

XDP2_MAKE_PROTO_TABLE(superp_tal_op_table,
	( SUPERP_OP_NOOP, superp_no_op_node),
	( SUPERP_OP_LAST_NULL, superp_no_op_node),
	( SUPERP_OP_TRANSACT_ERR, superp_tal_transact_err_node),
	( SUPERP_OP_READ, superp_tal_read_node),
	( SUPERP_OP_WRITE, superp_tal_write_node),
	( SUPERP_OP_READ_RESP, superp_tal_read_resp_node),
	( SUPERP_OP_SEND, superp_tal_send_node),
	( SUPERP_OP_SEND_TO_QP, superp_tal_send_to_qp_node)
);

#endif /* __SUPERP_PARSER_TEST_H__ */
