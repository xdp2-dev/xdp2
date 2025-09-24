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

#ifndef __XDP2_STABLES_H__
#define __XDP2_STABLES_H__

/* (Better than) P4-like lookup tables in C */

#include <string.h>

#include "xdp2/pmacro.h"
#include "xdp2/table_common.h"
#include "xdp2/utility.h"

/* Macros to make match entries. Match entries are created in an
 * section array
 */
#define __XDP2_SFTABLE_ADD_MATCH(NAME, KEY, KEY_MASK, PREFIX_LEN,	\
				 FUNCTION, COMMON_SIG, NARG, TYPE)	\
	static const struct XDP2_JOIN2(NAME, _table)			\
		*XDP2_UNIQUE_NAME(NAME, _check_table_type) __unused() =	\
				&XDP2_JOIN2(NAME, _table);		\
	static inline void						\
	    XDP2_JOIN3(NAME, _action_, NARG)				\
	    COMMON_SIG {						\
		FUNCTION;						\
	}								\
	static const struct XDP2_JOIN2(NAME, _entry_struct)		\
	    XDP2_SECTION_ATTR(XDP2_JOIN2(NAME, _section_entries))	\
		XDP2_UNIQUE_NAME(__xdp2_stable_entries_,) = {		\
		.key = { XDP2_DEPAIR(KEY) },				\
		.key_mask = { XDP2_DEPAIR(KEY_MASK) },			\
		.action = XDP2_JOIN3(NAME, _action_, NARG),		\
		.prefix_len = PREFIX_LEN,				\
	};

/* User visible macros to add satic entries */

#define XDP2_SFTABLE_ADD_PLAIN_MATCH(NAME, KEY, FUNCTION, COMMON_SIG)	\
	__XDP2_SFTABLE_ADD_MATCH(NAME, KEY, (), 0, FUNCTION,		\
				 COMMON_SIG, XDP2_UNIQUE_NAME(,), plain)

#define XDP2_SFTABLE_ADD_TERN_MATCH(NAME, KEY, KEY_MASK, FUNCTION,	\
				    COMMON_SIG)				\
	__XDP2_SFTABLE_ADD_MATCH(NAME, KEY, KEY_MASK, 0, FUNCTION,	\
				 COMMON_SIG, XDP2_UNIQUE_NAME(tern,), tern)

#define XDP2_SFTABLE_ADD_LPM_MATCH(NAME, KEY, PREFIX_LEN, FUNCTION,	\
				   COMMON_SIG)				\
	__XDP2_SFTABLE_ADD_MATCH(NAME, KEY, (), PREFIX_LEN, FUNCTION,	\
				 COMMON_SIG, XDP2_UNIQUE_NAME(lpm,), lpm)

/* Macros to make match entries. Match entries are created in an
 * section array
 */
#define __XDP2_STABLE_ADD_MATCH(NAME, KEY, KEY_MASK, PREFIX_LEN,	\
				TARGET, TYPE)				\
	static const struct XDP2_JOIN2(NAME, _table)			\
		*XDP2_UNIQUE_NAME(NAME, _check_table_type) __unused() =	\
				&XDP2_JOIN2(NAME, _table);		\
	static const struct XDP2_JOIN2(NAME, _entry_struct)		\
	    XDP2_SECTION_ATTR(XDP2_JOIN2(NAME, _section_entries))	\
		XDP2_UNIQUE_NAME(__xdp2_stable_entries_,) = {		\
		.key = { XDP2_DEPAIR(KEY) },				\
		.key_mask = { XDP2_DEPAIR(KEY_MASK) },			\
		.prefix_len = PREFIX_LEN,				\
		.target = TARGET,					\
	};

/* User visible macros to add satic entries */

#define XDP2_STABLE_ADD_PLAIN_MATCH(NAME, KEY, TARGET)		\
	__XDP2_STABLE_ADD_MATCH(NAME, KEY, (), 0, TARGET, plain)

#define XDP2_STABLE_ADD_TERN_MATCH(NAME, KEY, KEY_MASK, TARGET)	\
	__XDP2_STABLE_ADD_MATCH(NAME, KEY, KEY_MASK, 0, TARGET, tern)

#define XDP2_STABLE_ADD_LPM_MATCH(NAME, KEY, PREFIX_LEN, TARGET)	\
	__XDP2_STABLE_ADD_MATCH(NAME, KEY, (), PREFIX_LEN, TARGET, lpm)

/* Macro to create a table entry structure. This describes on entry in
 * the lookup table that is comprised of a key structure, a key mask structure,
 * a prefix length, and a pointer to an action function
 *
 * struct NAME_entry_struct {
 *	NAME_key_struct key;
 *	NAME_key_struct key_mask;
 *	unsigned int prefix_len;
 *	void (*action)(<common-arg-signature>);
 * }
 */
#define __XDP2_SFTABLE_MAKE_TABLE_ENTRY_STRUCT(NAME, KEY_ARG_TYPE,	\
					       COMMON_ARGS_SIG)		\
	typedef void (*XDP2_JOIN2(NAME, _action_t)) COMMON_ARGS_SIG;	\
	struct XDP2_JOIN2(NAME, _entry_struct) {			\
		KEY_ARG_TYPE key;					\
		KEY_ARG_TYPE key_mask;					\
		unsigned int prefix_len;				\
		void (*action) COMMON_ARGS_SIG;				\
	} XDP2_ALIGN_SECTION;

#define __XDP2_STABLE_MAKE_TABLE_ENTRY_STRUCT(NAME, KEY_ARG_TYPE,	\
						TARG_TYPE)		\
	struct XDP2_JOIN2(NAME, _entry_struct) {			\
		KEY_ARG_TYPE key;					\
		KEY_ARG_TYPE key_mask;					\
		unsigned int prefix_len;				\
		TARG_TYPE target;					\
	} XDP2_ALIGN_SECTION;

/* Macros to make action table structure and definition. This describes the
 * possible action functions in a table. The structure and definition are:
 *
 * struct NAME_actions_struct {
 *	<type> *<action>;
 *	<type> *<action>;
 *	...
 * };
 *
 * struct NAME_action_struct NAME_actions = {
 *	.<acition> = <action>;
 *	.<acition> = <action>;
 *	...
 * };
 */

#define __XDP2_SFTABLE_MAKE_ACTIONS_ONE(ACTION) typeof(ACTION) *ACTION;

#define __XDP2_SFTABLE_MAKE_ACTIONS_SET_ONE(ACTION) .ACTION = ACTION,

#define __XDP2_SFTABLE_MAKE_ACTIONS_TABLE(NAME, ACTIONS)		\
	struct XDP2_JOIN2(NAME, _actions_struct) {			\
		XDP2_PMACRO_APPLY_ALL(__XDP2_SFTABLE_MAKE_ACTIONS_ONE,	\
				      XDP2_DEPAIR(ACTIONS))		\
	};								\
	const struct XDP2_JOIN2(NAME, _actions_struct)			\
			XDP2_JOIN2(NAME, _actions) = {			\
		XDP2_PMACRO_APPLY_ALL(					\
				__XDP2_SFTABLE_MAKE_ACTIONS_SET_ONE,	\
				XDP2_DEPAIR(ACTIONS))			\
	};

/* Macros to make helper action functions. These are stub functions that
 * call the user specified backend functions. These "frontend" functions are
 * called at a match and the arguments are the common arguments for the table.
 * At the backen the user function is called which can have customized
 * arguments where some may be from the common arguments and others might
 * be something else like constants
 */

/* Make stub for default miss function */
#define __XDP2_SFTABLE_MAKE_DEFAULT_FUNC(NAME, COMMON_ARGS_SIG, FUNC)	\
	static inline void XDP2_JOIN2(NAME, _default_func)		\
					COMMON_ARGS_SIG {		\
		XDP2_JOIN2(NAME, _actions).FUNC;			\
	}

/* Make the match table structure and instantiate a definition:
 *
 * struct NAME_TABLE {
 *	const char *name;
 *	void (*default_action)(void);
 * };
 */

#define __XDP2_SFTABLE_MAKE_MATCH_TABLE(NAME, COMMON_ARGS_SIG, TYPE)	\
	XDP2_DEFINE_SECTION(XDP2_JOIN2(NAME, _section_entries),		\
		    struct XDP2_JOIN2(NAME, _entry_struct));		\
	struct XDP2_JOIN2(NAME, _table) {				\
		const char *name;					\
		void (*default_action) COMMON_ARGS_SIG;			\
	};								\
	struct XDP2_JOIN2(NAME, _table) XDP2_JOIN2(NAME, _table) = {	\
		.name = #NAME,						\
		.default_action = XDP2_JOIN2( NAME, _default_func),	\
	};

#define __XDP2_STABLE_MAKE_MATCH_TABLE(NAME, TARG_TYPE, DEFAULT_TARG,	\
				       TYPE)				\
	XDP2_DEFINE_SECTION(XDP2_JOIN2(NAME, _section_entries),		\
		    struct XDP2_JOIN2(NAME, _entry_struct));		\
	struct XDP2_JOIN2(NAME, _table) {				\
		const char *name;					\
		TARG_TYPE default_target;				\
	};								\
	struct XDP2_JOIN2(NAME, _table) XDP2_JOIN2(NAME, _table) = {	\
		.name = #NAME,						\
		.default_target = DEFAULT_TARG,				\
	};

/* Make match entries (either one at a time or a list of them) */

#define __XDP2_SFTABLE_MAKE_ONE_MATCH_ENTRY(NAMESIG, ENTRY)		\
	XDP2_SFTABLE_ADD_PLAIN_MATCH(					\
			XDP2_GET_POS_ARG2_1 NAMESIG,			\
			XDP2_GET_POS_ARG2_1 ENTRY,			\
			XDP2_GET_POS_ARG2_2 ENTRY,			\
			XDP2_GET_POS_ARG2_2 NAMESIG)

#define __XDP2_SFTABLE_MAKE_PLAIN_MATCH_ENTRIES(NAME, ENTRIES,		\
						COMMON_ARGS_SIG)	\
	XDP2_PMACRO_APPLY_ALL_CARG(__XDP2_SFTABLE_MAKE_ONE_MATCH_ENTRY,	\
				   (NAME, COMMON_ARGS_SIG),		\
				   XDP2_DEPAIR(ENTRIES))

#define __XDP2_SFTABLE_MAKE_ONE_TERN_MATCH_ENTRY(NAMESIG, ENTRY)	\
	XDP2_SFTABLE_ADD_TERN_MATCH(					\
			XDP2_GET_POS_ARG2_1 NAMESIG,			\
			XDP2_GET_POS_ARG3_1 ENTRY,			\
			XDP2_GET_POS_ARG3_2 ENTRY,			\
			XDP2_GET_POS_ARG3_3 ENTRY,			\
			XDP2_GET_POS_ARG2_2 NAMESIG)

#define __XDP2_SFTABLE_MAKE_TERN_MATCH_ENTRIES(NAME, ENTRIES,		\
					       COMMON_ARGS_SIG)		\
	XDP2_PMACRO_APPLY_ALL_CARG(					\
			__XDP2_SFTABLE_MAKE_ONE_TERN_MATCH_ENTRY,	\
			(NAME, COMMON_ARGS_SIG), XDP2_DEPAIR(ENTRIES))

#define __XDP2_SFTABLE_MAKE_ONE_LPM_MATCH_ENTRY(NAMESIG, ENTRY)		\
	XDP2_SFTABLE_ADD_LPM_MATCH(					\
			XDP2_GET_POS_ARG2_1 NAMESIG,			\
			XDP2_GET_POS_ARG3_1 ENTRY,			\
			XDP2_GET_POS_ARG3_2 ENTRY,			\
			XDP2_GET_POS_ARG3_3 ENTRY,			\
			XDP2_GET_POS_ARG2_2 NAMESIG)

#define __XDP2_SFTABLE_MAKE_LPM_MATCH_ENTRIES(NAME, ENTRIES,		\
					      COMMON_ARGS_SIG)		\
	XDP2_PMACRO_APPLY_ALL_CARG(					\
			__XDP2_SFTABLE_MAKE_ONE_LPM_MATCH_ENTRY,	\
			(NAME, COMMON_ARGS_SIG), XDP2_DEPAIR(ENTRIES))

/* Make match entries (either one at a time or a list of them) */

#define __XDP2_STABLE_MAKE_ONE_MATCH_ENTRY(NAME, ENTRY)			\
	XDP2_STABLE_ADD_PLAIN_MATCH(					\
			NAME,						\
			XDP2_GET_POS_ARG2_1 ENTRY,			\
			XDP2_GET_POS_ARG2_2 ENTRY)

#define __XDP2_STABLE_MAKE_PLAIN_MATCH_ENTRIES(NAME, ENTRIES)		\
	XDP2_PMACRO_APPLY_ALL_CARG(					\
			__XDP2_STABLE_MAKE_ONE_MATCH_ENTRY, NAME,	\
			XDP2_DEPAIR(ENTRIES))

#define __XDP2_STABLE_MAKE_ONE_TERN_MATCH_ENTRY(NAME, ENTRY)		\
	XDP2_STABLE_ADD_TERN_MATCH(					\
			NAME,						\
			XDP2_GET_POS_ARG3_1 ENTRY,			\
			XDP2_GET_POS_ARG3_2 ENTRY,			\
			XDP2_GET_POS_ARG3_3 ENTRY)

#define __XDP2_STABLE_MAKE_TERN_MATCH_ENTRIES(NAME, ENTRIES)		\
	XDP2_PMACRO_APPLY_ALL_CARG(					\
			__XDP2_STABLE_MAKE_ONE_TERN_MATCH_ENTRY,	\
			NAME, XDP2_DEPAIR(ENTRIES))

#define __XDP2_STABLE_MAKE_ONE_LPM_MATCH_ENTRY(NAME, ENTRY)		\
	XDP2_STABLE_ADD_LPM_MATCH(					\
			NAME,						\
			XDP2_GET_POS_ARG3_1 ENTRY,			\
			XDP2_GET_POS_ARG3_2 ENTRY,			\
			XDP2_GET_POS_ARG3_3 ENTRY)

#define __XDP2_STABLE_MAKE_LPM_MATCH_ENTRIES(NAME, ENTRIES)		\
	XDP2_PMACRO_APPLY_ALL_CARG(					\
			__XDP2_STABLE_MAKE_ONE_LPM_MATCH_ENTRY,		\
			NAME, XDP2_DEPAIR(ENTRIES))

/* Macros to create compare functions */

/* General compare function entry. Enforces argument types */
#define __XDP2_STABLE_COMPARE_FUNC(NAME, KEY_ARG_TYPE)			\
static __unused() inline bool XDP2_JOIN2(NAME, _compare_func)		\
	(const KEY_ARG_TYPE params,					\
	 const struct XDP2_JOIN2(NAME, _entry_struct) *entry)		\
{									\
	struct XDP2_JOIN2(NAME, _key_struct) key;			\
									\
	XDP2_JOIN2(NAME, _make_key)(params, &key);			\
	return XDP2_JOIN2(NAME, _compare_by_key)(&key, entry);		\
}

/* Compare plain or ternary by key */
#define __XDP2_STABLE_COMPARE_FUNC_BY_KEY(NAME, KEY_ARG_TYPE,		\
					  USE_KEY_MASK)			\
static __unused() inline bool XDP2_JOIN2(NAME, _compare_by_key)		\
	(const struct XDP2_JOIN2(NAME, _key_struct) *key,		\
	 const struct XDP2_JOIN2(NAME, _entry_struct) *entry)		\
{									\
	if (USE_KEY_MASK)						\
		return xdp2_compare_tern(&entry->key, key,		\
					 &entry->key_mask,		\
					 sizeof(*key));			\
									\
	return xdp2_compare_equal(&entry->key, key, sizeof(*key));	\
}

/* Compare plain or ternary by skey (basically by_key) */
#define __XDP2_STABLE_COMPARE_FUNC_SKEY(NAME, KEY_ARG_TYPE,		\
					USE_KEY_MASK)			\
static __unused() inline bool XDP2_JOIN2(NAME, _compare_by_key)		\
	(const KEY_ARG_TYPE key,					\
	 const struct XDP2_JOIN2(NAME, _entry_struct)			\
							*entry)		\
{									\
	if (USE_KEY_MASK)						\
		return xdp2_compare_tern(&entry->key, key,		\
					 &entry->key_mask,		\
					 sizeof(*key));			\
									\
	return xdp2_compare_equal(&entry->key, key, sizeof(*key));	\
}

/* Compare one field by longest prefix match by key */
#define __XDP2_STABLE_COMPARE_LPM_FUNC_BY_KEY(NAME, KEY_ARG_TYPE)	\
static __unused() inline bool XDP2_JOIN2(NAME, _compare_by_key)		\
	(const struct XDP2_JOIN2(NAME, _key_struct) *key,		\
	 const struct XDP2_JOIN2(NAME, _entry_struct) *entry)		\
{									\
	return xdp2_compare_prefix(&entry->key, key, entry->prefix_len);\
}

/* Compare one field by longest prefix match by skey			\
 * (basically by_key)
 */
#define __XDP2_STABLE_COMPARE_LPM_FUNC_SKEY(NAME, KEY_ARG_TYPE)		\
static __unused() inline bool XDP2_JOIN2(NAME, _compare_by_key)		\
	(const KEY_ARG_TYPE key,					\
	 const struct XDP2_JOIN2(NAME, _entry_struct) *entry)		\
{									\
	return xdp2_compare_prefix(&entry->key, key, entry->prefix_len);\
}

/* Lookup function for a table. Creates a specialized match function for
 * a table. If a match is founf then the corresponding action function
 * in the entry is called. If no match is found then the default action
 * is called
 */

/* Make lookup functions for plain or ternary tables (selected by
 * USE_KEY_MASK
 */

/* Make lookup functions for plain and ternary by key */
#define __XDP2_SFTABLE_MAKE_LOOKUP_FUNC_BY_KEY(NAME,			\
		COMMON_ARGS_SIG, COMMON_ARGS_LIST, SECTION, TYPE,	\
		KEY_TYPE)						\
	static __unused() inline void XDP2_JOIN2(NAME, _lookup_by_key)(	\
	    const KEY_TYPE key,						\
	    XDP2_DEPAIR(COMMON_ARGS_SIG)) {				\
		const struct XDP2_JOIN2(NAME, _table) *table =		\
			&XDP2_JOIN2(NAME, _table);			\
		const struct XDP2_JOIN2(NAME, _entry_struct) *def_base =\
			XDP2_JOIN2(xdp2_section_base_, SECTION)();	\
		int i, num_els;						\
									\
		num_els = XDP2_JOIN2(xdp2_section_array_size_,		\
				     SECTION)();			\
		for (i = 0; i < num_els; i++) {				\
			if (XDP2_JOIN2(NAME, _compare_by_key)(key,	\
					       &def_base[i])) {		\
				def_base[i].action COMMON_ARGS_LIST;	\
				return;					\
			}						\
		}							\
		table->default_action COMMON_ARGS_LIST;			\
	}								\

#define __XDP2_SFTABLE_MAKE_LOOKUP_FUNC_COMMON(NAME, KEY_ARG_TYPE,	\
					       COMMON_ARGS_SIG,		\
					       COMMON_ARGS_LIST)	\
	static __unused() inline void XDP2_JOIN2(NAME, _lookup)(	\
	    const KEY_ARG_TYPE params, XDP2_DEPAIR(COMMON_ARGS_SIG)) {	\
		struct XDP2_JOIN2(NAME, _key_struct) key;		\
									\
		XDP2_JOIN2(NAME, _make_key)(params, &key);		\
		XDP2_JOIN2(NAME, _lookup_by_key)(			\
			&key, XDP2_DEPAIR(COMMON_ARGS_LIST));		\
	}								\
	__XDP2_STABLE_COMPARE_FUNC(NAME, KEY_ARG_TYPE)

/* Make lookup and compare functions for plain, ternary, and lpm by key */
#define __XDP2_SFTABLE_MAKE_LOOKUP_FUNC(NAME, KEY_ARG_TYPE,		\
		COMMON_ARGS_SIG, COMMON_ARGS_LIST, SECTION,		\
		USE_KEY_MASK, TYPE)					\
	__XDP2_STABLE_COMPARE_FUNC_BY_KEY(NAME, KEY_ARG_TYPE,		\
					  USE_KEY_MASK)			\
	__XDP2_SFTABLE_MAKE_LOOKUP_FUNC_BY_KEY(NAME,			\
		COMMON_ARGS_SIG, COMMON_ARGS_LIST, SECTION, TYPE,	\
		struct XDP2_JOIN2(NAME, _key_struct) *)			\
	__XDP2_SFTABLE_MAKE_LOOKUP_FUNC_COMMON(NAME, KEY_ARG_TYPE,	\
					       COMMON_ARGS_SIG,		\
					       COMMON_ARGS_LIST)

/* Make lookup and compare functions for skey */
#define __XDP2_SFTABLE_MAKE_LOOKUP_FUNC_SKEY(NAME, KEY_ARG_TYPE,	\
					     COMMON_ARGS_SIG,		\
					     COMMON_ARGS_LIST, SECTION,	\
					     USE_KEY_MASK, TYPE)	\
	__XDP2_STABLE_COMPARE_FUNC_SKEY(NAME, KEY_ARG_TYPE,		\
					USE_KEY_MASK)			\
	__XDP2_SFTABLE_MAKE_LOOKUP_FUNC_BY_KEY(NAME,			\
		COMMON_ARGS_SIG, COMMON_ARGS_LIST, SECTION, TYPE,	\
		KEY_ARG_TYPE)

/* Make plain lookup functions */
#define __XDP2_SFTABLE_MAKE_LOOKUP_PLAIN_FUNC(NAME, KEY_ARG_TYPE,	\
					      COMMON_ARGS_SIG,		\
					      COMMON_ARGS_LIST)		\
	__XDP2_SFTABLE_MAKE_LOOKUP_FUNC(NAME, KEY_ARG_TYPE,		\
			COMMON_ARGS_SIG, COMMON_ARGS_LIST,		\
			XDP2_JOIN2(NAME, _section_entries), false, plain)

/* Make plain lookup functions for skey */
#define __XDP2_SFTABLE_MAKE_LOOKUP_PLAIN_FUNC_SKEY(NAME, KEY_ARG_TYPE,	\
						   COMMON_ARGS_SIG,	\
						   COMMON_ARGS_LIST)	\
	__XDP2_SFTABLE_MAKE_LOOKUP_FUNC_SKEY(NAME, KEY_ARG_TYPE,	\
			COMMON_ARGS_SIG, COMMON_ARGS_LIST,		\
			XDP2_JOIN2(NAME, _section_entries), false, plain)

/* Make ternary lookup functions */
#define __XDP2_SFTABLE_MAKE_LOOKUP_TERN_FUNC(NAME, KEY_ARG_TYPE,	\
					     COMMON_ARGS_SIG,		\
					     COMMON_ARGS_LIST)		\
	__XDP2_SFTABLE_MAKE_LOOKUP_FUNC(NAME, KEY_ARG_TYPE,		\
			COMMON_ARGS_SIG, COMMON_ARGS_LIST,		\
			XDP2_JOIN2(NAME, _section_entries), true, tern)

/* Make ternary lookup functions for skey */
#define __XDP2_SFTABLE_MAKE_LOOKUP_TERN_FUNC_SKEY(NAME, KEY_ARG_TYPE,	\
						  COMMON_ARGS_SIG,	\
						  COMMON_ARGS_LIST)	\
	__XDP2_SFTABLE_MAKE_LOOKUP_FUNC_SKEY(NAME, KEY_ARG_TYPE,	\
			COMMON_ARGS_SIG, COMMON_ARGS_LIST,		\
			XDP2_JOIN2(NAME, _section_entries), true, tern)

/* Make lookup functions for longest prefix match by key */
#define __XDP2_SFTABLE_MAKE_LOOKUP_LPM_FUNC_BY_KEY(NAME,		\
		COMMON_ARGS_SIG, COMMON_ARGS_LIST, SECTION, KEY_TYPE)	\
	static __unused() inline void XDP2_JOIN2(NAME, _lookup_by_key)(	\
	    const KEY_TYPE key, XDP2_DEPAIR(COMMON_ARGS_SIG)) {		\
		const struct XDP2_JOIN2(NAME, _table) *table =		\
			&XDP2_JOIN2(NAME, _table);			\
		const struct XDP2_JOIN2(NAME, _entry_struct)		\
		    *def_base = XDP2_JOIN2(xdp2_section_base_,		\
					   SECTION)();			\
		const struct XDP2_JOIN2(NAME, _entry_struct)		\
						*lm_def = NULL;		\
		int longest_match = 0, num_els, i;			\
									\
		num_els = XDP2_JOIN2(xdp2_section_array_size_,		\
				     SECTION)();			\
		for (i = 0; i < num_els; i++) {				\
			if (longest_match >= def_base[i].prefix_len)	\
				continue;				\
			if (XDP2_JOIN2(NAME, _compare_by_key)(key,	\
						 &def_base[i]))	{	\
				longest_match = def_base[i].prefix_len;	\
				lm_def = &def_base[i];			\
			}						\
		}							\
		if (lm_def)						\
			lm_def->action COMMON_ARGS_LIST;		\
		else							\
			table->default_action COMMON_ARGS_LIST;		\
	}

/* Make longest prefix match lookup functions */
#define __XDP2_SFTABLE_MAKE_LOOKUP_LPM_FUNC(NAME, KEY_ARG_TYPE,		\
					   COMMON_ARGS_SIG,		\
					   COMMON_ARGS_LIST)		\
	__XDP2_STABLE_COMPARE_LPM_FUNC_BY_KEY(NAME, KEY_ARG_TYPE)	\
	__XDP2_SFTABLE_MAKE_LOOKUP_LPM_FUNC_BY_KEY(NAME,		\
		COMMON_ARGS_SIG, COMMON_ARGS_LIST,			\
		XDP2_JOIN2(NAME, _section_entries),			\
		struct XDP2_JOIN2(NAME, _key_struct) *)			\
	__XDP2_SFTABLE_MAKE_LOOKUP_FUNC_COMMON(NAME, KEY_ARG_TYPE,	\
					       COMMON_ARGS_SIG,		\
					       COMMON_ARGS_LIST)

/* Make longest prefix match skey lookup functions */
#define __XDP2_SFTABLE_MAKE_LOOKUP_LPM_FUNC_SKEY(NAME, KEY_ARG_TYPE,	\
						 COMMON_ARGS_SIG,	\
						 COMMON_ARGS_LIST)	\
	__XDP2_STABLE_COMPARE_LPM_FUNC_SKEY(NAME, KEY_ARG_TYPE)		\
	__XDP2_SFTABLE_MAKE_LOOKUP_LPM_FUNC_BY_KEY(NAME,		\
		COMMON_ARGS_SIG, COMMON_ARGS_LIST,			\
		XDP2_JOIN2(NAME, _section_entries), KEY_ARG_TYPE)

/* Make lookup functions for plain or ternary tables (selected by
 * USE_KEY_MASK
 */

/* Make lookup functions for plain and ternary by key */
#define __XDP2_STABLE_MAKE_LOOKUP_FUNC_BY_KEY(NAME,			\
		TARG_TYPE, SECTION, TYPE, KEY_TYPE)			\
	static __unused() inline TARG_TYPE XDP2_JOIN2(			\
					NAME, _lookup_by_key)(		\
					const KEY_TYPE key) {	\
		const struct XDP2_JOIN2(NAME, _table) *table =		\
			&XDP2_JOIN2(NAME, _table);			\
		const struct XDP2_JOIN2(NAME, _entry_struct)		\
					*def_base =			\
			XDP2_JOIN2(xdp2_section_base_, SECTION)();	\
		int i, num_els;						\
									\
		num_els = XDP2_JOIN2(xdp2_section_array_size_,		\
				     SECTION)();			\
		for (i = 0; i < num_els; i++) {				\
			if (XDP2_JOIN2(NAME, _compare_by_key)(key,	\
					       &def_base[i]))		\
				return def_base[i].target;		\
		}							\
		return table->default_target;				\
	}								\

#define __XDP2_STABLE_MAKE_LOOKUP_FUNC_COMMON(NAME, KEY_ARG_TYPE,	\
					      TARG_TYPE)		\
	static __unused() inline TARG_TYPE XDP2_JOIN2(NAME, _lookup)(	\
	    const KEY_ARG_TYPE params) {				\
		struct XDP2_JOIN2(NAME, _key_struct) key;		\
									\
		XDP2_JOIN2(NAME, _make_key)(params, &key);		\
		return XDP2_JOIN2(NAME, _lookup_by_key)(&key);		\
	}								\
	__XDP2_STABLE_COMPARE_FUNC(NAME, KEY_ARG_TYPE)

/* Make lookup and compare functions for plain, ternary, and lpm by key */
#define __XDP2_STABLE_MAKE_LOOKUP_FUNC(NAME, KEY_ARG_TYPE,		\
				       TARG_TYPE, SECTION,		\
				       USE_KEY_MASK, TYPE)		\
	__XDP2_STABLE_COMPARE_FUNC_BY_KEY(NAME,	KEY_ARG_TYPE,		\
					  USE_KEY_MASK)			\
	__XDP2_STABLE_MAKE_LOOKUP_FUNC_BY_KEY(NAME, TARG_TYPE, SECTION,	\
					      TYPE,			\
					      struct XDP2_JOIN2(NAME,	\
						_key_struct) *)		\
	__XDP2_STABLE_MAKE_LOOKUP_FUNC_COMMON(NAME, KEY_ARG_TYPE,	\
					      TARG_TYPE)

/* Make lookup and compare functions for skey */
#define __XDP2_STABLE_MAKE_LOOKUP_FUNC_SKEY(NAME, KEY_ARG_TYPE,		\
					    TARG_TYPE, SECTION,		\
					    USE_KEY_MASK, TYPE)		\
	__XDP2_STABLE_COMPARE_FUNC_SKEY(NAME, KEY_ARG_TYPE,		\
					USE_KEY_MASK)			\
	__XDP2_STABLE_MAKE_LOOKUP_FUNC_BY_KEY(NAME, TARG_TYPE, SECTION,	\
					      TYPE, KEY_ARG_TYPE)

/* Make plain lookup functions */
#define __XDP2_STABLE_MAKE_LOOKUP_PLAIN_FUNC(NAME, KEY_ARG_TYPE,	\
					     TARG_TYPE)			\
	__XDP2_STABLE_MAKE_LOOKUP_FUNC(NAME, KEY_ARG_TYPE, TARG_TYPE,	\
			XDP2_JOIN2(NAME, _section_entries), false, plain)

/* Make plain lookup functions for skey */
#define __XDP2_STABLE_MAKE_LOOKUP_PLAIN_FUNC_SKEY(NAME, KEY_ARG_TYPE,	\
						  TARG_TYPE)		\
	__XDP2_STABLE_MAKE_LOOKUP_FUNC_SKEY(NAME, KEY_ARG_TYPE,		\
			TARG_TYPE,					\
			XDP2_JOIN2(NAME, _section_entries), false, plain)

/* Make ternary lookup functions */
#define __XDP2_STABLE_MAKE_LOOKUP_TERN_FUNC(NAME, KEY_ARG_TYPE,		\
					    TARG_TYPE)			\
	__XDP2_STABLE_MAKE_LOOKUP_FUNC(NAME, KEY_ARG_TYPE, TARG_TYPE,	\
			XDP2_JOIN2(NAME, _section_entries), true, tern)

/* Make ternary lookup functions for skey */
#define __XDP2_STABLE_MAKE_LOOKUP_TERN_FUNC_SKEY(NAME, KEY_ARG_TYPE,	\
						 TARG_TYPE)		\
	__XDP2_STABLE_MAKE_LOOKUP_FUNC_SKEY(NAME, KEY_ARG_TYPE,		\
					    TARG_TYPE,			\
			XDP2_JOIN2(NAME, _section_entries), true, tern)

/* Make lookup functions for longest prefix match by key */
#define __XDP2_STABLE_MAKE_LOOKUP_LPM_FUNC_BY_KEY(NAME,			\
		TARG_TYPE, SECTION, KEY_TYPE)				\
	static __unused() inline TARG_TYPE XDP2_JOIN2(			\
					NAME, _lookup_by_key)(		\
					const KEY_TYPE key) {	\
		const struct XDP2_JOIN2(NAME, _table) *table =		\
			&XDP2_JOIN2(NAME, _table);			\
		const struct XDP2_JOIN2(NAME, _entry_struct)		\
		    *def_base = XDP2_JOIN2(xdp2_section_base_,		\
					   SECTION)();			\
		const struct XDP2_JOIN2(NAME, _entry_struct)		\
					*lm_def = NULL;			\
		int longest_match = 0, num_els, i;			\
									\
		num_els = XDP2_JOIN2(xdp2_section_array_size_,		\
				     SECTION)();			\
		for (i = 0; i < num_els; i++) {				\
			if (longest_match >= def_base[i].prefix_len)	\
				continue;				\
			if (XDP2_JOIN2(NAME, _compare_by_key)(key,	\
						 &def_base[i]))	{	\
				longest_match = def_base[i].prefix_len;	\
				lm_def = &def_base[i];			\
			}						\
		}							\
		if (lm_def)						\
			return lm_def->target;				\
		return table->default_target;				\
	}

/* Make longest prefix match lookup functions */
#define __XDP2_STABLE_MAKE_LOOKUP_LPM_FUNC(NAME, KEY_ARG_TYPE,		\
					   TARG_TYPE)			\
	__XDP2_STABLE_COMPARE_LPM_FUNC_BY_KEY(NAME, KEY_ARG_TYPE)	\
	__XDP2_STABLE_MAKE_LOOKUP_LPM_FUNC_BY_KEY(NAME, TARG_TYPE,	\
		XDP2_JOIN2(NAME, _section_entries),			\
		struct XDP2_JOIN2(NAME, _key_struct) *)			\
	__XDP2_STABLE_MAKE_LOOKUP_FUNC_COMMON(NAME, KEY_ARG_TYPE,	\
					      TARG_TYPE)

/* Make longest prefix match skey lookup functions */
#define __XDP2_STABLE_MAKE_LOOKUP_LPM_FUNC_SKEY(NAME, KEY_ARG_TYPE,	\
						TARG_TYPE)		\
	__XDP2_STABLE_COMPARE_LPM_FUNC_SKEY(NAME, KEY_ARG_TYPE)		\
	__XDP2_STABLE_MAKE_LOOKUP_LPM_FUNC_BY_KEY(NAME, TARG_TYPE,	\
		XDP2_JOIN2(NAME, _section_entries), KEY_ARG_TYPE)

/* Macros to create a lookup table.
 *
 * There are variants for plain, ternary, and longest prefix match tbales.
 * The common macro arguments are:
 *	- NAME: User defined name for the table (e.g. "my_table")
 *	- KEY_ARG_TYPE: The argument type for the key. For example, if the
 *		argument type is "struct iphdr *" then the keys can be derived
 *		based on that (e.g. ihl, ros, saddr fields to match)
 *	- KEY_DEF: This defines the key structure for the table. The keys
 *		fields are expressed as a list of three tuples, where each is
 *		comprised of a field in arugument type, match property (exact
 *		or ternary), and an alternate type (the alternate type is used
 *		when the field is a bit field like it would be for the ihl
 *		field in struct iphdr). For example:
 *		(
 *				(ihl, (__u8)),
 *				(tos,),
 *				(saddr,),
 *				(daddr,),
 *				(ttl,),
 *				(protocol,)
 *			)
 *	- COMMON_ARGS_SIG: A signature for common arguments that are passed to
 *		the frontend helper action functions.
 *		(e.g. "(struct my_ctx *ctx)")
 *	- COMMON_ARGS_LIST: The common arguments passed to the frontend helper
 *		action functions. (e.g. "(ctx)")
 *
 *	- ACTION: A list of action functions by name. For example:
 *		(drop, forward, NoAction)
 *	- DEFAULT_ACTION: The invocation of a function called on a lookup
 *		table miss (e.g. "Miss(ctx)")
 *
 * For each of plain, ternary, and longest prefix match there are two
 * sub-variants: one that doesn't define intial entries and one that does.
 * The macros that include a list of entries are denoted by the _ENTS suffix
 * and take and additional parameter.
 *	- ENTRIES: The list of match entries for the table. Each entry is
 *		composed of key data to match against and an action function
 *		invocation that is called on a match. For example, here is
 *		the etries for an IPv4 match table corresponding to the
 *		KEY_DEF show above.
 *
 * Formats for Entries:
 *	- For plain tables an entry has the format as a pair of:
 *		(<key>, <function>)
 *	  For instance, the following defines a list of three entries
 *	  that could be used with XDP2_STABLES_LOOKUP_TABLE_ENTS
 *	    (
 *		((5, 0x3f, 0x00aaaaaa, 0x00aaaabb, 0xff, IPPROTO_TCP),
 *		 forward(ctx, 1)),
 *		((5, 0x0c, 0x00aaaaaa, 0x00aaaabb, 0xff, IPPROTO_UDP),
 *		 forward(ctx, 2)),
 *		((5, 0x0c, 0x00cccccc, 0x00ccccdd, 0x0f, IPPROTO_UDP),
 *		 NoAction(ctx))
 *	    )
 *
 *	- For ternary tables an entry has the format as a tuple of:
 *		(<key>, <key-mask>, <function>)
 *	  For instance, the following defines a list of two tuples
 *	  that could be used with XDP2_STABLES_LOOKUP_TERN_TABLE_ENTS
 *	    (
 *		(
 *		  ( {{{ 0x20, 0x01, 0x00, 0x00, 0x5e, 0xf5, 0x79, 0xfd,
 *		        0x38, 0x0c, 0x1d, 0x57, 0xa6, 0x01, 0x24, 0xfa }}},
 *		    {{{ 0x20, 0x01, 0x06, 0x7c, 0x21, 0x58, 0xa0, 0x19,
 *		        0x00, 0x00 , 0x00, 0x00 , 0x00, 0x00 , 0x0a, 0xce }}},
 *		    __cpu_to_be16(13788), __cpu_to_be16(53104)),
 *		  hit(frame, seqno, 6, 0)),
 *		),
 *		(
 *		  ( {{{ 0x20, 0x01, 0x00, 0x00, 0x5e, 0xf5, 0x79, 0xfd,
 *		        0x38, 0x0c, 0x1d, 0x57, 0xa6, 0x01, 0x24, 0xfa }}},
 *		    {{{ 0x20, 0x01, 0x06, 0x7c, 0x21, 0x58, 0xa0, 0x19,
 *		        0x00, 0x00 , 0x00, 0x00 , 0x00, 0x00 , 0x0a, 0xce }}},
 *		    __cpu_to_be16(13788), __cpu_to_be16(53104)),
 *		  hit(frame, seqno, 6, 0)),
 *		)
 *	    )
 *
 *	- For longest prefic match (LPM) tables an entry has the format as
 *	  a tuple of:
 *		(<key>, <prefix-length>, <function>)
 *	  For instance, the following defines a list of two tuples
 *	  that could be used with XDP2_STABLES_LOOKUP_LPM_TABLE_ENTS
 *	    (
 *		(
 *		  ( {{{ 0x20, 0x01, 0x00, 0x00, 0x5e, 0xf5, 0x79, 0xfd,
 *		        0x38, 0x0c, 0x1d, 0x57, 0xa6, 0x01, 0x24, 0xfa }}} ),
 *		    __cpu_to_be16(13788), __cpu_to_be16(53104)),
 *		  hit(frame, seqno, 6, 0)),
 *		),
 *		(
 *		  ( {{{ 0x20, 0x01, 0x00, 0x00, 0x5e, 0xf5, 0x79, 0xfd,
 *		        0x38, 0x0c, 0x1d, 0x57, 0xa6, 0x01, 0x24, 0xfa }}},
 *		    {{{ 0x20, 0x01, 0x06, 0x7c, 0x21, 0x58, 0xa0, 0x19,
 *		        0x00, 0x00 , 0x00, 0x00 , 0x00, 0x00 , 0x0a, 0xce }}},
 *		    __cpu_to_be16(13788), __cpu_to_be16(53104)),
 *		  hit(frame, seqno, 6, 0)),
 *		)
 *	    )
 */

/* Common macros to create lookup tables */

#define __XDP2_SFTABLE_TABLE_COMMON(NAME, COMMON_ARGS_SIG, ACTIONS,	\
				    DEFAULT_ACTION)			\
	__XDP2_SFTABLE_MAKE_TABLE_ENTRY_STRUCT(NAME,			\
		struct XDP2_JOIN2(NAME, _key_struct),			\
		COMMON_ARGS_SIG)					\
	__XDP2_SFTABLE_MAKE_ACTIONS_TABLE(NAME, ACTIONS)		\
	__XDP2_SFTABLE_MAKE_DEFAULT_FUNC(NAME, COMMON_ARGS_SIG,		\
					 DEFAULT_ACTION)

#define __XDP2_SFTABLE_TABLE_COMMON_CAST(NAME, KEY_ARG_TYPE, KEY_DEF,	\
					 COMMON_ARGS_SIG, ACTIONS,	\
					 DEFAULT_ACTION)		\
	__XDP2_TABLES_MAKE_KEY_STRUCT_CAST(NAME, KEY_ARG_TYPE, KEY_DEF)	\
	__XDP2_MAKE_KEY_FUNC_CAST(NAME, KEY_ARG_TYPE, KEY_DEF)		\
	__XDP2_SFTABLE_TABLE_COMMON(NAME, COMMON_ARGS_SIG, ACTIONS,	\
				    DEFAULT_ACTION)

#define __XDP2_SFTABLE_TABLE_COMMON_NAME(NAME, KEY_ARG_TYPE, KEY_DEF,	\
					 COMMON_ARGS_SIG, ACTIONS,	\
					 DEFAULT_ACTION)		\
	__XDP2_TABLES_MAKE_KEY_STRUCT_NAME(NAME, KEY_ARG_TYPE, KEY_DEF)	\
	__XDP2_MAKE_KEY_FUNC_NAME(NAME, KEY_ARG_TYPE, KEY_DEF)		\
	__XDP2_SFTABLE_TABLE_COMMON(NAME, COMMON_ARGS_SIG, ACTIONS,	\
				    DEFAULT_ACTION)

#define __XDP2_SFTABLE_TABLE_COMMON_NAME_CAST(NAME,			\
		KEY_ARG_TYPE, KEY_DEF, COMMON_ARGS_SIG, ACTIONS,	\
		DEFAULT_ACTION)						\
	__XDP2_TABLES_MAKE_KEY_STRUCT_NAME_CAST(NAME, KEY_ARG_TYPE,	\
						KEY_DEF)		\
	__XDP2_MAKE_KEY_FUNC_NAME_CAST(NAME, KEY_ARG_TYPE, KEY_DEF)	\
	__XDP2_SFTABLE_TABLE_COMMON(NAME, COMMON_ARGS_SIG, ACTIONS,	\
				    DEFAULT_ACTION)

#define __XDP2_STABLE_TABLE_COMMON(NAME, TARG_TYPE)			\
	__XDP2_STABLE_MAKE_TABLE_ENTRY_STRUCT(NAME,			\
		struct XDP2_JOIN2(NAME, _key_struct), TARG_TYPE)

#define __XDP2_STABLE_TABLE_COMMON_CAST(NAME, KEY_ARG_TYPE, KEY_DEF,	\
					TARG_TYPE, DEFAULT_TARG)	\
	__XDP2_TABLES_MAKE_KEY_STRUCT_CAST(NAME, KEY_ARG_TYPE, KEY_DEF)	\
	__XDP2_MAKE_KEY_FUNC_CAST(NAME, KEY_ARG_TYPE, KEY_DEF)		\
	__XDP2_STABLE_TABLE_COMMON(NAME, TARG_TYPE)

#define __XDP2_STABLE_TABLE_COMMON_NAME(NAME, KEY_ARG_TYPE, KEY_DEF,	\
					COMMON_ARGS_SIG, ACTIONS)	\
	__XDP2_TABLES_MAKE_KEY_STRUCT_NAME(NAME, KEY_ARG_TYPE, KEY_DEF)	\
	__XDP2_MAKE_KEY_FUNC_NAME(NAME, KEY_ARG_TYPE, KEY_DEF)		\
	__XDP2_STABLE_TABLE_COMMON(NAME, TARG_TYPE)

#define __XDP2_STABLE_TABLE_COMMON_NAME_CAST(NAME, KEY_ARG_TYPE,	\
					     KEY_DEF, TARG_TYPE)	\
	__XDP2_TABLES_MAKE_KEY_STRUCT_NAME_CAST(NAME, KEY_ARG_TYPE,	\
						KEY_DEF)		\
	__XDP2_MAKE_KEY_FUNC_NAME_CAST(NAME, KEY_ARG_TYPE, KEY_DEF)	\
	__XDP2_STABLE_TABLE_COMMON(NAME, TARG_TYPE)

#include "xdp2/_stable.h"

#endif /* __XDP2_STABLES__ */
