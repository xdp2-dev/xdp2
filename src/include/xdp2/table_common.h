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

#ifndef __XDP2_TABLES_COMMON_H__
#define __XDP2_TABLES_COMMON_H__

/* Common table definitions */

#include <string.h>

#include "xdp2/pmacro.h"
#include "xdp2/switch.h"
#include "xdp2/utility.h"

#define __XDP2_DTABLE_MAKE_KEY_ARG_TYPEDEF(NAME, KEY_ARG_TYPE)		\
	typedef const KEY_ARG_TYPE XDP2_JOIN2(NAME, _key_arg_t);	\
	typedef KEY_ARG_TYPE XDP2_JOIN2(NAME, _key_arg_noconst_t);

#define __XDP2_DTABLE_KEY_ARG_TYPE(NAME) XDP2_JOIN2(NAME, _key_arg_t)

/* Macros to make the keys structure. This structure describes the match
 * key for table lookup. The user visible structure is:
 *
 * struct NAME_key_struct {
 *	<type> <key_name>
 *	<type> <key_name>
 *	...
 * }
 */

/* Key structure and functions for just key field */

#define __XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE1(NARG, ARG_TYPE,	\
						KEY_FIELD, CAST)	\
	typeof(CAST((ARG_TYPE)(0))->KEY_FIELD) XDP2_JOIN2(field_, NARG);

#define __XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE(NARG, ARG_TYPE,		\
						KEY_FIELD)		\
	__XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE1(NARG, ARG_TYPE,	\
						KEY_FIELD,)

#define __XDP2_TABLES_MAKE_KEY_STRUCT_ALL_KEYS(ARG_TYPE, KEY)		\
	XDP2_PMACRO_APPLY_ALL_NARG_CARG(				\
		__XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE,		\
		ARG_TYPE, XDP2_DEPAIR(KEY))

#define __XDP2_TABLES_MAKE_KEY_STRUCT(NAME, KEY)			\
	struct XDP2_JOIN2(NAME, _key_struct) {				\
		__XDP2_TABLES_MAKE_KEY_STRUCT_ALL_KEYS(			\
			XDP2_JOIN2(NAME, _key_arg_noconst_t), KEY)	\
	} __packed;

#define __XDP2_MAKE_KEY_FIELD(NARG, KEY)				\
	memcpy(&key->XDP2_JOIN2(field_, NARG), &params->KEY,		\
	       sizeof(params->KEY));

#define __XDP2_MAKE_KEY_FUNC(NAME, KEY_DEF)				\
static inline void XDP2_JOIN2(NAME, _make_key)				\
	(__XDP2_DTABLE_KEY_ARG_TYPE(NAME) params,			\
	 struct XDP2_JOIN2(NAME, _key_struct) *key)			\
{									\
	memset(key, 0, sizeof(*key));					\
	XDP2_PMACRO_APPLY_ALL_NARG(__XDP2_MAKE_KEY_FIELD,		\
				   XDP2_DEPAIR(KEY_DEF))		\
}

/* Key structure and functions for key field with cast */

#define __XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE_CAST(NARG, ARG_TYPE,	\
						     KEY_FIELD)		\
	__XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE1(NARG, ARG_TYPE,	\
		XDP2_GET_POS_ARG2_1 KEY_FIELD,				\
		XDP2_GET_POS_ARG2_2 KEY_FIELD)

#define __XDP2_TABLES_MAKE_KEY_STRUCT_ALL_KEYS_CAST(ARG_TYPE, KEY)	\
	XDP2_PMACRO_APPLY_ALL_NARG_CARG(				\
		__XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE_CAST,		\
		ARG_TYPE, XDP2_DEPAIR(KEY))

#define __XDP2_TABLES_MAKE_KEY_STRUCT_CAST(NAME, KEY)			\
	struct XDP2_JOIN2(NAME, _key_struct) {				\
		__XDP2_TABLES_MAKE_KEY_STRUCT_ALL_KEYS_CAST(		\
			XDP2_JOIN2(NAME, _key_arg_noconst_t), KEY)	\
	} __packed;

#define __XDP2_MAKE_KEY_FIELD_CAST1(NARG, KEY)				\
	key->XDP2_JOIN2(field_, NARG) = params->KEY;

#define __XDP2_MAKE_KEY_FIELD_NAME_CAST1(NAME, KEY)			\
	key->NAME = params->KEY;

#define __XDP2_MAKE_KEY_FIELD_CAST(NARG, KEY)				\
	__XDP2_MAKE_KEY_FIELD_CAST1(NARG, XDP2_GET_POS_ARG2_1 KEY)

#define __XDP2_MAKE_KEY_FUNC_CAST(NAME, KEY_DEF)			\
static inline void XDP2_JOIN2(NAME, _make_key)				\
	(__XDP2_DTABLE_KEY_ARG_TYPE(NAME) params,			\
	 struct XDP2_JOIN2(NAME, _key_struct) *key)			\
{									\
	memset(key, 0, sizeof(*key));					\
	XDP2_PMACRO_APPLY_ALL_NARG(__XDP2_MAKE_KEY_FIELD_CAST,		\
				   XDP2_DEPAIR(KEY_DEF))		\
}

/* Key structure and functions for key field with name */

#define __XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE_NAME1(ARG_TYPE,		\
						KEY_FIELD, NAME, CAST)	\
	typeof(CAST((ARG_TYPE)(0))->KEY_FIELD) NAME;

#define __XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE_NAME(ARG_TYPE,		\
						     KEY_FIELD)		\
	__XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE_NAME1(			\
		ARG_TYPE,						\
		XDP2_GET_POS_ARG2_1 KEY_FIELD,				\
		XDP2_GET_POS_ARG2_2 KEY_FIELD,)

#define __XDP2_TABLES_MAKE_KEY_STRUCT_ALL_KEYS_NAME(ARG_TYPE, KEY)	\
	XDP2_PMACRO_APPLY_ALL_CARG(					\
		__XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE_NAME,		\
		ARG_TYPE, XDP2_DEPAIR(KEY))

#define __XDP2_TABLES_MAKE_KEY_STRUCT_NAME(NAME, KEY)			\
	struct XDP2_JOIN2(NAME, _key_struct) {				\
		__XDP2_TABLES_MAKE_KEY_STRUCT_ALL_KEYS_NAME(		\
			XDP2_JOIN2(NAME, _key_arg_noconst_t), KEY)	\
	} __packed;

#define __XDP2_MAKE_KEY_FIELD_NAME1(KEY, NAME)				\
	memcpy(&key->NAME, &params->KEY, sizeof(params->KEY));

#define __XDP2_MAKE_KEY_FIELD_NAME(KEY)					\
	__XDP2_MAKE_KEY_FIELD_NAME1(XDP2_GET_POS_ARG2_1 KEY,		\
				    XDP2_GET_POS_ARG2_2 KEY)

#define __XDP2_MAKE_KEY_FUNC_NAME(NAME, KEY_DEF)			\
static inline void XDP2_JOIN2(NAME, _make_key)				\
	(__XDP2_DTABLE_KEY_ARG_TYPE(NAME) params,			\
	 struct XDP2_JOIN2(NAME, _key_struct) *key)			\
{									\
	memset(key, 0, sizeof(*key));					\
	XDP2_PMACRO_APPLY_ALL(__XDP2_MAKE_KEY_FIELD_NAME,		\
				   XDP2_DEPAIR(KEY_DEF))		\
}

/* Key structure and functions for key field with name and cast */

#define __XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE_NAME_CAST(ARG_TYPE,	\
						     KEY_FIELD)		\
	__XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE_NAME1(			\
		ARG_TYPE,						\
		XDP2_GET_POS_ARG3_1 KEY_FIELD,				\
		XDP2_GET_POS_ARG3_2 KEY_FIELD,				\
		XDP2_GET_POS_ARG3_3 KEY_FIELD)

#define __XDP2_TABLES_MAKE_KEY_STRUCT_ALL_KEYS_NAME_CAST(ARG_TYPE, KEY)	\
	XDP2_PMACRO_APPLY_ALL_CARG(					\
		__XDP2_TABLES_MAKE_KEY_STRUCT_FIELD_ONE_NAME_CAST,	\
		ARG_TYPE, XDP2_DEPAIR(KEY))

#define __XDP2_TABLES_MAKE_KEY_STRUCT_NAME_CAST(NAME, KEY)		\
	struct XDP2_JOIN2(NAME, _key_struct) {				\
		__XDP2_TABLES_MAKE_KEY_STRUCT_ALL_KEYS_NAME_CAST(	\
			XDP2_JOIN2(NAME, _key_arg_noconst_t), KEY)	\
	} __packed;

#define __XDP2_MAKE_KEY_FIELD_NAME_CAST(KEY)				\
	__XDP2_MAKE_KEY_FIELD_NAME_CAST1(XDP2_GET_POS_ARG3_2 KEY,	\
					 XDP2_GET_POS_ARG3_1 KEY)

#define __XDP2_MAKE_KEY_FUNC_NAME_CAST(NAME, KEY_DEF)			\
static inline void XDP2_JOIN2(NAME, _make_key)				\
	(__XDP2_DTABLE_KEY_ARG_TYPE(NAME) params,			\
	 struct XDP2_JOIN2(NAME, _key_struct) *key)			\
{									\
	memset(key, 0, sizeof(*key));					\
	XDP2_PMACRO_APPLY_ALL(__XDP2_MAKE_KEY_FIELD_NAME_CAST,		\
				   XDP2_DEPAIR(KEY_DEF))		\
}

#endif /* __XDP2_TABLES_COMMON_H__ */
