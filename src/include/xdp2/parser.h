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

#ifndef __XDP2_PARSER_H__
#define __XDP2_PARSER_H__

/* Parser interface for XDP2
 *
 * Definitions and functions for XDP2 parser.
 */

#include <linux/types.h>
#include <string.h>

#include "xdp2/compiler_helpers.h"
#include "xdp2/flag_fields.h"
#include "xdp2/parser_types.h"
#include "xdp2/pmacro.h"
#include "xdp2/tlvs.h"
#include "xdp2/utility.h"

#ifndef __KERNEL__
#include "siphash/siphash.h"
#endif

static const struct {
	int code;
	char *text;
} xdp2_text_codes[] = {
	{ XDP2_OKAY, "XDP2 okay" },
	{ XDP2_STOP_OKAY, "XDP2 stop okay" },
	{ XDP2_STOP_FAIL, "XDP2 stop fail" },
	{ XDP2_STOP_LENGTH, "XDP2 stop length" },
	{ XDP2_STOP_UNKNOWN_PROTO, "XDP2 stop unknown protocol" },
	{ XDP2_STOP_ENCAP_DEPTH, "XDP2 stop encapsulation depth" },
	{ XDP2_STOP_UNKNOWN_TLV, "XDP2 stop unknown TLV" },
	{ XDP2_STOP_TLV_LENGTH, "XDP2 stop TLV length" },
	{ XDP2_STOP_BAD_FLAG, "XDP2 stop bad flag" },
};

static const struct xdp2_proto_def xdp2_parse_null __unused() = {
	.name = "NULL-proto",
};

static const struct xdp2_proto_def xdp2_parse_null_overlay __unused() = {
	.name = "NULL-proto",
	.overlay = 1,
};

static inline const char *xdp2_get_text_code(int code)
{
	static char buff[sizeof("XDP2 Unknown code: XXXXXXXX")];
	int i;

	for (i = 0; i < ARRAY_SIZE(xdp2_text_codes); i++)
		if (xdp2_text_codes[i].code == code)
			return xdp2_text_codes[i].text;

	snprintf(buff, sizeof(buff), "XDP2 Unknown code: %u", code);

	return buff;
}

#define XDP2_PARSER_DEFAULT_MAX_NODES		255
#define XDP2_PARSER_DEFAULT_MAX_ENCAPS		4
#define XDP2_PARSER_DEFAULT_MAX_FRAMES		4
#define XDP2_PARSER_DEFAULT_METAMETA_SIZE	64
#define XDP2_PARSER_DEFAULT_FRAME_SIZE		256

#define __XDP2_PARSER_CONFIG_DEFAULTS(NAME, ROOT_NODE)			\
	.max_nodes = XDP2_PARSER_DEFAULT_MAX_NODES,			\
	.max_encaps = XDP2_PARSER_DEFAULT_MAX_ENCAPS,			\
	.max_frames = XDP2_PARSER_DEFAULT_MAX_FRAMES,			\
	.metameta_size = XDP2_PARSER_DEFAULT_METAMETA_SIZE,		\
	.frame_size = XDP2_PARSER_DEFAULT_FRAME_SIZE,

#define XDP2_CODE_IS_OKAY(CODE)						\
	((CODE) == XDP2_OKAY) || ((CODE) == XDP2_STOP_OKAY)

#define __XDP2_PARSER(PARSER, TYPE, NAME, ROOT_NODE, ENTRY_POINT,	\
		      CONFIG, STATIC)					\
	XDP2_PUSH_NO_WEXTRA();						\
	STATIC const struct xdp2_parser __##PARSER = {			\
		.parser_type = TYPE,					\
		.parser_entry_point = ENTRY_POINT,			\
		.name = NAME,						\
		.root_node = &ROOT_NODE.pn,				\
		.config = {						\
			__XDP2_PARSER_CONFIG_DEFAULTS(NAME, ROOT_NODE)	\
			XDP2_DEPAIR(CONFIG)				\
		}							\
	};								\
	XDP2_POP_NO_WEXTRA();						\
	STATIC const struct xdp2_parser *PARSER __unused() =		\
					&XDP2_JOIN2(__, PARSER);

/* Helper function to create an XDP2 parser */
#define XDP2_PARSER(PARSER, NAME, ROOT_NODE, CONFIG)			\
	__XDP2_PARSER(PARSER, XDP2_GENERIC, NAME, ROOT_NODE, NULL, CONFIG,)

/* Helper function to create an XDP2 parser that is declared to be static */
#define XDP2_PARSER_STATIC(PARSER, NAME, ROOT_NODE, CONFIG)		\
	__XDP2_PARSER(PARSER, XDP2_GENERIC, NAME, ROOT_NODE, NULL,	\
		      CONFIG, static)

/* Helper to make an extern for a parser */
#define XDP2_PARSER_EXTERN(NAME) extern const struct xdp2_parser *NAME

/* Helper to make forward declaration for a const parser */
#define XDP2_PARSER_STATIC_DECL(NAME) static const struct xdp2_parser *NAME

/* Helper to create an optimized parservairant */
#define XDP2_PARSER_OPT(PARSER, NAME, ROOT_NODE, FUNC, CONFIG)		\
	__XDP2_PARSER(PARSER, XDP2_OPTIMIZED, NAME, ROOT_NODE,		\
		      FUNC, CONFIG,)					\

/* Helper to create an XDP parser variant */
#define XDP2_PARSER_XDP(PARSER, NAME, ROOT_NODE, FUNC, CONFIG)		\
	__XDP2_PARSER(PARSER, XDP2_XDP, NAME, ROOT_NODE,		\
		      FUNC, CONFIG,)					\

#define __XDP2_MAKE_PARSER_TABLE_ONE_ENTRY1(KEY, NODE)			\
	{								\
		.value = KEY,						\
		.parser = &NODE						\
	},

#define __XDP2_MAKE_PARSER_TABLE_ONE_ENTRY(ENTRY)			\
	__XDP2_MAKE_PARSER_TABLE_ONE_ENTRY1(				\
		XDP2_GET_POS_ARG2_1 ENTRY, XDP2_GET_POS_ARG2_2 ENTRY)

#define __XDP2_MAKE_PARSER_TABLE_ENTRIES(...)				\
		XDP2_PMACRO_APPLY_ALL(					\
				__XDP2_MAKE_PARSER_TABLE_ONE_ENTRY,	\
				__VA_ARGS__)

/* Helper to create a parser table */
#define XDP2_MAKE_PARSER_TABLE(NAME, ...)				\
	static const struct xdp2_parser_table_entry __##NAME[] =	\
		{ __XDP2_MAKE_PARSER_TABLE_ENTRIES(__VA_ARGS__) };	\
	static const struct xdp2_parser_table NAME =	{		\
		.num_ents = sizeof(__##NAME) /				\
			sizeof(struct xdp2_parser_table_entry),	\
		.entries = __##NAME,					\
	}

#define __XDP2_MAKE_PROTO_TABLE_ONE_ENTRY1(KEY, NODE)			\
	{								\
		.value = KEY,						\
		.node = &NODE.pn					\
	},

#define __XDP2_MAKE_PROTO_TABLE_ONE_ENTRY(ENTRY)			\
	__XDP2_MAKE_PROTO_TABLE_ONE_ENTRY1(				\
		XDP2_GET_POS_ARG2_1 ENTRY, XDP2_GET_POS_ARG2_2 ENTRY)

#define __XDP2_MAKE_PROTO_TABLE_ENTRIES(...)				\
		XDP2_PMACRO_APPLY_ALL(					\
				__XDP2_MAKE_PROTO_TABLE_ONE_ENTRY,	\
				__VA_ARGS__)

/* Helper to create a protocol table */
#define XDP2_MAKE_PROTO_TABLE(NAME, ...)				\
	static const struct xdp2_proto_table_entry __##NAME[] =		\
		{ __XDP2_MAKE_PROTO_TABLE_ENTRIES(__VA_ARGS__) };	\
	static const struct xdp2_proto_table NAME =	{		\
		.num_ents = sizeof(__##NAME) /				\
				sizeof(struct xdp2_proto_table_entry),	\
		.entries = __##NAME,					\
	}

/* User visible plain parse node. Just contains an xdp2_parse_node structure */
struct xdp2_parse_user_node {
	const struct xdp2_parse_node pn;
};

/* Forward declarations for parse nodes */
#define XDP2_DECL_PARSE_NODE(PARSE_NODE)				\
	static const struct xdp2_parse_user_node PARSE_NODE

/* Forward declarations for protocol tables */
#define XDP2_DECL_PROTO_TABLE(PROTO_TABLE)				\
	static const struct xdp2_proto_table PROTO_TABLE

#define __XDP2_MAKE_PARSE_NODE_OPT_ONE(OPT) .pn OPT,

#define __XDP2_MAKE_PARSE_NODE_COMMON(PARSE_NODE, PROTO_DEF,		\
				      PROTO_TABLE, EXTRA)		\
		.pn.node_type = XDP2_NODE_TYPE_PLAIN,			\
		.pn.text_name = #PARSE_NODE,				\
		.pn.proto_def = &PROTO_DEF,				\
		.pn.proto_table = PROTO_TABLE,				\
		.pn.unknown_ret = XDP2_STOP_UNKNOWN_PROTO,		\
		XDP2_PMACRO_APPLY_ALL(					\
				__XDP2_MAKE_PARSE_NODE_OPT_ONE,		\
				XDP2_DEPAIR(EXTRA))

/* Helper to create a parse node with a next protocol table */
#define XDP2_MAKE_PARSE_NODE(PARSE_NODE, PROTO_DEF,			\
			     PROTO_TABLE, EXTRA)			\
	XDP2_DECL_PROTO_TABLE(PROTO_TABLE);				\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_user_node PARSE_NODE = {		\
		__XDP2_MAKE_PARSE_NODE_COMMON(PARSE_NODE, PROTO_DEF,	\
					      &PROTO_TABLE, EXTRA)	\
	};								\
	XDP2_POP_NO_WEXTRA()

/* Helper to create a parse node with an automatic next node */
#define XDP2_MAKE_AUTONEXT_PARSE_NODE(PARSE_NODE, PROTO_DEF,		\
				      OVERLAY_NODE, EXTRA)		\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_user_node PARSE_NODE = {		\
		__XDP2_MAKE_PARSE_NODE_COMMON(PARSE_NODE, PROTO_DEF,	\
					      NULL, EXTRA)		\
		.pn.wildcard_node = &OVERLAY_NODE.pn,			\
	};								\
	XDP2_POP_NO_WEXTRA()

/* Helper to create a leaf parse node */
#define XDP2_MAKE_LEAF_PARSE_NODE(PARSE_NODE, PROTO_DEF, EXTRA)	\
	XDP2_PUSH_NO_WEXTRA();						\
	static const struct xdp2_parse_user_node PARSE_NODE = {		\
		__XDP2_MAKE_PARSE_NODE_COMMON(PARSE_NODE, PROTO_DEF,	\
					      NULL, EXTRA)		\
	}

/* Parsing functions */

/* Flags to XDP2 parser functions */
#define XDP2_F_DEBUG			(1 << 0)

#ifndef __KERNEL__
/* Parse starting at the provided root node */
int __xdp2_parse(const struct xdp2_parser *parser, void *hdr,
		 size_t len, void *metadata,
		 struct xdp2_ctrl_data *ctrl, unsigned int flags);

int __xdp2_parse_fast(const struct xdp2_parser *parser, void *hdr,
		      size_t len, void *metadata,
		      struct xdp2_ctrl_data *ctrl);
#else
static inline int __xdp2_parse(const struct xdp2_parser *parser, void *hdr,
			       size_t len, void *metadata,
			       struct xdp2_ctrl_data *ctrl, unsigned int flags)
{
	return 0;
}

static inline int __xdp2_parse_fast(const struct xdp2_parser *parser, void *hdr,
			     size_t len, void *metadata,
			     struct xdp2_ctrl_data *ctrl)
{
	return 0;
}
#endif

bool xdp2_parse_validate_fast(const struct xdp2_parser *parser);

/* Parse packet starting from a parser node
 *
 * Arguments:
 *	- parser: Parser being invoked
 *	- hdr: pointer to start of packet
 *	- len: length of packet
 *	- metadata: metadata structure
 *	- flags: allowed parameterized parsing
 *	- max_encaps: maximum layers of encapsulation to parse
 *
 * Returns XDP2 return code value.
 */
static inline int xdp2_parse(const struct xdp2_parser *parser,
			     void *hdr, size_t len,
			     void *metadata,
			     struct xdp2_ctrl_data *ctrl,
			     unsigned int flags)
{
	switch (parser->parser_type) {
	case XDP2_GENERIC:
		return __xdp2_parse(parser, hdr, len, metadata,
				    ctrl, flags);
	case XDP2_OPTIMIZED:
		return (parser->parser_entry_point)(parser, hdr, len,
						    metadata, ctrl, flags);
	default:
		return XDP2_STOP_FAIL;
	}
}

static inline int xdp2_parse_fast(const struct xdp2_parser *parser,
			     void *hdr, size_t len,
			     void *metadata,
			     struct xdp2_ctrl_data *ctrl)
{
	return __xdp2_parse_fast(parser, hdr, len, metadata, ctrl);
}

#define XDP2_CTRL_RESET_VAR_DATA(CTRL)					\
	memset(&(CTRL).var, 0, sizeof((CTRL).var))

#define XDP2_CTRL_SET_BASIC_PKT_DATA(CTRL, PACKET, LENGTH, SEQNO) do {	\
	memset(&(CTRL).pkt, 0, sizeof((CTRL).pkt));			\
	(CTRL).pkt.packet = PACKET;					\
	(CTRL).pkt.pkt_len = LENGTH;					\
	(CTRL).pkt.seqno = SEQNO;					\
} while (0)

#define XDP2_CTRL_INIT_KEY_DATA(CTRL, PARSER, ARG) do {			\
	const struct xdp2_parser *_parser = (PARSER);			\
	struct xdp2_ctrl_data *_ctrl = &(CTRL);				\
	void *p;							\
									\
	memset(_ctrl, 0, sizeof(*_ctrl));				\
	_ctrl->key.arg = ARG;						\
	if (_parser->config.num_counters) {				\
		size_t sz = _parser->config.num_counters *		\
					sizeof(_ctrl->key.counters[0]);	\
									\
		p = alloca(sz);						\
		memset(p, 0, sz);					\
		_ctrl->key.counters = p;				\
	}								\
	if (_parser->config.num_keys) {					\
		size_t sz = _parser->config.num_keys *			\
					sizeof(_ctrl->key.keys[0]);	\
									\
		p = alloca(sz);						\
		memset(p, 0, sz);					\
		_ctrl->key.keys = p;					\
	}								\
} while (0)

static inline const struct xdp2_parser *xdp2_lookup_parser_table(
				const struct xdp2_parser_table *table,
				int key)
{
	int i;

	for (i = 0; i < table->num_ents; i++)
		if (table->entries[i].value == key)
			return *table->entries[i].parser;

	return NULL;
}

static inline int xdp2_parse_from_table(const struct xdp2_parser_table *table,
					int key,
					struct xdp2_ctrl_data *ctrl,
					void *metadata, unsigned int flags)
{
	const struct xdp2_parser *parser;

	if (!(parser = xdp2_lookup_parser_table(table, key)))
		return XDP2_STOP_FAIL;

	return xdp2_parse(parser, NULL, 0, metadata, ctrl, flags);
}


static inline int xdp2_parse_xdp(const struct xdp2_parser *parser,
				  struct xdp2_xdp_ctx *ctx, void *packet,
				  void *hdr, const void *hdr_end,
				  bool tailcall, void *arg)
{
	if (parser->parser_type != XDP2_XDP)
		return XDP2_STOP_FAIL;

	return (parser->parser_xdp_entry_point)(ctx, hdr, hdr_end, tailcall);
}

static inline int __xdp2_parse_run_exit_node(const struct xdp2_parser *parser,
					     const struct xdp2_parse_node
								*parse_node,
					     void *metadata,
					     void *_frame,
					     struct xdp2_ctrl_data *ctrl,
					     unsigned int flags)
{
	if (parse_node->ops.extract_metadata)
		parse_node->ops.extract_metadata(NULL, 0, 0, metadata, _frame,
						 ctrl);

	if (parse_node->ops.handler)
		parse_node->ops.handler(NULL, 0, 0, metadata, _frame, ctrl);

	return 0;
}

#define XDP2_PARSE_XDP(PARSER, CTX, HDR, HDR_END, TAILCALL, FLAGS)	\
	xdp2_xdp_parser_##PARSER(CTX, HDR, HDR_END, TAILCALL, FLAGS)

/* Helper macro when accessing a parse node in named parameters or elsewhere */
#define XDP2_PARSE_NODE(NAME) &NAME.pn

#ifndef __KERNEL__

extern siphash_key_t __xdp2_hash_key;

/* Helper functions to compute the siphash from start pointer
 * through len bytes. Note that siphash library expects start to
 * be aligned to 64 bits
 */
static inline __u32 xdp2_compute_hash(const void *start, size_t len)
{
	__u32 hash;

	hash = siphash(start, len, &__xdp2_hash_key);
	if (!hash)
		hash = 1;

	return hash;
}

/* Helper macro to compute a hash from a metadata structure. METADATA
 * is a pointer to a metadata structure and HASH_START_FIELD is the offset
 * within the structure to start the hash. This macro requires that the
 * common metadata for IP addresses is defined in the metadata structure,
 * that is there is an addrs field of type XDP2_METADATA_addrs in the
 * metadata structure. The end offset of the hash area is the last byte
 * of the addrs structure which can be different depending on the type
 * of address (for instance, IPv6 addresses have more bytes than IPv4
 * addresses so the length of the bytes hashed area will be greater).
 */
#define XDP2_COMMON_COMPUTE_HASH(METADATA, HASH_START_FIELD) ({	\
	__u32 hash;							\
	const void *start = XDP2_HASH_START(METADATA,			\
					     HASH_START_FIELD);		\
	size_t olen = XDP2_HASH_LENGTH(METADATA,			\
				offsetof(typeof(*METADATA),		\
				HASH_START_FIELD));			\
									\
	hash = xdp2_compute_hash(start, olen);				\
	hash;								\
})

/* Initialization function for hash key. If the argument is NULL the
 * hash key is randomly set
 */
void xdp2_hash_secret_init(siphash_key_t *init_key);

/* Function to print the raw bytesused in a hash */
void xdp2_print_hash_input(const void *start, size_t len);

#endif /* __KERNEL__ */

#endif /* __XDP2_PARSER_H__ */
