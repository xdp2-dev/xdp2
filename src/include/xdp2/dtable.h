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

#ifndef __XDP2_DTABLES_H__
#define __XDP2_DTABLES_H__

/* Dyanamic tables */

#include <sys/queue.h>

#include "xdp2/table_common.h"

typedef void (*xdp2_dftable_func_t)(void *call_arg, void *entry_arg);

struct __xdp2_dtable_entry_func_target {
	xdp2_dftable_func_t func;
	void *arg;
};

/* Generic dtable table entry */
struct xdp2_dtable_entry {
	int ident;
	struct xdp2_dtable_entry *next;
	LIST_ENTRY(xdp2_dtable_entry) list_ent;
	LIST_ENTRY(xdp2_dtable_entry) list_ent_lookup;
	__u8 data[];
};

#define XDP2_DTABLE_TARG(TABLE, ENTRY) ((ENTRY)->data)
#define XDP2_DTABLE_KEY(TABLE, ENTRY) ((ENTRY)->data + (TABLE)->targ_len)

LIST_HEAD(__xdp2_dtable_list_head, xdp2_dtable_entry);

enum xdp2_dtable_table_types {
	XDP2_DTABLE_TABLE_TYPE_INVALID = 0,
	XDP2_DTABLE_TABLE_TYPE_PLAIN,
	XDP2_DTABLE_TABLE_TYPE_TERN,
	XDP2_DTABLE_TABLE_TYPE_LPM,
};

struct xdp2_dtable_config {
	size_t size;
};

#define DTABLE_STRUCT_ELS()						\
	const char *name;						\
	size_t key_len;							\
	size_t targ_len;						\
	size_t entry_len;						\
	const void *default_target;					\
	int ident;							\
	LIST_ENTRY(xdp2_dtable_table) list_ent;			\
	struct __xdp2_dtable_list_head entries;			\
	struct __xdp2_dtable_list_head entries_lookup;			\
	bool constant;							\
	enum xdp2_dtable_table_types table_type;			\
	struct xdp2_dtable_config config;

struct xdp2_dtable_table {
	DTABLE_STRUCT_ELS();
} XDP2_ALIGN_SECTION;

XDP2_DEFINE_SECTION(xdp2_dtable_section_tables, struct xdp2_dtable_table);

/* Create table structure for plain, ternary, and longest prefix
 * match tables
 */
#define __XDP2_DTABLE_DEFINE_TABLE(TYPE)				\
struct  XDP2_JOIN3(xdp2_dtable_, TYPE, _table)	{			\
	union {								\
		struct {						\
			DTABLE_STRUCT_ELS();				\
		};							\
		struct xdp2_dtable_table _t;				\
	} XDP2_ALIGN_SECTION;						\
};

__XDP2_DTABLE_DEFINE_TABLE(plain)
__XDP2_DTABLE_DEFINE_TABLE(tern)
__XDP2_DTABLE_DEFINE_TABLE(lpm)

/* Table functions prototypes */

/* Called to initialize any constant dtable (from section array) */
void xdp2_dtable_init(void);

struct xdp2_dtable_plain_table *xdp2_dtable_create_plain(
		const char *name, size_t key_len, void *default_target,
		size_t target_len, int *ident);

struct xdp2_dtable_tern_table *xdp2_dtable_create_tern(
		const char *name, size_t key_len, void *default_target,
		size_t target_len, int *ident);

struct xdp2_dtable_lpm_table *xdp2_dtable_create_lpm(
		const char *name, size_t key_len, void *default_target,
		size_t target_len, int *ident);

int xdp2_dtable_add_plain(struct xdp2_dtable_plain_table *table,
			  int ident, const void *key, void *target);

int xdp2_dtable_add_tern(struct xdp2_dtable_tern_table *table,
			 int ident, const void *key, const void *key_mask,
			 unsigned int pos, void *target);

int xdp2_dtable_add_lpm(struct xdp2_dtable_lpm_table *table, int ident,
			const void *key, size_t prefix_len, void *target);

void xdp2_dtable_del_plain(struct xdp2_dtable_plain_table *table,
			   const void *key);

void xdp2_dtable_del_plain_by_id(struct xdp2_dtable_plain_table *table,
				 int ident);

void xdp2_dtable_del_tern(struct xdp2_dtable_tern_table *table,
			  const void *key, const void *key_mask,
			  unsigned int pos);

void xdp2_dtable_del_tern_by_id(struct xdp2_dtable_tern_table *table,
				int ident);

void xdp2_dtable_del_lpm(struct xdp2_dtable_lpm_table *table,
			 const void *key, size_t prefix_len);

void xdp2_dtable_del_lpm_by_id(struct xdp2_dtable_lpm_table *table, int ident);

int xdp2_dtable_change_plain(struct xdp2_dtable_plain_table *table,
			     const void *key, void *target);

int xdp2_dtable_change_plain_by_id(struct xdp2_dtable_plain_table *table,
				   int ident, void *target);

int xdp2_dtable_change_tern(struct xdp2_dtable_tern_table *table,
			    const void *key, const void *key_mask,
			    unsigned int pos, void *target);

int xdp2_dtable_change_tern_by_id(struct xdp2_dtable_tern_table *table,
				  int ident, void *target);

int xdp2_dtable_change_lpm(struct xdp2_dtable_lpm_table *table,
			   const void *key, size_t prefix_len, void *target);

int xdp2_dtable_change_lpm_by_id(struct xdp2_dtable_lpm_table *table,
				 int ident, void *target);

const void *xdp2_dtable_lookup_plain(struct xdp2_dtable_plain_table *table,
				     const void *key);

const void *xdp2_dtable_lookup_tern(struct xdp2_dtable_tern_table *table,
				    const void *key);

const void *xdp2_dtable_lookup_lpm(struct xdp2_dtable_lpm_table *table,
				   const void *key);

void xdp2_dtable_print_all_tables(void);

void xdp2_dtable_print_one_table(void *cli, struct xdp2_dtable_table *table);

/* Definitions and macros for entry key structures */

struct xdp2_plain_entry_key {
	__u64 hash;
	__u8 key[];
};

struct xdp2_tern_entry_key {
	unsigned int pos;
	__u8 key[]; /* Key and key-mask */
};

struct xdp2_lpm_entry_key {
	size_t prefix_len;
	__u8 key[];
};

enum {
	__xdp2_key_mult_plain = 1,
	__xdp2_key_mult_tern = 2,
	__xdp2_key_mult_lpm = 1,
};

#define __XDP2_DTABLE_KEY_STRUCT_SIZE(LTYPE, KEY_TYPE)			\
	(XDP2_JOIN2(__xdp2_key_mult_, LTYPE) *				\
		sizeof *((KEY_TYPE)(0)) +				\
		sizeof(struct XDP2_JOIN3(xdp2_, LTYPE, _entry_key)))

/* Define a table */
#define ___XDP2_DTABLE_MAKE_MATCH_TABLE(NAME, TARG_TYPE, DEFAULT_TARG,	\
					TYPE, LTYPE, KEY_TYPE, CONFIG)	\
	const TARG_TYPE XDP2_JOIN2(NAME, _default_target) =		\
			DEFAULT_TARG;					\
									\
	struct XDP2_JOIN3(xdp2_dtable_, LTYPE, _table)			\
			XDP2_JOIN2(NAME, _table)			\
			XDP2_SECTION_ATTR(				\
					xdp2_dtable_section_tables) = {\
		.name = #NAME,						\
		.default_target = &XDP2_JOIN2(NAME, _default_target),	\
		.key_len = sizeof *((KEY_TYPE)(0)),			\
		.targ_len = sizeof(TARG_TYPE),				\
		.entry_len = sizeof(struct xdp2_dtable_entry) +		\
			__XDP2_DTABLE_KEY_STRUCT_SIZE(LTYPE, KEY_TYPE) +\
			sizeof(TARG_TYPE),				\
		.constant = true,					\
		.table_type = XDP2_JOIN2(XDP2_DTABLE_TABLE_TYPE_, TYPE),\
		.config = {						\
			.size = -1U,					\
			XDP2_DEPAIR(CONFIG)				\
		},							\
	};

#define __XDP2_DTABLE_MAKE_MATCH_TABLE(NAME, DEFAULT_TARG, TYPE, LTYPE,	\
				       KEY_TYPE, CONFIG)		\
	___XDP2_DTABLE_MAKE_MATCH_TABLE(				\
		NAME, XDP2_JOIN2(NAME, _targ_t), DEFAULT_TARG,		\
		TYPE, LTYPE, KEY_TYPE, CONFIG)

#define __XDP2_DFTABLE_MAKE_MATCH_TABLE(NAME, DEFAULT_FUNC,		\
					DEFAULT_ARG, TYPE,		\
					LTYPE, KEY_TYPE, CONFIG)	\
	const struct __xdp2_dtable_entry_func_target			\
		XDP2_JOIN2(NAME, _default_func_target) = {		\
		.func = DEFAULT_FUNC,					\
		.arg = DEFAULT_ARG,					\
	};								\
	___XDP2_DTABLE_MAKE_MATCH_TABLE(NAME,				\
			struct __xdp2_dtable_entry_func_target,	\
			XDP2_JOIN2(NAME, _default_func_target),		\
				   TYPE, LTYPE, KEY_TYPE, CONFIG)

/* Macros for making lookup functions */
#define __XDP2_DTABLE_MAKE_LOOKUP_FUNC_BY_KEY(NAME, TYPE, KEY_TYPE,	\
					      TARG_TYPE)		\
	static inline const TARG_TYPE XDP2_JOIN2(			\
			NAME, _lookup_by_key)(const KEY_TYPE key)	\
	{								\
		struct XDP2_JOIN3(xdp2_dtable_, TYPE, _table) *table =	\
			&XDP2_JOIN2(NAME, _table);			\
		const void *ret;					\
									\
		ret = XDP2_JOIN2(xdp2_dtable_lookup_, TYPE)(		\
				 table, key);				\
		return *((const TARG_TYPE *)ret);			\
	}

#define __XDP2_DTABLE_MAKE_LOOKUP_FUNC(NAME, TYPE, TARG_TYPE)		\
	__XDP2_DTABLE_MAKE_LOOKUP_FUNC_BY_KEY(NAME, TYPE,		\
		struct XDP2_JOIN2(NAME, _key_struct) *, TARG_TYPE)	\
									\
	static inline const TARG_TYPE XDP2_JOIN2(NAME, _lookup)(	\
		XDP2_JOIN2(NAME, _key_arg_t) params) {			\
		struct XDP2_JOIN2(NAME, _key_struct) key;		\
									\
		XDP2_JOIN2(NAME, _make_key)(params, &key);		\
		return XDP2_JOIN2(NAME, _lookup_by_key)(&key);		\
	}

#define __XDP2_DTABLE_MAKE_LOOKUP_FUNC_SKEY(NAME, KEY_ARG_TYPE, TYPE,	\
					    TARG_TYPE)			\
	__XDP2_DTABLE_MAKE_LOOKUP_FUNC_BY_KEY(NAME, TYPE, KEY_ARG_TYPE,	\
					      TARG_TYPE)

#define __XDP2_DFTABLE_MAKE_LOOKUP_FUNC_BY_KEY(NAME, TYPE, KEY_TYPE)	\
	static inline void XDP2_JOIN2(NAME, _lookup_by_key)(		\
	    const KEY_TYPE key, void *arg)				\
	{								\
		struct XDP2_JOIN3(xdp2_dtable_, TYPE,			\
				  _table) *table =			\
			&XDP2_JOIN2(NAME, _table);			\
		const struct __xdp2_dtable_entry_func_target *ftarg =	\
			XDP2_JOIN2(xdp2_dtable_lookup_, TYPE)(		\
					table, key);			\
		if (!ftarg)						\
			ftarg =						\
			    (struct __xdp2_dtable_entry_func_target *)	\
				table->default_target;			\
									\
		ftarg->func(arg, ftarg->arg);				\
	}

#define __XDP2_DFTABLE_MAKE_LOOKUP_FUNC(NAME, TYPE)			\
	__XDP2_DFTABLE_MAKE_LOOKUP_FUNC_BY_KEY(NAME, TYPE,		\
		struct XDP2_JOIN2(NAME, _key_struct) *)			\
									\
	static inline void XDP2_JOIN2(NAME, _lookup)(			\
	    const XDP2_JOIN2(NAME, _key_arg_t) params, void *arg) {	\
		struct XDP2_JOIN2(NAME, _key_struct) key;		\
									\
		XDP2_JOIN2(NAME, _make_key)(params, &key);		\
		XDP2_JOIN2(NAME, _lookup_by_key)(&key, arg);		\
	}

#define __XDP2_DFTABLE_MAKE_LOOKUP_FUNC_SKEY(NAME, KEY_ARG_TYPE, TYPE)	\
	__XDP2_DFTABLE_MAKE_LOOKUP_FUNC_BY_KEY(NAME, TYPE, KEY_ARG_TYPE)

/* Macros for add, delete, and change for statically defined tables */

#define __XDP2_DFTABLE_MAKE_DEL_PLAIN_FUNC(NAME, TYPE)			\
	static inline void XDP2_JOIN2(NAME, _del)(const TYPE key)	\
	{								\
		struct xdp2_dtable_plain_table *table =		\
			&XDP2_JOIN2(NAME, _table);			\
		xdp2_dtable_del_plain(table, key);			\
	}

#define __XDP2_DFTABLE_MAKE_PLAIN_FUNCS(NAME, TYPE)			\
	static inline void XDP2_JOIN2(NAME, _add)(			\
	    const TYPE key, xdp2_dftable_func_t func, void *arg)	\
	{								\
		struct __xdp2_dtable_entry_func_target ftarg = {	\
			.func = func,					\
			.arg = arg,					\
		};							\
		struct xdp2_dtable_plain_table *table =		\
			&XDP2_JOIN2(NAME, _table);			\
									\
		xdp2_dtable_add_plain(table, 0, key, &ftarg);		\
	}								\
									\
	static inline void XDP2_JOIN2(NAME, _change)(			\
	    const TYPE key, xdp2_dftable_func_t func, void *arg)	\
	{								\
		struct __xdp2_dtable_entry_func_target ftarg = {	\
			.func = func,					\
			.arg = arg,					\
		};							\
		struct xdp2_dtable_plain_table *table =		\
			&XDP2_JOIN2(NAME, _table);			\
									\
		xdp2_dtable_change_plain(table, key, &ftarg);		\
	}								\
	__XDP2_DFTABLE_MAKE_DEL_PLAIN_FUNC(NAME, TYPE)


#define __XDP2_DTABLE_MAKE_PLAIN_FUNCS(NAME, TYPE, TARG_TYPE)		\
	static inline void XDP2_JOIN2(NAME, _add)(			\
		const TYPE key, TARG_TYPE targ)				\
	{								\
		struct xdp2_dtable_plain_table *table =		\
			&XDP2_JOIN2(NAME, _table);			\
									\
		xdp2_dtable_add_plain(table, 0, key, &targ);		\
	}								\
									\
	static inline void XDP2_JOIN2(NAME, _change)(			\
	    const TYPE key, TARG_TYPE targ)				\
	{								\
		struct xdp2_dtable_plain_table *table =		\
			&XDP2_JOIN2(NAME, _table);			\
									\
		xdp2_dtable_change_plain(table, key, &targ);		\
	}								\
									\
	__XDP2_DTABLE_MAKE_DEL_PLAIN_FUNC(NAME, TYPE)

#define __XDP2_DTABLE_MAKE_DEL_PLAIN_FUNC(NAME, TYPE)			\
	static inline void XDP2_JOIN2(NAME, _del)(			\
	    const TYPE key)						\
	{								\
		struct xdp2_dtable_plain_table *table =		\
			&XDP2_JOIN2(NAME, _table);			\
		xdp2_dtable_del_plain(table, key);			\
	}

#define __XDP2_DTABLE_MAKE_DEL_TERN_FUNC(NAME, TYPE)			\
	static inline void XDP2_JOIN2(NAME, _del)(			\
	    const TYPE key, const TYPE key_mask,			\
	    unsigned int pos)						\
	{								\
		struct xdp2_dtable_tern_table *table =			\
			&XDP2_JOIN2(NAME, _table);			\
									\
		xdp2_dtable_del_tern(table, (void *)key,		\
				       (void *)key_mask, pos);		\
	}

#define __XDP2_DFTABLE_MAKE_TERN_FUNCS(NAME, TYPE)			\
	static inline void XDP2_JOIN2(NAME, _add)(			\
			const TYPE key, const TYPE key_mask,		\
			unsigned int pos, xdp2_dftable_func_t func,	\
			void *arg)					\
	{								\
		struct __xdp2_dtable_entry_func_target ftarg = {	\
			.func = func,					\
			.arg = arg,					\
		};							\
		struct xdp2_dtable_tern_table *table =			\
			&XDP2_JOIN2(NAME, _table);			\
		xdp2_dtable_add_tern(table, 0, key, key_mask, pos,	\
				      &ftarg);				\
	}								\
									\
	static inline void XDP2_JOIN2(NAME, _change)(			\
			const TYPE key, const TYPE key_mask,		\
			unsigned int pos, xdp2_dftable_func_t func,	\
			void *arg)					\
	{								\
		struct __xdp2_dtable_entry_func_target ftarg = {	\
			.func = func,					\
			.arg = arg,					\
		};							\
		struct xdp2_dtable_tern_table *table =			\
			&XDP2_JOIN2(NAME, _table);			\
		xdp2_dtable_change_tern(table, key, key_mask, pos,	\
					 &ftarg);			\
	}								\
									\
	__XDP2_DTABLE_MAKE_DEL_TERN_FUNC(NAME, TYPE)

#define __XDP2_DTABLE_MAKE_TERN_FUNCS(NAME, TYPE, TARG_TYPE)		\
	static inline void XDP2_JOIN2(NAME, _add)(			\
			const TYPE key, const TYPE key_mask,		\
			unsigned int pos, TARG_TYPE targ)		\
	{								\
		struct xdp2_dtable_tern_table *table =			\
			&XDP2_JOIN2(NAME, _table);			\
									\
		xdp2_dtable_add_tern(table, 0, key, key_mask, pos,	\
				      &targ);				\
	}								\
									\
	static inline void XDP2_JOIN2(NAME, _change)(			\
			const TYPE key, const TYPE key_mask,		\
			unsigned int pos, TARG_TYPE targ)		\
	{								\
		struct xdp2_dtable_tern_table *table =			\
			&XDP2_JOIN2(NAME, _table);			\
									\
		xdp2_dtable_change_tern(table, key, key_mask, pos,	\
					 &targ);			\
	}								\
									\
	__XDP2_DTABLE_MAKE_DEL_TERN_FUNC(NAME, TYPE)


#define __XDP2_DTABLE_MAKE_BY_ID_FUNC(NAME, TYPE)			\
	static inline void XDP2_JOIN2(NAME, del_by_id)(			\
	    int ident)							\
	{								\
		struct XDP2_JOIN3(xdp2_dtable_, TYPE,			\
				  _table) *table =			\
			&XDP2_JOIN2(NAME, _table);			\
		XDP2_JOIN3(xdp2_dtable_del_, TYPE, _by_id)(		\
			table, ident);					\
	}								\
	static inline void XDP2_JOIN2(NAME, change_by_id)(		\
						int ident, void *targ)	\
	{								\
		struct XDP2_JOIN3(xdp2_dtable_, TYPE,			\
				  _table) *table =			\
			&XDP2_JOIN2(NAME, _table);			\
									\
		XDP2_JOIN3(xdp2_dtable_change_, TYPE, _by_id)(		\
			   table, ident, targ);				\
	}

#define __XDP2_DFTABLE_MAKE_BY_ID_FUNC(NAME, TYPE)			\
	static inline void XDP2_JOIN2(NAME, _del_by_id)(int ident)	\
	{								\
		struct XDP2_JOIN3(xdp2_dtable_, TYPE,			\
				  _table) *table =			\
			&XDP2_JOIN2(NAME, _table);			\
		XDP2_JOIN3(xdp2_dtable_del_, TYPE, _by_id)(		\
			table, ident);					\
	}								\
	static inline void XDP2_JOIN2(NAME, _change_by_id)(int ident,	\
				      xdp2_dftable_func_t func,		\
				      void *arg)			\
	{								\
		struct __xdp2_dtable_entry_func_target ftarg = {	\
			.func = func,					\
			.arg = arg,					\
		};							\
		struct XDP2_JOIN3(xdp2_dtable_, TYPE,			\
				  _table) *table =			\
			&XDP2_JOIN2(NAME, _table);			\
		XDP2_JOIN3(xdp2_dtable_change_, TYPE, _by_id)(		\
			table, ident, &ftarg);				\
	}

#define __XDP2_DTABLE_MAKE_DEL_LPM_FUNC(NAME, TYPE)			\
	static inline void XDP2_JOIN2(NAME, _del)(			\
	    const TYPE key, size_t prefix_len)				\
	{								\
		struct xdp2_dtable_lpm_table *table =			\
			&XDP2_JOIN2(NAME, _table);			\
		xdp2_dtable_del_lpm(table, (void *)key, prefix_len);	\
	}

#define __XDP2_DFTABLE_MAKE_LPM_FUNCS(NAME, TYPE)			\
	static inline void XDP2_JOIN2(NAME, _add)(			\
			const TYPE key, size_t prefix_len,		\
			xdp2_dftable_func_t func, void *arg)		\
	{								\
		struct __xdp2_dtable_entry_func_target ftarg = {	\
			.func = func,					\
			.arg = arg,					\
		};							\
		struct xdp2_dtable_lpm_table *table =			\
			&XDP2_JOIN2(NAME, _table);			\
									\
		xdp2_dtable_add_lpm(table, 0, key, prefix_len,		\
				     &ftarg);				\
	}								\
									\
	static inline void XDP2_JOIN2(NAME, _change)(			\
			const TYPE key, size_t prefix_len,		\
			xdp2_dftable_func_t func, void *arg)		\
	{								\
		struct xdp2_dtable_lpm_table *table =			\
			&XDP2_JOIN2(NAME, _table);			\
		struct __xdp2_dtable_entry_func_target ftarg = {	\
			.func = func,					\
			.arg = arg,					\
		};							\
									\
		xdp2_dtable_change_lpm(table, key, prefix_len,		\
					&ftarg);			\
	}								\
									\
	__XDP2_DTABLE_MAKE_DEL_LPM_FUNC(NAME, TYPE)

#define __XDP2_DTABLE_MAKE_LPM_FUNCS(NAME, TYPE, TARG_TYPE)		\
	static inline void XDP2_JOIN2(NAME, _add)(			\
			const TYPE key, size_t prefix_len,		\
			TARG_TYPE targ)					\
	{								\
		struct xdp2_dtable_lpm_table *table =			\
			&XDP2_JOIN2(NAME, _table);			\
									\
		xdp2_dtable_add_lpm(table, 0, key, prefix_len,		\
				     &targ);				\
	}								\
									\
	static inline void XDP2_JOIN2(NAME, _change)(			\
			const TYPE key, size_t prefix_len,		\
			TARG_TYPE targ)					\
	{								\
		struct xdp2_dtable_lpm_table *table =			\
			&XDP2_JOIN2(NAME, _table);			\
									\
		xdp2_dtable_change_lpm(table, key, prefix_len,		\
					&targ);				\
	}								\
									\
	__XDP2_DTABLE_MAKE_DEL_LPM_FUNC(NAME, TYPE)

#define __XDP2_DTABLE_MAKE_KEY_TYPEDEF(NAME, TYPE)			\
	typedef typeof(*(TYPE)0) XDP2_JOIN2(NAME, _key_t);

#define __XDP2_DTABLE_MAKE_TARG_TYPEDEF(NAME, TARG_TYPE)		\
	typedef TARG_TYPE XDP2_JOIN2(NAME, _targ_t);

#define __XDP2_DTABLE_PLAIN_TABLE_DECL(NAME)				\
	extern struct xdp2_dtable_plain_table XDP2_JOIN2(NAME, _table);

#define __XDP2_DTABLE_TERN_TABLE_DECL(NAME)				\
	extern struct xdp2_dtable_tern_table XDP2_JOIN2(NAME, _table);

#define __XDP2_DTABLE_LPM_TABLE_DECL(NAME)				\
	extern struct xdp2_dtable_lpm_table XDP2_JOIN2(NAME, _table);

/* Macros to create service functions for static tables */
#define __XDP2_DFTABLE_LOOKUP_TABLE_PLAIN_FUNCS(NAME, TYPE)		\
	__XDP2_DTABLE_MAKE_KEY_TYPEDEF(NAME, TYPE)			\
	__XDP2_DFTABLE_MAKE_PLAIN_FUNCS(NAME, TYPE)			\
	__XDP2_DFTABLE_MAKE_BY_ID_FUNC(NAME, plain)

#define __XDP2_DFTABLE_LOOKUP_TABLE_TERN_FUNCS(NAME, TYPE)		\
	__XDP2_DTABLE_MAKE_KEY_TYPEDEF(NAME, TYPE)			\
	__XDP2_DFTABLE_MAKE_TERN_FUNCS(NAME, TYPE)			\
	__XDP2_DFTABLE_MAKE_BY_ID_FUNC(NAME, tern)

#define __XDP2_DFTABLE_LOOKUP_TABLE_LPM_FUNCS(NAME, TYPE)		\
	__XDP2_DTABLE_MAKE_KEY_TYPEDEF(NAME, TYPE)			\
	__XDP2_DFTABLE_MAKE_LPM_FUNCS(NAME, TYPE)			\
	__XDP2_DFTABLE_MAKE_BY_ID_FUNC(NAME, lpm)

#define __XDP2_DTABLE_LOOKUP_TABLE_PLAIN_FUNCS(NAME, TYPE, TARG_TYPE)	\
	__XDP2_DTABLE_MAKE_KEY_TYPEDEF(NAME, TYPE)			\
	__XDP2_DTABLE_MAKE_TARG_TYPEDEF(NAME, TARG_TYPE);		\
	__XDP2_DTABLE_MAKE_PLAIN_FUNCS(NAME, TYPE, TARG_TYPE)		\
	__XDP2_DTABLE_MAKE_BY_ID_FUNC(NAME, plain)

#define __XDP2_DTABLE_LOOKUP_TABLE_TERN_FUNCS(NAME, TYPE, TARG_TYPE)	\
	__XDP2_DTABLE_MAKE_KEY_TYPEDEF(NAME, TYPE)			\
	__XDP2_DTABLE_MAKE_TARG_TYPEDEF(NAME, TARG_TYPE);		\
	__XDP2_DTABLE_MAKE_TERN_FUNCS(NAME, TYPE, TARG_TYPE)		\
	__XDP2_DTABLE_MAKE_BY_ID_FUNC(NAME, tern)

#define __XDP2_DTABLE_LOOKUP_TABLE_LPM_FUNCS(NAME, TYPE, TARG_TYPE)	\
	__XDP2_DTABLE_MAKE_KEY_TYPEDEF(NAME, TYPE)			\
	__XDP2_DTABLE_MAKE_TARG_TYPEDEF(NAME, TARG_TYPE);		\
	__XDP2_DTABLE_MAKE_LPM_FUNCS(NAME, TYPE, TARG_TYPE)		\
	__XDP2_DTABLE_MAKE_BY_ID_FUNC(NAME, lpm)

#define XDP2_DFTABLE_ADD_PLAIN(NAME, KEY, FUNC, ARG)			\
do {									\
	XDP2_JOIN2(NAME, _key_t) key = { XDP2_DEPAIR(KEY) };		\
	XDP2_JOIN2(NAME, _add)(&key, FUNC, ARG);			\
} while (0)

#define XDP2_DFTABLE_ADD_LPM(NAME, KEY, PREFIX_LEN, FUNC, ARG)		\
do {									\
	XDP2_JOIN2(NAME, _key_t) key = { XDP2_DEPAIR(KEY) };		\
	XDP2_JOIN2(NAME, _add)(&key, PREFIX_LEN, FUNC, ARG);		\
} while (0)

#define XDP2_DFTABLE_ADD_TERN(NAME, KEY, KEY_MASK, POS, FUNC, ARG)	\
do {									\
	XDP2_JOIN2(NAME, _key_t) key = { XDP2_DEPAIR(KEY) };		\
	XDP2_JOIN2(NAME, _key_t) key_mask = { XDP2_DEPAIR(KEY_MASK) };	\
	XDP2_JOIN2(NAME, _add)(&key, &key_mask, POS, FUNC, ARG);	\
} while (0)

#define XDP2_DTABLE_ADD_PLAIN(NAME, KEY, TARG)				\
do {									\
	XDP2_JOIN2(NAME, _key_t) _key = { XDP2_DEPAIR(KEY) };		\
	XDP2_JOIN2(NAME, _add)(&_key, TARG);				\
} while (0)

#define XDP2_DTABLE_ADD_LPM(NAME, KEY, PREFIX_LEN, TARG)		\
do {									\
	XDP2_JOIN2(NAME, _key_t) key = { XDP2_DEPAIR(KEY) };		\
	XDP2_JOIN2(NAME, _add)(&key, PREFIX_LEN, TARG);			\
} while (0)

#define XDP2_DTABLE_ADD_TERN(NAME, KEY, KEY_MASK, POS, TARG)		\
do {									\
	XDP2_JOIN2(NAME, _key_t) key = { XDP2_DEPAIR(KEY) };		\
	XDP2_JOIN2(NAME, _key_t) key_mask = { XDP2_DEPAIR(KEY_MASK) };	\
	XDP2_JOIN2(NAME, _add)(&key, &key_mask, POS, TARG);		\
} while (0)

/* Include the generated file contain the macros for defining static tables */
#include "xdp2/_dtable.h"

#endif /* __XDP2_DTABLES__ */
