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

#ifndef __XDP2_ARRAYS_H__
#define __XDP2_ARRAYS_H__

/* Definitions and functions for processing and parsing arrays */

#include <linux/types.h>

#ifndef __KERNEL__
#include <stddef.h>
#include <sys/types.h>
#endif

#include "xdp2/parser_types.h"
#include "xdp2/pmacro.h"
#include "xdp2/utility.h"

/* Definitions for parsing arrays
 *
 * Arrays are a common protocol header structure that is any array of
 * elements of a fixed size. The number of elements in an array is variable
 */

/* Descriptor for parsing operations of an array node
 *
 * num_els: Return the number of elements in an array, input is the top
 *	    level header
 * el_type: Return the type of an array element. If the return value is less
 *	    than zero (XDP2_STOP_* value) then this indicates an error or
 *	    an end of the array marker
 * start_offset: Return the start offset of the array in a top level header
 */
struct xdp2_proto_array_opts {
	unsigned int (*num_els)(const void *hdr, size_t hlen);
	int (*el_type)(const void *el_hdr);
	size_t (*start_offset)(const void *hdr);
};

/* Array element parse node operations
 *
 * Operations to process a single array element
 *
 * extract_metadata: Extract metadata for the node. Input is the meta
 *	data frame which points to a parser defined metadata structure.
 *	If the value is NULL then no metadata is extracted
 * handler: Per array element type  handler which allows arbitrary processing
 *	of an array element. Input is the array element and a parser defined
 *	metadata structure for the current frame. Return value is a parser
 *	return code: XDP2_OKAY indicates no errors, XDP2_STOP* return
 *	values indicate to stop parsing
 */
struct xdp2_parse_arrel_node_ops {
	void (*extract_metadata)(const void *el_hdr, size_t hdr_len,
				 size_t hdr_off, void *metadata, void *frame,
				 const struct xdp2_ctrl_data *ctrl);
	int (*handler)(const void *el_hdr, size_t hdr_len, size_t hdr_off,
		       void *metadata, void *frame,
		       const struct xdp2_ctrl_data *ctrl);
};

/* Parse node for a single array element. Use common parse node operations
 * (extract_metadata and handle_proto)
 */
struct xdp2_parse_arrel_node {
	const struct xdp2_parse_arrel_node_ops ops;
	const char *name;
};

/* One entry in an array element type table:
 *	type: array element type
 *	node: associated array parse node for the type
 */
struct xdp2_proto_array_table_entry {
	int type;
	const struct xdp2_parse_arrel_node *node;
};

/* Array table
 *
 * Contains a table that maps an array element type to a array element
 * parse node
 */
struct xdp2_proto_array_table {
	int num_ents;
	const struct xdp2_proto_array_table_entry *entries;
};

/* Parse node for parsing a protocol header that contains an array to be
 * parsed:
 *
 * parse_node: Node for main protocol header (e.g. IPv6 node in case of HBH
 *	options) Note that node_type is set in parse_node to
 *	XDP2_NODE_TYPE_ARRAY and that the parse node can then be cast to a
 *	parse_array_node
 * array_proto_table: Lookup table for array element type
 * el_length: Length of each element
 * max_els: Maximum number of elements that are to be parseed in one list
 * unknown_arrel_type_ret: Error value to set if type is unknown
 * arrel_wildcard_node: Wildcard array element used if type is not found
 */
struct xdp2_parse_array_node {
	const struct xdp2_parse_node pn;
	const struct xdp2_proto_array_table *array_proto_table;
	unsigned int max_els;
	int unknown_array_type_ret;
	const struct xdp2_parse_arrel_node *array_wildcard_node;
};

/* A protocol definition for parsing proto with an array
 *
 * proto_def: Protocol definition
 * ops: Operations for parsing arrays
 * el_length: Constant length of an element
 */
struct xdp2_proto_array_def {
	struct xdp2_proto_def proto_def;
	struct xdp2_proto_array_opts ops;
	size_t el_length;
};

#define __XDP2_MAKE_ARRAY_TABLE_ONE_ENTRY1(KEY, NODE)			\
	{								\
		.type = KEY,						\
		.node = &NODE						\
	},

#define __XDP2_MAKE_ARRAY_TABLE_ONE_ENTRY(ENTRY)			\
	__XDP2_MAKE_ARRAY_TABLE_ONE_ENTRY1(				\
		XDP2_GET_POS_ARG2_1 ENTRY,				\
		XDP2_GET_POS_ARG2_2 ENTRY)

#define __XDP2_MAKE_ARRAY_TABLE_ENTRIES(...)				\
		XDP2_PMACRO_APPLY_ALL(					\
				__XDP2_MAKE_ARRAY_TABLE_ONE_ENTRY,	\
				__VA_ARGS__)

/* Helper to create an array protocol table */
#define XDP2_MAKE_ARRAY_TABLE(NAME, ...)				\
	static const struct xdp2_proto_array_table_entry __##NAME[] =	\
		 { __XDP2_MAKE_ARRAY_TABLE_ENTRIES(__VA_ARGS__) };	\
	static const struct xdp2_proto_array_table NAME = {		\
		.num_ents = sizeof(__##NAME) /				\
			sizeof(struct xdp2_proto_array_table_entry),	\
		.entries = __##NAME,					\
	}

/* Forward declarations for array parser nodes */
#define XDP2_DECL_ARRAY_PARSE_NODE(ARRAY_PARSE_NODE)			\
	static const struct xdp2_parse_array_node ARRAY_PARSE_NODE

/* Forward declarations for array element type tables */
#define XDP2_DECL_ARRAY_TABLE(ARRAY_TABLE)				\
	static const struct xdp2_proto_array_table ARRAY_TABLE

#define __XDP2_MAKE_ARRAY_PARSE_NODE_OPT_ONE(OPT) .pn OPT,

/* Common macro for array parse nodes */
#define __XDP2_MAKE_ARRAY_PARSE_NODE_COMMON(PARSE_ARRAY_NODE,		\
					    PROTO_ARRAY_DEF,		\
					    ARRAY_TABLE, EXTRA_PN,	\
					    EXTRA_ARRAY)		\
		.pn.text_name = #PARSE_ARRAY_NODE,			\
		.pn.node_type = XDP2_NODE_TYPE_ARRAY,			\
		.pn.proto_def = &PROTO_ARRAY_DEF.proto_def,		\
		.pn.unknown_ret = XDP2_STOP_UNKNOWN_PROTO,		\
		.array_proto_table = ARRAY_TABLE,			\
		.max_els = -1U,						\
		.unknown_array_type_ret = XDP2_STOP_UNKNOWN_PROTO,	\
		XDP2_PMACRO_APPLY_ALL(					\
			__XDP2_MAKE_ARRAY_PARSE_NODE_OPT_ONE,		\
			XDP2_DEPAIR(EXTRA_PN))				\
		XDP2_DEPAIR(EXTRA_ARRAY)

#define __XDP2_MAKE_ARRAY_PARSE_NODE(PARSE_ARRAY_NODE, PROTO_ARRAY_DEF,	\
				     PROTO_TABLE, ARRAY_TABLE,		\
				     EXTRA_PN, EXTRA_ARRAY)		\
	XDP2_DECL_PROTO_TABLE(PROTO_TABLE);				\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_array_node PARSE_ARRAY_NODE = {	\
		.pn.proto_table = &PROTO_TABLE,				\
		__XDP2_MAKE_ARRAY_PARSE_NODE_COMMON(PARSE_ARRAY_NODE,	\
			PROTO_ARRAY_DEF, ARRAY_TABLE, EXTRA_PN,		\
			EXTRA_ARRAY)					\
	};								\
	XDP2_POP_NO_WEXTRA()

/* Helper to create an array parse node with a next protocol */
#define XDP2_MAKE_ARRAY_PARSE_NODE(PARSE_ARRAY_NODE, PROTO_ARRAY_DEF,	\
				   PROTO_TABLE, ARRAY_TABLE, EXTRA_PN,	\
				   EXTRA_ARRAY)				\
	XDP2_DECL_ARRAY_TABLE(ARRAY_TABLE);				\
	__XDP2_MAKE_ARRAY_PARSE_NODE(PARSE_ARRAY_NODE, PROTO_ARRAY_DEF,	\
				     PROTO_TABLE, &ARRAY_TABLE,		\
				     EXTRA_PN, EXTRA_ARRAY)

/* Helper to create an array parse node with a next protocol with no
 * array table. The wildcard node can be set to process al the array
 * elements the same way
 */
#define XDP2_MAKE_ARRAY_PARSE_NODE_NOTAB(PARSE_ARRAY_NODE,		\
					 PROTO_ARRAY_DEF, PROTO_TABLE,	\
					 EXTRA_PN, EXTRA_ARRAY)		\
	__XDP2_MAKE_ARRAY_PARSE_NODE(PARSE_ARRAY_NODE, PROTO_ARRAY_DEF,	\
				     PROTO_TABLE, NULL,			\
				     EXTRA_PN, EXTRA_ARRAY)

#define __XDP2_MAKE_ARRAY_AUTONEXT_PARSE_NODE(PARSE_ARRAY_NODE,		\
					      PROTO_ARRAY_DEF,		\
					      NEXT_NODE, ARRAY_TABLE,	\
					      EXTRA_PN, EXTRA_ARRAY)	\
	XDP2_DECL_ARRAY_TABLE(ARRAY_TABLE);				\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_array_node PARSE_ARRAY_NODE = {	\
		.pn.wildcard_node = NEXT_NODE,				\
		__XDP2_MAKE_ARRAY_PARSE_NODE_COMMON(PARSE_ARRAY_NODE,	\
			PROTO_ARRAY_DEF, ARRAY_TABLE, EXTRA_PN,		\
			EXTRA_ARRAY)					\
	}								\
	XDP2_POP_NO_WEXTRA()

/* Helper to create array parse node with an automatic next node */
#define XDP2_MAKE_ARRAY_AUTONEXT_PARSE_NODE(PARSE_ARRAY_NODE,		\
					    PROTO_ARRAY_DEF,		\
					    NEXT_NODE, ARRAY_TABLE,	\
					    EXTRA_PN, EXTRA_ARRAY)	\
	__XDP2_MAKE_ARRAY_AUTONEXT_PARSE_NODE(PARSE_ARRAY_NODE,		\
					      PROTO_ARRAY_DEF,		\
					      NEXT_NODE, &ARRAY_TABLE,	\
					      EXTRA_PN, EXTRA_ARRAY)

/* Helper to create array parse node with an automatic next node
 * with no array table
 */
#define XDP2_MAKE_ARRAY_AUTONEXT_PARSE_NODE_NOTAB(PARSE_ARRAY_NODE,	\
						  PROTO_ARRAY_DEF,	\
						  NEXT_NODE, EXTRA_PN,	\
						  EXTRA_ARRAY)		\
	__XDP2_MAKE_ARRAY_AUTONEXT_PARSE_NODE(PARSE_ARRAY_NODE,		\
					      PROTO_ARRAY_DEF,		\
					      NEXT_NODE, NULL,		\
					      EXTRA_PN, EXTRA_ARRAY)

#define __XDP2_MAKE_LEAF_ARRAY_PARSE_NODE(PARSE_ARRAY_NODE,		\
					  PROTO_ARRAY_DEF, ARRAY_TABLE,	\
					  EXTRA_PN, EXTRA_ARRAY)	\
	XDP2_DECL_ARRAY_TABLE(ARRAY_TABLE);				\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_array_node PARSE_array_NODE = {	\
		__XDP2_MAKE_ARRAY_PARSE_NODE_COMMON(PARSE_ARRAY_NODE,	\
			PROTO_ARRAY_DEF, ARRAY_TABLE, EXTRA_PN,	\
			EXTRA_ARRAY)					\
	};								\
	XDP2_POP_NO_WEXTRA()

/* Helper to create a leaf array parse node */
#define XDP2_MAKE_LEAF_ARRAY_PARSE_NODE(PARSE_ARRAY_NODE,		\
					PROTO_ARRAY_DEF, ARRAY_TABLE,	\
					EXTRA_PN, EXTRA_ARRAY)		\
	__XDP2_MAKE_LEAF_ARRAY_PARSE_NODE(PARSE_ARRAY_NODE,		\
					  PROTO_ARRAY_DEF,		\
					  &ARRAY_TABLE,	EXTRA_PN,	\
					  EXTRA_ARRAY)

/* Helper to create a leaf array parse node with no array table */
#define XDP2_MAKE_LEAF_ARRAY_PARSE_NODE_NOTAB(PARSE_ARRAY_NODE,		\
					      PROTO_ARRAY_DEF,		\
					      EXTRA_PN, EXTRA_ARRAY)	\
	__XDP2_MAKE_LEAF_ARRAY_PARSE_NODE(PARSE_ARRAY_NODE,		\
					  PROTO_ARRAY_DEF, NULL,	\
					  EXTRA_PN, EXTRA_ARRAY)

#define __XDP2_MAKE_ARREL_PARSE_NODE_COMMON(NODE_NAME, EXTRA)		\
		.name = #NODE_NAME,					\
		XDP2_DEPAIR(EXTRA)					\

/* Helper to create a array element parse node */
#define XDP2_MAKE_ARREL_PARSE_NODE(NODE_NAME, EXTRA)			\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_arrel_node NODE_NAME = {		\
		__XDP2_MAKE_ARREL_PARSE_NODE_COMMON(NODE_NAME, EXTRA)	\
	};								\
	XDP2_POP_NO_WEXTRA()

/* Helper macro when accessing a array element node in named parameters or
 * elsewhere
 */
#define XDP2_ARREL_NODE(NAME) &NAME

#endif /* __XDP2_ARRAYS_H__ */
