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

#ifndef __XDP2_TLVS_H__
#define __XDP2_TLVS_H__

/* Definitions and functions for processing and parsing TLVs */

#include <linux/types.h>

#ifndef __KERNEL__
#include <stddef.h>
#include <sys/types.h>
#endif

#include "xdp2/parser_types.h"
#include "xdp2/pmacro.h"
#include "xdp2/utility.h"

/* Definitions for parsing TLVs
 *
 * TLVs are a common protocol header structure consisting of Type, Length,
 * Value tuple (e.g. for handling TCP or IPv6 HBH options TLVs)
 */

/* Descriptor for parsing operations of one type of TLV. Fields are:
 *
 * len: Return length of a TLV. Must be set. If the return value < 0 (a
 *	XDP2_STOP_* return code value) this indicates an error and parsing
 *	is stopped. A the return value greater than or equal to zero then
 *	gives the protocol length. If the returned length is less than the
 *	minimum TLV option length, indicated by min_len by the TLV protocol
 *	node, then this considered and error.
 * type: Return the type of the TLV. If the return value is less than zero
 *	(XDP2_STOP_* value) then this indicates and error and parsing stops
 * start_offset: Return the start offset for TLV data
 * len_maxlen: Same as length function execept that the maximum length is
 *	input (i.e. the length of the encompassing protocol header)
 */
struct xdp2_proto_tlvs_opts {
	ssize_t (*len)(const void *hdr, size_t maxlen);
	int (*type)(const void *hdr);
	size_t (*start_offset)(const void *hdr);
};

/* TLV parse node operations
 *
 * Operations to process a sigle TLV parsenode
 *
 * extract_metadata: Extract metadata for the node. Input is the meta
 *	data frame which points to a parser defined metadata structure.
 *	If the value is NULL then no metadata is extracted
 * handler: Per TLV type handler which allows arbitrary processing
 *	of a TLV. Input is the TLV data and a parser defined metadata
 *	structure for the current frame. Return value is a parser
 *	return code: XDP2_OKAY indicates no errors, XDP2_STOP* return
 *	values indicate to stop parsing
 */
struct xdp2_parse_tlv_node_ops {
	void (*extract_metadata)(const void *hdr, size_t hdr_len,
				 size_t hdr_off, void *metadata, void *frame,
				 const struct xdp2_ctrl_data *ctrl);
	int (*handler)(const void *hdr, size_t hdr_len, size_t hdr_off,
		       void *metadata, void *frame,
		       const struct xdp2_ctrl_data *ctrl);
};

/* Parse node for a single TLV. Use common parse node operations
 * (extract_metadata and handle_proto)
 */
struct xdp2_parse_tlv_node {
	const struct xdp2_proto_tlv_def *proto_tlv_def;
	const struct xdp2_parse_tlv_node_ops tlv_ops;
	const struct xdp2_proto_tlvs_table *overlay_table;
	const struct xdp2_parse_tlv_node *overlay_wildcard_node;
	const struct xdp2_parse_node *nested_node;
	int unknown_overlay_ret;
	const char *name;
};

/* One entry in a TLV table:
 *	type: TLV type
 *	node: associated TLV parse structure for the type
 */
struct xdp2_proto_tlvs_table_entry {
	int type;
	const struct xdp2_parse_tlv_node *node;
};

/* TLV table
 *
 * Contains a table that maps a TLV type to a TLV parse node
 */
struct xdp2_proto_tlvs_table {
	int num_ents;
	const struct xdp2_proto_tlvs_table_entry *entries;
};

/* Parse node for parsing a protocol header that contains TLVs to be
 * parser:
 *
 * parse_node: Node for main protocol header (e.g. IPv6 node in case of HBH
 *	options) Note that node_type is set in parse_node to
 *	XDP2_NODE_TYPE_TLVS and that the parse node can then be cast to a
 *	parse_tlv_node
 * tlv_proto_table: Lookup table for TLV type
 * max_tlvs: Maximum number of TLVs that are to be parseed in one list
 * max_tlv_len: Maximum length allowed for any TLV in a list
 *	one type of TLVS.
 * unknown_tlv_type_ret: Error value to set if type is unknown
 * tlv_wildcard_node: Wildcard TLV node used if type is not found
 */
struct xdp2_parse_tlvs_node {
	const struct xdp2_parse_node pn;
	const struct xdp2_proto_tlvs_table *tlv_proto_table;
	size_t max_tlvs;
	size_t max_tlv_len;
	int unknown_tlv_type_ret;
	const struct xdp2_parse_tlv_node *tlv_wildcard_node;
};

/* A protocol definition for parsing proto with TLVs
 *
 * proto_def: Protocol defintion
 * ops: Operations for parsing TLVs
 * pad1_val: Type value indicating one byte of TLV padding (e.g. would be
 *	for IPv6 HBH TLVs)
 * pad1_enable: Pad1 value is used to detect single byte padding
 * eol_val: Type value that indicates end of TLV list
 * eol_enable: End of list value in eol_val is used
 * start_offset: When there TLVs start relative the enapsulating protocol
 *	(e.g. would be twenty for TCP)
 * min_len: Minimal length of a TLV option
 */
struct xdp2_proto_tlvs_def {
	struct xdp2_proto_def proto_def;
	struct xdp2_proto_tlvs_opts ops;
	__u8 pad1_val;
	__u8 eol_val;
	__u8 pad1_enable;
	__u8 eol_enable;
	size_t min_len;
};

/* TLV protocol definition operations
 *
 * overlay_type: Return the overlay type for a TLV (if not set for an overlay
 *	node then the TLV length is used
 * nested_offset: Starting offset of a nested TLV
 */
struct xdp2_proto_tlv_def_ops  {
	int (*overlay_type)(const void *hdr);
	size_t (*nested_offset)(const void *hdr, size_t maxlen);
};

/* A protocol definition for parsing proto with TLVs
 *
 * min_len: Minimal length of TLV
 * ops: TLV operations
 */
struct xdp2_proto_tlv_def {
	size_t min_len;
	struct xdp2_proto_tlv_def_ops ops;
};

static const struct xdp2_proto_tlv_def xdp2_parse_tlv_null __unused() = {};

/* Look up a TLV parse node given
 *
 * Arguments:
 *	- node: A TLVs parse node containing lookup table
 *	- type: TLV type to lookup
 *
 * Returns pointer to parse node if the protocol is matched else returns
 * NULL if the parse node isn't found
 */
const struct xdp2_parse_tlv_node *xdp2_parse_lookup_tlv(
				const struct xdp2_parse_tlvs_node *node,
				unsigned int type);

#define __XDP2_MAKE_TLV_TABLE_ONE_ENTRY1(KEY, NODE)			\
	{								\
		.type = KEY,						\
		.node = &NODE						\
	},

#define __XDP2_MAKE_TLV_TABLE_ONE_ENTRY(ENTRY)				\
	__XDP2_MAKE_TLV_TABLE_ONE_ENTRY1(				\
		XDP2_GET_POS_ARG2_1 ENTRY,				\
		XDP2_GET_POS_ARG2_2 ENTRY)

#define __XDP2_MAKE_TLV_TABLE_ENTRIES(...)				\
		XDP2_PMACRO_APPLY_ALL(					\
				__XDP2_MAKE_TLV_TABLE_ONE_ENTRY,	\
				__VA_ARGS__)

/* Helper to create a TLV protocol table */
#define XDP2_MAKE_TLV_TABLE(NAME, ...)					\
	static const struct xdp2_proto_tlvs_table_entry __##NAME[] =	\
		 { __XDP2_MAKE_TLV_TABLE_ENTRIES(__VA_ARGS__) };	\
	static const struct xdp2_proto_tlvs_table NAME = {		\
		.num_ents = sizeof(__##NAME) /				\
			sizeof(struct xdp2_proto_tlvs_table_entry),	\
		.entries = __##NAME,					\
	}

/* Forward declarations for TLV parser nodes */
#define XDP2_DECL_TLVS_PARSE_NODE(TLVS_PARSE_NODE)			\
	static const struct xdp2_parse_tlvs_node TLVS_PARSE_NODE

/* Forward declarations for TLV type tables */
#define XDP2_DECL_TLVS_TABLE(TLVS_TABLE)				\
	static const struct xdp2_proto_tlvs_table TLVS_TABLE

#define __XDP2_MAKE_TLVS_PARSE_NODE_OPT_ONE(OPT) .pn OPT,

#define __XDP2_MAKE_TLVS_PARSE_NODE_COMMON(PARSE_TLV_NODE,		\
					   PROTO_TLV_DEF,		\
					   TLV_TABLE, EXTRA_PN,		\
					   EXTRA_TLVS)			\
		.pn.text_name = #PARSE_TLV_NODE,			\
		.pn.node_type = XDP2_NODE_TYPE_TLVS,			\
		.pn.proto_def = &PROTO_TLV_DEF.proto_def,		\
		.tlv_proto_table = &TLV_TABLE,				\
									\
		.pn.unknown_ret = XDP2_STOP_UNKNOWN_PROTO,		\
		.unknown_tlv_type_ret = XDP2_OKAY,			\
		XDP2_PMACRO_APPLY_ALL(					\
			__XDP2_MAKE_TLVS_PARSE_NODE_OPT_ONE,		\
			XDP2_DEPAIR(EXTRA_PN))				\
		XDP2_DEPAIR(EXTRA_TLVS)

/* Helper to create a TLVs parse node with a next protocol */
#define XDP2_MAKE_TLVS_PARSE_NODE(PARSE_TLV_NODE, PROTO_TLV_DEF,	\
				  PROTO_TABLE, TLV_TABLE, EXTRA_PN,	\
				  EXTRA_TLVS)				\
	XDP2_DECL_TLVS_TABLE(TLV_TABLE);				\
	XDP2_DECL_PROTO_TABLE(PROTO_TABLE);				\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_tlvs_node PARSE_TLV_NODE = {	\
		.pn.proto_table = &PROTO_TABLE,				\
		__XDP2_MAKE_TLVS_PARSE_NODE_COMMON(PARSE_TLV_NODE,	\
			PROTO_TLV_DEF,	TLV_TABLE, EXTRA_PN, EXTRA_TLVS)\
	};								\
	XDP2_POP_NO_WEXTRA()

/* Helper to create TLVs parse node with an automatic next node */
#define XDP2_MAKE_TLVS_AUTONEXT_PARSE_NODE(PARSE_TLV_NODE,		\
					   PROTO_TLV_DEF,		\
					   NEXT_NODE, TLV_TABLE,	\
					   EXTRA_PM, EXTRA_TLVS)	\
	XDP2_DECL_TLVS_TABLE(TLV_TABLE);				\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_tlvs_node PARSE_TLV_NODE = {	\
		.pn.wildcard_node = NEXT_NODE,				\
		__XDP2_MAKE_TLVS_PARSE_NODE_COMMON(PARSE_TLV_NODE,	\
			PROTO_TLV_DEF,	TLV_TABLE, EXTRA_PN, EXTRA_TLVS)\
	}								\
	XDP2_POP_NO_WEXTRA()

/* Helper to create a leaf TLVs parse node */
#define XDP2_MAKE_LEAF_TLVS_PARSE_NODE(PARSE_TLV_NODE, PROTO_TLV_DEF,	\
				       TLV_TABLE, EXTRA_PN, EXTRA_TLVS)	\
	XDP2_DECL_TLVS_TABLE(TLV_TABLE);				\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_tlvs_node PARSE_TLV_NODE = {	\
		__XDP2_MAKE_TLVS_PARSE_NODE_COMMON(PARSE_TLV_NODE,	\
			PROTO_TLV_DEF,	TLV_TABLE, EXTRA_PN, EXTRA_TLVS)\
	};								\
	XDP2_POP_NO_WEXTRA()

#define __XDP2_MAKE_TLV_PARSE_NODE_COMMON(NODE_NAME, EXTRA)		\
		.name = #NODE_NAME,					\
		.unknown_overlay_ret = XDP2_OKAY,			\
		XDP2_DEPAIR(EXTRA)					\

/* Helper to create a TLV parse node for a single TLV */
#define XDP2_MAKE_TLV_PARSE_NODE(NODE_NAME, PROTO_TLV_DEF, EXTRA)	\
	XDP2_PUSH_NO_WEXTRA();						\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_tlv_node NODE_NAME = {		\
		.proto_tlv_def = &PROTO_TLV_DEF,			\
		__XDP2_MAKE_TLV_PARSE_NODE_COMMON(NODE_NAME, EXTRA)	\
	};								\
	XDP2_POP_NO_WEXTRA()

/* Helper to create an overlay TLV parse node for a single TLV */
#define XDP2_MAKE_TLV_OVERLAY_PARSE_NODE(NODE_NAME, PROTO_TLV_DEF,	\
					 OVERLAY_TABLE, EXTRA)		\
	XDP2_DECL_TLVS_TABLE(OVERLAY_TABLE);				\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_tlv_node NODE_NAME = {		\
		.overlay_table = &OVERLAY_TABLE,			\
		.proto_tlv_def = &PROTO_TLV_DEF,			\
		__XDP2_MAKE_TLV_PARSE_NODE_COMMON(NODE_NAME, EXTRA)	\
	};								\
	XDP2_POP_NO_WEXTRA()

/* Helper macro when accessing a TLV node in named parameters or elsewhere */
#define XDP2_TLV_NODE(NAME) &NAME

#endif /* __XDP2_TLVS_H__ */
