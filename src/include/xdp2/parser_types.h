/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
  Copyright (c) 2025 Tom Herbert
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

#ifndef __XDP2_TYPES_H__
#define __XDP2_TYPES_H__

/* Type definitions for XDP2 parser */

#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

#include <linux/types.h>

#include "xdp2/compiler_helpers.h"
#include "xdp2/utility.h"

/* XDP2 parser return codes */
enum {
	XDP2_OKAY = 0,			/* Okay and continue */
	XDP2_RET_OKAY = -1,		/* Encoding of OKAY in ret code */

	XDP2_OKAY_USE_WILD = -2,	/* cam instruction */
	XDP2_OKAY_USE_ALT_WILD = -3,	/* cam instruction */

	XDP2_STOP_OKAY = -4,		/* Okay and stop parsing */
	XDP2_STOP_NODE_OKAY = -5,	/* Stop parsing current node */
	XDP2_STOP_SUB_NODE_OKAY = -6,	/* Stop parsing current sub-node */

	/* Parser failure */
	XDP2_STOP_FAIL = -12,
	XDP2_STOP_LENGTH = -13,
	XDP2_STOP_UNKNOWN_PROTO = -14,
	XDP2_STOP_ENCAP_DEPTH = -15,
	XDP2_STOP_UNKNOWN_TLV = -16,
	XDP2_STOP_TLV_LENGTH = -17,
	XDP2_STOP_BAD_FLAG = -18,
	XDP2_STOP_FAIL_CMP = -19,
	XDP2_STOP_LOOP_CNT = -20,
	XDP2_STOP_TLV_PADDING = -21,
	XDP2_STOP_OPTION_LIMIT = -22,
	XDP2_STOP_MAX_NODES = -23,
	XDP2_STOP_COMPARE = -24,
	XDP2_STOP_BAD_EXTRACT = -25,
	XDP2_STOP_BAD_CNTR = -26,
	XDP2_STOP_CNTR1 = -27,
	XDP2_STOP_CNTR2 = -28,
	XDP2_STOP_CNTR3 = -29,
	XDP2_STOP_CNTR4 = -30,
	XDP2_STOP_CNTR5 = -31,
	XDP2_STOP_CNTR6 = -32,
	XDP2_STOP_CNTR7 = -33,

	XDP2_STOP_THREADS_FAIL = -34,
};

/* XDP2 parser type codes */
enum xdp2_parser_type {
	/* Use non-optimized loop xdp2 parser algorithm */
	XDP2_GENERIC,
	/* Use optimized, generated, parser algorithm  */
	XDP2_OPTIMIZED,
	/* XDP parser */
	XDP2_XDP,
};

/* Parse and protocol defintion types */
enum xdp2_parser_node_type {
	/* Plain node, no super structure */
	XDP2_NODE_TYPE_PLAIN,
	/* TLVs node with super structure for TLVs */
	XDP2_NODE_TYPE_TLVS,
	/* Flag-fields with super structure for flag-fields */
	XDP2_NODE_TYPE_FLAG_FIELDS,
};

/* Protocol parsing operations:
 *
 * len: Return length of protocol header. If value is NULL then the length of
 *	the header is taken from the min_len in the protocol definition. If the
 *	return value < 0 (a XDP2_STOP_* return code value) this indicates an
 *	error and parsing is stopped. A the return value greater than or equal
 *	to zero then gives the protocol length. If the returned length is less
 *	than the minimum protocol length, indicated in min_len by the protocol
 *	node, then this considered and error.
 * next_proto: Return next protocol. If value is NULL then there is no
 *	next protocol. If return value is greater than or equal to zero
 *	this indicates a protocol number that is used in a table lookup
 *	to get the next layer protocol definition.
 */
struct xdp2_parse_ops {
	ssize_t (*len)(const void *hdr, size_t maxlen);
	int (*next_proto)(const void *hdr);
	int (*next_proto_keyin)(const void *hdr, __u32 key);
};

/* Protocol definition
 *
 * This structure contains the definitions to describe parsing of one type
 * of protocol header. Fields are:
 *
 * node_type: The type of the node (plain, TLVs, flag-fields)
 * encap: Indicates an encapsulation protocol (e.g. IPIP, GRE)
 * overlay: Indicates an overlay protocol. This is used, for example, to
 *	switch on version number of a protocol header (e.g. IP version number
 *	or GRE version number)
 * name: Text name of protocol definition for debugging
 * min_len: Minimum length of the protocol header
 * ops: Operations to parse protocol header
 */
struct xdp2_proto_def {
	enum xdp2_parser_node_type node_type __attribute__((__packed__));
	__u8 encap;
	__u8 overlay;
	__u16 min_len;
	const char *name;
	const struct xdp2_parse_ops ops;
} __aligned(XDP2_CACHELINE_SIZE) __packed;

XDP2_BUILD_BUG_ON(sizeof(struct xdp2_proto_def) <= XDP2_CACHELINE_SIZE);

struct xdp2_xdp_ctx {
	__u32 frame_num;
	__u32 next;
	__u32 offset;
	void *metadata;
	const struct xdp2_parser *parser;
};

struct xdp2_parse_node;

struct xdp2_ctrl_packet_data {
	void *packet;		/* Packet handle */
	size_t pkt_len;		/* Full length of packet */
	__u32 seqno;		/* Sequence number per interface */
	__u32 timestamp;	/* Received timestamp */
	__u32 in_port;		/* Received port number */
	__u32 vrf_id;		/* Interface ID */
	__u16 pkt_csum;		/* Checksum over packet */
	__u16 flags;		/* Flags */
};

struct xdp2_ctrl_var_data {
	const struct xdp2_parse_node *last_node; /* Last node parsed */
	__s8 ret_code;		/* Return code */
	__u8 encaps;		/* Number of encapsulations */
	__u8 node_cnt;		/* NUmber of nodes visited */
	__u8 tlv_levels;	/* Num. TLV levels (used with nested TLVs) */
	__u16 pkt_csum;		/* Packet checksum to header start */
	__u16 hdr_csum;		/* CHecksum of current header */
};

struct xdp2_ctrl_key_data {
	__u8 *counters;		/* Array of 8-bit counters */
	__u32 *keys;		/* Array of keys for passing between nodes */
	void *arg;		/* Caller argument */
};

struct xdp2_ctrl_data {
	struct xdp2_ctrl_var_data var;
	struct xdp2_ctrl_packet_data pkt;
	struct xdp2_ctrl_key_data key;
};

/* Parse node operations
 *
 * Operations to process a parsing node
 *
 * extract_metadata: Extract metadata for the node. Input is the meta
 *	data frame which points to a parser defined metadata structure.
 *	If the value is NULL then no metadata is extracted
 * handle_proto: Per protocol handler which allows arbitrary processing
 *	of a protocol layer. Input is the header data and a parser defined
 *	metadata structure for the current frame. Return value is a parser
 *	return code: XDP2_OKAY indicates no errors, XDP2_STOP* return
 *	values indicate to stop parsing
 */
struct xdp2_parse_node_ops {
	void (*extract_metadata)(const void *hdr, size_t hdr_len,
				 size_t hdr_off, void *metadata, void *frame,
				 const struct xdp2_ctrl_data *ctrl);
	int (*handler)(const void *hdr, size_t hdr_len, size_t hdr_off,
		       void *metadata, void *frame,
		       const struct xdp2_ctrl_data *ctrl);
	int (*post_handler)(const void *hdr, size_t hdr_len, size_t hdr_off,
			    void *metadata, void *frame,
			    const struct xdp2_ctrl_data *ctrl);
};

/* Protocol definitions and parse node operations ordering. When processing a
 * layer, operations are called in following order:
 *
 * protoop.len
 * parseop.extract_metadata
 * parseop.handle_proto
 * protoop.next_proto
 */

/* One entry in a protocol table:
 *	value: protocol number
 *	node: associated parse node for the protocol number
 */
struct xdp2_proto_table_entry {
	int value;
	const struct xdp2_parse_node *node;
};

/* Protocol table
 *
 * Contains a protocol table that maps a protocol number to a parse
 * node
 */
struct xdp2_proto_table {
	int num_ents;
	const struct xdp2_proto_table_entry *entries;
};

#define XDP2_PARSE_NODE_F_ZERO_LEN_OK	1

/* Parse node definition. Defines parsing and processing for one node in
 * the parse graph of a parser. Contains:
 *
 * node_type: The type of the node (plain, TLVs, flag-fields)
 * proto_def: Protocol definition
 * ops: Parse node operations
 * proto_table: Protocol table for next protocol. This must be non-null if
 * next_proto is not NULL
 */
struct xdp2_parse_node {
	enum xdp2_parser_node_type node_type __attribute__((__packed__));
	__s8 unknown_ret;
	__u8 key_sel;
	__u8 flags;
	__u8 rsvd;
	const struct xdp2_proto_def *proto_def;
	const struct xdp2_parse_node_ops ops;
	const struct xdp2_proto_table *proto_table;
	const struct xdp2_parse_node *wildcard_node;
	char *text_name;
} __aligned(XDP2_CACHELINE_SIZE) __packed;

XDP2_BUILD_BUG_ON(sizeof(struct xdp2_parse_node) <= XDP2_CACHELINE_SIZE);

/* Declaration of a XDP2 parser */
struct xdp2_parser;

/* XDP2 entry-point for optimized parsers */
typedef int (*xdp2_parser_opt_entry_point)(const struct xdp2_parser *parser,
					   void *hdr, size_t len,
					   void *metadata,
					   struct xdp2_ctrl_data *ctrl,
					   unsigned int flags);

/* XDP2 entry-point for XDP parsers */
typedef int (*xdp2_parser_xdp_entry_point)(struct xdp2_xdp_ctx *ctx,
					   const void **hdr,
					   const void *hdr_end,
					   bool tailcall);

struct xdp2_parser_config {
	__u16 max_nodes;
	__u16 max_encaps;
	__u16 max_frames;
	size_t metameta_size;
	size_t frame_size;
	__u8 num_counters;
	__u8 num_keys;
	const struct xdp2_parse_node *okay_node;
	const struct xdp2_parse_node *fail_node;
	const struct xdp2_parse_node *atencap_node;
};

/* Definition of a XDP2 parser. Fields are:
 *
 * name: Text name for the parser
 * root_node: Root parse node of the parser. When the parser is invoked
 *	parsing commences at this parse node
 */
struct xdp2_parser {
	const char *name;
	struct xdp2_parser_config config;
	const struct xdp2_parse_node *root_node;
	enum xdp2_parser_type parser_type;
	xdp2_parser_opt_entry_point parser_entry_point;
	xdp2_parser_xdp_entry_point parser_xdp_entry_point;
};

/* One entry in a parser table:
 *	value: key vlaue
 *	parser: parser associated with the key value
 */
struct xdp2_parser_table_entry {
	int value;
	const struct xdp2_parser **parser;
};

/* Parser table
 *
 * Contains a parser table that maps a key value, which could be a protocol
 * number, to a parser
 */
struct xdp2_parser_table {
	int num_ents;
	const struct xdp2_parser_table_entry *entries;
};

#endif /* __XDP2_TYPES_H__ */
