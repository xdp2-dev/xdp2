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

#ifndef __PROTO_SUPERP_H__
#define __PROTO_SUPERP_H__

/* SUPERp protocol definitions */

#include "superp/superp.h"
#include "xdp2/parser.h"

/* MAcro to create operation specific length functions for TAL and
 * PDL with TAL
 */ 
#define SUPERP_TAL_MAKE_OP_LENGTH_FUNC(OP, STRUCT)			\
static inline ssize_t XDP2_JOIN3(superp_tal_with_, OP, _ops_len)(	\
				const void *vhdr, size_t maxlen)	\
{									\
	const struct superp_tal_hdr *thdr = vhdr;			\
									\
	return sizeof(*thdr) + (thdr->num_ops * sizeof(struct STRUCT));	\
}									\
									\
static inline ssize_t XDP2_JOIN3(superp_pdl_tal_with_, OP, _ops_len)(	\
				const void *vhdr, size_t maxlen)	\
{									\
	const struct superp_pdl_tal_hdr *pthdr = vhdr;			\
									\
	return sizeof(*pthdr) + (pthdr->tal.num_ops *			\
						sizeof(struct STRUCT));	\
}

/* Create operation specific length functions for various operation types */
SUPERP_TAL_MAKE_OP_LENGTH_FUNC(read, superp_op_read)
SUPERP_TAL_MAKE_OP_LENGTH_FUNC(write, superp_op_write)
SUPERP_TAL_MAKE_OP_LENGTH_FUNC(send, superp_op_send)
SUPERP_TAL_MAKE_OP_LENGTH_FUNC(send_to_qp, superp_op_send_to_qp)
SUPERP_TAL_MAKE_OP_LENGTH_FUNC(read_resp, superp_op_read_resp)
SUPERP_TAL_MAKE_OP_LENGTH_FUNC(transact_err, superp_op_transact_err)

static inline unsigned int superp_tal_num_els(const void *vhdr, size_t hlen)
{
	const struct superp_tal_hdr *thdr = vhdr;

	return thdr->num_ops;
}

static inline int superp_tal_el_type(const void *vhdr)
{
	return 0;
}

static inline size_t superp_tal_array_start_offset(const void *vhdr)
{
	return sizeof(struct superp_tal_hdr);
}

static inline int superp_tal_next_hdr(const void *vhdr)
{
	const struct superp_tal_hdr *thdr = vhdr;

	return thdr->opcode;
}

static inline unsigned int superp_pdl_tal_num_els(const void *vhdr, size_t hlen)
{
	const struct superp_pdl_tal_hdr *pthdr = vhdr;

	return pthdr->tal.num_ops;
}

static inline int superp_pdl_tal_el_type(const void *vhdr)
{
	return 0;
}

static inline size_t superp_pdl_tal_array_start_offset(const void *vhdr)
{
	return sizeof(struct superp_pdl_tal_hdr);
}

static inline int superp_pdl_tal_next_hdr(const void *vhdr)
{
	const struct superp_pdl_tal_hdr *thdr = vhdr;

	return thdr->tal.opcode;
}

#endif /* __PROTO_SUPERP_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* superp_parse_pdl definition
 *
 * Parse Packet Delivery Layer (PDL) header
 */
static const struct xdp2_proto_def superp_parse_pdl __unused() = {
	.name = "SUPERp Packet Delivery Layer header",
	.min_len = sizeof(struct superp_pdl_hdr),
};

/* superp_parse_tal definition
 *
 * Parse Transaction Layer (TAL) header
 */
static const struct xdp2_proto_def superp_parse_tal __unused() = {
	.name = "SUPERp Transaction Layer header",
	.min_len = sizeof(struct superp_tal_hdr),
	.ops.next_proto = superp_tal_next_hdr,
	.overlay = 1,
};

/* superp_parse_pdl_tal definition
 *
 * Parse Packet Delivery Layer and Transaction Layer as one header
 */
static const struct xdp2_proto_def superp_parse_pdl_tal __unused() = {
	.name = "SUPERp PDL and Transaction Layer header",
	.min_len = sizeof(struct superp_pdl_tal_hdr),
	.ops.next_proto = superp_pdl_tal_next_hdr,
	.overlay = 1,
};

/* Macro to create operation specific protocol definitions for TAL and
 * PDL with TAL
 *
 * Creates:
 * superp_parse_tal_with_OP_ops definition
 * superp_parse_pdl_pdl_tal_with_OP_ops definition
 */
#define SUPERP_MAKE_OP_ARRAY_PROTO_DEF(OP, STRUCT)			\
static const struct XDP2_JOIN3(xdp2_proto_def superp_parser_, OP, _op)	\
							__unused() = {	\
	.name = "SUPERp " #OP " operation",				\
	.min_len = sizeof(struct STRUCT),				\
};									\
									\
static const struct xdp2_proto_array_def				\
    XDP2_JOIN3(superp_parse_tal_with_, OP, _ops) __unused() = {		\
	.proto_def.name = "SUPERp Transaction Layer header "		\
			  "with " #OP " ops",				\
	.proto_def.min_len = sizeof(struct superp_tal_hdr),		\
	.proto_def.ops.len = XDP2_JOIN3(superp_tal_with_, OP, _ops_len),\
	.proto_def.node_type = XDP2_NODE_TYPE_ARRAY,			\
	.ops.el_type = superp_tal_el_type,				\
	.ops.num_els = superp_tal_num_els,				\
	.ops.start_offset = superp_tal_array_start_offset,		\
	.el_length = sizeof(struct STRUCT),				\
};									\
									\
static const struct xdp2_proto_array_def				\
	XDP2_JOIN3(superp_parse_pdl_tal_with_, OP, _ops) __unused() = {	\
	.proto_def.name = "SUPERp PDL and Transaction Layer header "	\
			  "with " #OP " ops",				\
	.proto_def.min_len = sizeof(struct superp_pdl_tal_hdr),		\
	.proto_def.ops.len = XDP2_JOIN3(superp_pdl_tal_with_, OP,	\
					_ops_len),			\
	.proto_def.node_type = XDP2_NODE_TYPE_ARRAY,			\
	.ops.el_type = superp_pdl_tal_el_type,				\
	.ops.num_els = superp_pdl_tal_num_els,				\
	.ops.start_offset = superp_pdl_tal_array_start_offset,		\
	.el_length = sizeof(struct STRUCT),				\
};

/* Create operation specific protocol definitions for the various
 * operations for TAL and PDL with TAL
 */
SUPERP_MAKE_OP_ARRAY_PROTO_DEF(read, superp_op_read)
SUPERP_MAKE_OP_ARRAY_PROTO_DEF(write, superp_op_write)
SUPERP_MAKE_OP_ARRAY_PROTO_DEF(send, superp_op_send)
SUPERP_MAKE_OP_ARRAY_PROTO_DEF(send_to_qp, superp_op_send_to_qp)
SUPERP_MAKE_OP_ARRAY_PROTO_DEF(read_resp, superp_op_read_resp)
SUPERP_MAKE_OP_ARRAY_PROTO_DEF(transact_err, superp_op_transact_err)

#endif /* XDP2_DEFINE_PARSE_NODE */
