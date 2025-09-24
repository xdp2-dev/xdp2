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

#ifndef __XDP2_FLAG_FIELDS_H__
#define __XDP2_FLAG_FIELDS_H__

/* Definitions and functions for processing and parsing flag-fields */

#include <linux/types.h>
#include <stddef.h>
#include <stdbool.h>

#include "xdp2/compiler_helpers.h"
#include "xdp2/parser_types.h"
#include "xdp2/pmacro.h"
#include "xdp2/utility.h"

/* Definitions for parsing flag-fields
 *
 * Flag-fields is a common networking protocol construct that encodes optional
 * data in a set of flags and data fields. The flags indicate whether or not a
 * corresponding data field is present. The data fields are fixed length per
 * each flag-field definition and ordered by the ordering of the flags
 * indicating the presence of the fields (e.g. GRE and GUE employ flag-fields)
 */

/* Flag-fields descriptors and tables
 *
 * A set of flag-fields is defined in a table of type struct xdp2_flag_fields.
 * Each entry in the table is a descriptor for one flag-field in a protocol and
 * includes a flag value, mask (for the case of a multi-bit flag), and size of
 * the cooresponding field. A flag is matched if "(flags & mask) == flag"
 */

/* One descriptor for a flag
 *
 * flag: protocol value
 * mask: mask to apply to field
 * size: size for associated field data
 */
struct xdp2_flag_field {
	__u32 flag;
	__u32 mask;
	size_t size;
};

/* Descriptor for a protocol field with flag fields
 *
 * Defines the flags and their data fields for one instance a flag field in
 * in a protocol header (e.g. GRE v0 flags):
 *
 * num_idx: Number of flag_field structures
 * fields: List of defined flag fields
 */
struct xdp2_flag_fields {
	size_t num_idx;
	struct xdp2_flag_field fields[];
};

/* Return the offset of a particular flag */
static inline ssize_t __xdp2_flag_fields_offset(__u32 targ_idx, __u32 flags,
						 const struct xdp2_flag_fields
							*flag_fields)
{
	size_t offset = 0;
	__u32 mask;
	int i;

	for (i = 0; i < targ_idx; i++) {
		mask = flag_fields->fields[i].mask ? :
						flag_fields->fields[i].flag;

		if ((flags & mask) == flag_fields->fields[i].flag)
			offset += flag_fields->fields[i].size;
	}

	return offset;
}

/* Compute the length of optional fields present in a flags field. This is
 * equivalent to finding the "offset" of the theoretical field following the
 * last one
 */
static inline size_t xdp2_flag_fields_length(__u32 flags,
					      const struct xdp2_flag_fields
							*flag_fields)
{
	return __xdp2_flag_fields_offset(flag_fields->num_idx, flags,
					 flag_fields);
}

/* Determine offset of a field given a set of flags */
static inline ssize_t xdp2_flag_fields_offset(__u32 targ_idx, __u32 flags,
					       const struct xdp2_flag_fields
							*flag_fields)
{
	__u32 mask;

	mask = flag_fields->fields[targ_idx].mask ? :
				flag_fields->fields[targ_idx].flag;
	if ((flags & mask) != flag_fields->fields[targ_idx].flag) {
		/* Flag not set */
		return -1;
	}

	return __xdp2_flag_fields_offset(targ_idx, flags, flag_fields);
}

/* Check flags are legal */
static inline bool xdp2_flag_fields_check_invalid(__u32 flags, __u32 mask)
{
	return !!(flags & ~mask);
}

#define __XDP_FLAG_FIELDS_MAKE_GET(NUM)					\
static inline __u##NUM xdp2_flag_fields_get##NUM(const __u8 *fields,	\
			__u32 targ_idx, __u32 flags,			\
			const struct xdp2_flag_fields *flag_fields)	\
{									\
	ssize_t offset = xdp2_flag_fields_offset(targ_idx,		\
						flags, flag_fields);	\
									\
	return offset < 0 ? (__u##NUM)-1ULL :				\
					*(__u##NUM *)&fields[offset];	\
}

/* Make functions to retrieve ordinal values from a flag field */
__XDP_FLAG_FIELDS_MAKE_GET(8)
__XDP_FLAG_FIELDS_MAKE_GET(16)
__XDP_FLAG_FIELDS_MAKE_GET(32)
__XDP_FLAG_FIELDS_MAKE_GET(64)

/* Structure or parsing operations for flag-fields
 *
 * flags_offset: Offset of flags in the protocol header
 * start_fields_offset: Return the offset in the header of the start of the
 *	flag fields data
 */
struct xdp2_proto_flag_fields_ops {
	__u32 (*get_flags)(const void *hdr);
	size_t (*start_fields_offset)(const void *hdr);
};

/* Flag-field parse node operations
 *
 * Operations to process a single flag-field
 *
 * extract_metadata: Extract metadata for the node. Input is the meta
 *	data frame which points to a parser defined metadata structure.
 *	If the value is NULL then no metadata is extracted
 * handler: Per flag-field handler which allows arbitrary processing
 *	of a flag-field. Input is the flag-field data and a parser defined
 *	metadata structure for the current frame. Return value is a parser
 *	return code: XDP2_OKAY indicates no errors, XDP2_STOP* return
 *	values indicate to stop parsing
 */
struct xdp2_parse_flag_field_node_ops {
	void (*extract_metadata)(const void *hdr, void *frame,
				 struct xdp2_ctrl_data ctrl);
	int (*handler)(const void *hdr, void *frame,
		       struct xdp2_ctrl_data ctrl);
};

/* A parse node for a single flag field */
struct xdp2_parse_flag_field_node {
	const struct xdp2_parse_flag_field_node_ops ops;
	const char *name;
};

/* One entry in a flag-fields protocol table:
 *	index: flag-field index (index in a flag-fields table)
 *	node: associated TLV parse structure for the type
 */
struct xdp2_proto_flag_fields_table_entry {
	int index;
	const struct xdp2_parse_flag_field_node *node;
};

/* Flag-fields table
 *
 * Contains a table that maps a flag-field index to a flag-field parse node.
 * Note that the index correlates to an entry in a flag-fields table that
 * describes the flag-fields of a protocol
 */
struct xdp2_proto_flag_fields_table {
	int num_ents;
	const struct xdp2_proto_flag_fields_table_entry *entries;
};

/* A flag-fields parse node. Note this is a super structure for a XDP2 parse
 * node and tyoe is XDP2_NODE_TYPE_FLAG_FIELDS
 */
struct xdp2_parse_flag_fields_node {
	const struct xdp2_parse_node pn;
	const struct xdp2_proto_flag_fields_table *flag_fields_proto_table;
};

/* A flag-fields protocol definition. Note this is a super structure for an
 * XDP2 protocol definition and type is XDP2_NODE_TYPE_FLAG_FIELDS
 */
struct xdp2_proto_flag_fields_def {
	struct xdp2_proto_def proto_def;
	struct xdp2_proto_flag_fields_ops ops;
	const struct xdp2_flag_fields *flag_fields;
};

#define __XDP2_MAKE_FLAG_FIELD_TABLE_ONE_ENTRY1(KEY, NODE)		\
	{								\
		.index = KEY,						\
		.node = &NODE						\
	},

#define __XDP2_MAKE_FLAG_FIELD_TABLE_ONE_ENTRY(ENTRY)			\
	__XDP2_MAKE_FLAG_FIELD_TABLE_ONE_ENTRY1(			\
		XDP2_GET_POS_ARG2_1 ENTRY,				\
		XDP2_GET_POS_ARG2_2 ENTRY)

#define __XDP2_MAKE_FLAG_FIELD_TABLE_ENTRIES(...)			\
		XDP2_PMACRO_APPLY_ALL(					\
				__XDP2_MAKE_FLAG_FIELD_TABLE_ONE_ENTRY,	\
				__VA_ARGS__)

/* Helper to create a flag-fields protocol table */
#define XDP2_MAKE_FLAG_FIELDS_TABLE(NAME, ...)				\
	static const struct xdp2_proto_flag_fields_table_entry		\
					__##NAME[] =			\
		{ __XDP2_MAKE_FLAG_FIELD_TABLE_ENTRIES(__VA_ARGS__) };	\
	static const struct xdp2_proto_flag_fields_table NAME = {	\
		.num_ents = sizeof(__##NAME) /				\
			sizeof(struct					\
				xdp2_proto_flag_fields_table_entry),	\
		.entries = __##NAME,					\
	}

/* Forward declarations for flag-fields parse nodes */
#define XDP2_DECL_FLAG_FIELDS_PARSE_NODE(FLAG_FIELDS_PARSE_NODE)	\
	static const struct xdp2_parse_flag_fields_node		\
						FLAG_FIELDS_PARSE_NODE

/* Forward declarations for flag-field proto tables */
#define XDP2_DECL_FLAG_FIELDS_TABLE(FLAG_FIELDS_TABLE)			\
	static const struct xdp2_proto_flag_fields_table		\
						FLAG_FIELDS_TABLE

#define __XDP2_MAKE_FLAG_FIELDS_PARSE_NODE_OPT_ONE(OPT) .pn OPT,

#define XDP2_MAKE_FLAG_FIELDS_PARSE_NODE_COMMON(PARSE_FLAG_FIELDS_NODE,	\
		PROTO_FLAG_FIELDS_DEF, FLAG_FIELDS_TABLE, EXTRA_PN,	\
		EXTRA_FF)						\
		.flag_fields_proto_table = &FLAG_FIELDS_TABLE,		\
		.pn.text_name = #PARSE_FLAG_FIELDS_NODE,		\
		.pn.node_type = XDP2_NODE_TYPE_FLAG_FIELDS,		\
		.pn.proto_def = &PROTO_FLAG_FIELDS_DEF.proto_def,	\
		XDP2_PMACRO_APPLY_ALL(					\
			__XDP2_MAKE_FLAG_FIELDS_PARSE_NODE_OPT_ONE,	\
			XDP2_DEPAIR(EXTRA_PN))				\
		XDP2_DEPAIR(EXTRA_FF)

/* Helper to create a flag-fields parse node with a next protocol */
#define XDP2_MAKE_FLAG_FIELDS_PARSE_NODE(PARSE_FLAG_FIELDS_NODE,	\
					  PROTO_FLAG_FIELDS_DEF,	\
					  PROTO_TABLE,			\
					  FLAG_FIELDS_TABLE, EXTRA_PN,	\
					  EXTRA_FF)			\
	XDP2_DECL_FLAG_FIELDS_TABLE(FLAG_FIELDS_TABLE);			\
	XDP2_DECL_PROTO_TABLE(PROTO_TABLE);				\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_flag_fields_node			\
					PARSE_FLAG_FIELDS_NODE = {	\
		.pn.proto_table = &PROTO_TABLE,				\
		XDP2_MAKE_FLAG_FIELDS_PARSE_NODE_COMMON(		\
			PARSE_FLAG_FIELDS_NODE, PROTO_FLAG_FIELDS_DEF,	\
			FLAG_FIELDS_TABLE, EXTRA_PN, EXTRA_FF)		\
	};								\
	XDP2_POP_NO_WEXTRA()

/* Helper to create flag-fields parse node with an automatic next node */
#define XDP2_MAKE_FLAG_FIELDS_AUTONEXT_PARSE_NODE(			\
					PARSE_FLAG_FIELDS_NODE,		\
					PROTO_FLAG_FIELDS_DEF,		\
					NEXT_NODE, FLAG_FIELDS_TABLE,	\
					EXTRA_PN, EXTRA_FF)		\
	XDP2_DECL_FLAG_FIELDS_TABLE(FLAG_FIELDS_TABLE);			\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_flag_fields_node			\
					PARSE_FLAG_FIELDS_NODE = {	\
		.pn.wildcard_node = &NEXT_NODE.pn,			\
		XDP2_MAKE_FLAG_FIELDS_PARSE_NODE_COMMON(		\
			PARSE_FLAG_FIELDS_NODE, PROTO_FLAG_FIELDS_DEF,	\
			FLAG_FIELDS_TABLE, EXTRA_PN, EXTRA_FF)		\
	};								\
	XDP2_POP_NO_WEXTRA()

/* Helper to create a leaf flag-fields parse node */
#define XDP2_MAKE_LEAF_FLAG_FIELDS_PARSE_NODE(PARSE_FLAG_FIELDS_NODE,	\
					      PROTO_FLAG_FIELDS_DEF,	\
					      FLAG_FIELDS_TABLE,	\
					      EXTRA_PN, EXTRA_FF)	\
	XDP2_DECL_FLAG_FIELDS_TABLE(FLAG_FIELDS_TABLE);			\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_flag_fields_node			\
					PARSE_FLAG_FIELDS_NODE = {	\
		XDP2_MAKE_FLAG_FIELDS_PARSE_NODE_COMMON(		\
			PARSE_FLAG_FIELDS_NODE, PROTO_FLAG_FIELDS_DEF,	\
			FLAG_FIELDS_TABLE, EXTRA_PN, EXTRA_FF)		\
	};								\
	XDP2_POP_NO_WEXTRA()

/* Helper to create a parse node for a single flag-field */
#define XDP2_MAKE_FLAG_FIELD_PARSE_NODE(NODE_NAME, EXTRA)		\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_flag_field_node NODE_NAME = {	\
		.name = #NODE_NAME,					\
		XDP2_DEPAIR(EXTRA)					\
	};								\
	XDP2_POP_NO_WEXTRA()

/* Null flag-field node for filling out flag-fields table */
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(XDP2_FLAG_NODE_NULL, ());

/* Helper macro when accessing a flag field  node in named parameters or
 * elsewhere
 */
#define XDP2_FLAG_FIELD_NODE(NAME) &NAME

#endif /* __XDP2_FLAG_FIELDS_H__ */
