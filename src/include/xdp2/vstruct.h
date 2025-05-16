/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2020,2021 Tom Herbert
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

#ifndef __XDP2_VSTRUCT_H__
#define __XDP2_VSTRUCT_H__

#include "xdp2/compiler_helpers.h"
#include "xdp2/utility.h"

/* Variable structures
 *
 * This header file provides the definitions to build variable structures
 *
 * A variable structure is composed of a static portion followed by a variable
 * region. The variable region is made up of two or more variable length arrays
 * of some element type. The sizes of the variable arrays are only known at
 * run time so these cannot be statically defined in the structure
 *
 * Variable structures are described in detail in documentation/vstruct.md
 */

#include <stdbool.h>

#include "xdp2/pmacro.h"

/* Data structures */

struct xdp2_vstruct_def_entry {
	unsigned int num_els;
	unsigned int struct_size;
	unsigned int align;
	char *name;
};

struct xdp2_vstruct_def_generic {
	unsigned int num_els;
	struct xdp2_vstruct_def_entry entries[0]; /* For casting only */
};

struct xdp2_vstruct_generic {
	__u32 offsets[0]; /* For casting only */
};

/* Prototypes for functions in vstruct.c */
const struct xdp2_vstruct_def_entry *xdp2_vstruct_find_def_entry(
		const struct xdp2_vstruct_def_generic *def_vsmap,
		const char *name, int *index);
void xdp2_vstruct_print_one(void *cli, char *indent,
			     struct xdp2_vstruct_generic *vsmap,
			     struct xdp2_vstruct_def_generic *def_vsmap,
			     int index, unsigned int offset, char *vsmap_name,
			     unsigned int base_offset, bool no_zero_size,
			     bool short_output, bool csv, bool label);
void xdp2_vstruct_print_vsmap(void *cli, char *indent,
			       struct xdp2_vstruct_def_generic *def_vsmap,
			       struct xdp2_vstruct_generic *vsmap,
			       char *vsmap_name, unsigned int base_offset,
			       bool no_zero_size, bool short_output, bool csv,
			       bool label);
size_t xdp2_vstruct_instantiate_vsmap(void *_vsmap, void *_def_vsmap,
				       unsigned int num_els, size_t offset);
unsigned int xdp2_vstruct_count_vsmap(struct xdp2_vstruct_generic *vsmap,
				       struct xdp2_vstruct_def_generic
								*def_vsmap,
				       unsigned int *_zero_count);

/* Macros to define a def_vsmap structure */
#define __XDP2_VSTRUCT_VFIELD_STRUCT_DEF_ENTRY(FNAME, ALLOC_TYPE,	\
		CAST_TYPE, NUM_ELS, ALIGNMENT)				\
	struct xdp2_vstruct_def_entry FNAME##_def;

#define __XDP2_VSTRUCT_VFIELD_STRUCT_DEF_ENTRY_DEPAIR(ARG)		\
		__XDP2_VSTRUCT_VFIELD_STRUCT_DEF_ENTRY ARG

#define __XDP2_VSTRUCT_VFIELD_STRUCT_DEF_CAST_TYPE(FNAME, ALLOC_TYPE,	\
		CAST_TYPE, NUM_ELS, ALIGNMENT)				\
	struct {							\
		__u8 __##FNAME##dummy;					\
		CAST_TYPE type[0];					\
	} FNAME##_cast_type __aligned((8));

#define __XDP2_VSTRUCT_VFIELD_STRUCT_DEF_CAST_TYPE_DEPAIR(ARG)		\
		__XDP2_VSTRUCT_VFIELD_STRUCT_DEF_CAST_TYPE ARG

#define __XDP2_VSTRUCT_VFIELD_STRUCT_DEF_ALLOC_TYPE(FNAME, ALLOC_TYPE,	\
		CAST_TYPE, NUM_ELS, ALIGNMENT)				\
	ALLOC_TYPE FNAME##_alloc_type[0];

#define __XDP2_VSTRUCT_VFIELD_STRUCT_DEF_ALLOC_TYPE_DEPAIR(ARG)	\
		__XDP2_VSTRUCT_VFIELD_STRUCT_DEF_ALLOC_TYPE ARG

#define __XDP2_VSTRUCT_MAKE_DEF_VSMAP(STRUCT, ...)			\
	struct STRUCT##_def_vsmap {					\
		struct xdp2_vstruct_def_generic gen;			\
		struct xdp2_vstruct_def_entry __begin_entries[0];	\
		XDP2_PMACRO_APPLY_ALL(					\
			__XDP2_VSTRUCT_VFIELD_STRUCT_DEF_ENTRY_DEPAIR,	\
				       __VA_ARGS__)			\
		struct xdp2_vstruct_def_entry __end_entries[0];	\
		XDP2_PMACRO_APPLY_ALL(					\
		    __XDP2_VSTRUCT_VFIELD_STRUCT_DEF_CAST_TYPE_DEPAIR,	\
				       __VA_ARGS__)			\
		XDP2_PMACRO_APPLY_ALL(					\
		  __XDP2_VSTRUCT_VFIELD_STRUCT_DEF_ALLOC_TYPE_DEPAIR,	\
				       __VA_ARGS__)			\
	};

/* Macros to define a vsmap structure */
#define __XDP2_VSTRUCT_VFIELD_STRUCT_ENTRY(FNAME, ALLOC_TYPE,		\
		CAST_TYPE, NUM_ELS, ALIGNMENT)				\
	__u32 FNAME##_offset;

#define __XDP2_VSTRUCT_VFIELD_STRUCT_ENTRY_DEPAIR(ARG)			\
		__XDP2_VSTRUCT_VFIELD_STRUCT_ENTRY ARG

#define __XDP2_VSTRUCT_VFIELD_STRUCT_TYPE(FNAME, ALLOC_TYPE,		\
		CAST_TYPE, NUM_ELS, ALIGNMENT)				\
	ALLOC_TYPE FNAME##_alloc_type[0];				\
	CAST_TYPE FNAME##_cast_type[0];
#define __XDP2_VSTRUCT_VFIELD_STRUCT_TYPE_DEPAIR(ARG)			\
		__XDP2_VSTRUCT_VFIELD_STRUCT_TYPE ARG

#define __XDP2_VSTRUCT_MAKE_VSMAP(STRUCT, ...)				\
	struct STRUCT##_vsmap {						\
		struct xdp2_vstruct_generic gen;			\
		XDP2_PMACRO_APPLY_ALL(					\
			__XDP2_VSTRUCT_VFIELD_STRUCT_ENTRY_DEPAIR,	\
				       __VA_ARGS__)			\
		XDP2_PMACRO_APPLY_ALL(					\
			__XDP2_VSTRUCT_VFIELD_STRUCT_TYPE_DEPAIR,	\
				       __VA_ARGS__)			\
	};

/* Macros to define a vsconst structure */
#define __XDP2_VSTRUCT_VFIELD_CONST_STRUCT_TYPE(FNAME, ALLOC_TYPE,	\
		CAST_TYPE, NUM_ELS, ALIGNMENT)				\
	CAST_TYPE FNAME##_cast_type[0] __aligned(ALIGNMENT ? : 1);	\
	CAST_TYPE FNAME[0];						\
	ALLOC_TYPE FNAME##_alloc_type[NUM_ELS];

#define __XDP2_VSTRUCT_VFIELD_CONST_STRUCT_TYPE_DEPAIR(ARG)		\
		__XDP2_VSTRUCT_VFIELD_CONST_STRUCT_TYPE ARG

/* Return the number of flexible arrays based on a def_vsmap */
#define XDP2_VSTRUCT_NUM_ELS(STRUCT)					\
	((offsetof(struct STRUCT##_def_vsmap, __end_entries) -		\
		   offsetof(struct STRUCT##_def_vsmap,			\
			    __begin_entries)) /				\
				sizeof(struct xdp2_vstruct_def_entry))

#define __XDP2_VSTRUCT_MAKE_VSCONST(STRUCT, ...)			\
	struct STRUCT##_vsconst {					\
		XDP2_PMACRO_APPLY_ALL(					\
		    __XDP2_VSTRUCT_VFIELD_CONST_STRUCT_TYPE_DEPAIR,	\
				       __VA_ARGS__)			\
	};

/* Make <struct>_instantiate_vsmap_from_config_arg function */
#define __XDP2_VSTRUCT_MAKE_INSTANTIATE(STRUCT, CONFIG_TYPE, ...)	\
__unused() static inline size_t						\
			STRUCT##_instantiate_vsmap_from_config_arg(	\
	const CONFIG_TYPE config,					\
	struct STRUCT##_vsmap *vsmap,					\
	struct STRUCT##_def_vsmap *def_vsmap,				\
	size_t base_struct_size, void *inst_arg)			\
{									\
	unsigned int num_els = XDP2_VSTRUCT_NUM_ELS(STRUCT);		\
	size_t init_offset = base_struct_size;				\
	size_t size;							\
									\
	XDP2_ASSERT(num_els == XDP2_PMACRO_NARGS(__VA_ARGS__),	\
		     "Vstruct instantiate length mismatch for %s: "	\
		     "num_els is but %u, but num_args is %u",		\
		     #STRUCT"_vsmap", num_els,				\
		     XDP2_PMACRO_NARGS(__VA_ARGS__));			\
	def_vsmap->gen.num_els = num_els;				\
	XDP2_PMACRO_APPLY_ALL(						\
		__XDP2_VSTRUCT_VFIELD_STRUCT_SET_ENTRY_DEPAIR,		\
			       __VA_ARGS__)				\
	size = xdp2_vstruct_instantiate_vsmap(vsmap, def_vsmap,	\
				       num_els, init_offset);		\
									\
	return size;							\
}									\
__unused() static inline size_t	STRUCT##_instantiate_vsmap_from_config(	\
	const CONFIG_TYPE config,					\
	struct STRUCT##_vsmap *vsmap,					\
	struct STRUCT##_def_vsmap *def_vsmap,				\
	size_t base_struct_size)					\
{									\
	return STRUCT##_instantiate_vsmap_from_config_arg(config,	\
		vsmap, def_vsmap, base_struct_size, NULL);		\
}

/* Extraction helper macros for printing vsmaps */
#define __XDP2_VSTRUCT_GET_STRUCT_PARAMS(CLI, STRUCT, VSMAP,		\
		DEF_VSMAP, INDENT, OFFSET, NO_SIZE_ZERO_PRINT,		\
		SHORT_OUTPUT, CSV, LABEL) STRUCT
#define __XDP2_VSTRUCT_GET_DEF_VSMAP_PARAMS(CLI, STRUCT, VSMAP,	\
		DEF_VSMAP, INDENT, OFFSET, NO_SIZE_ZERO_PRINT,		\
		SHORT_OUTPUT, CSV, LABEL) DEF_VSMAP
#define __XDP2_VSTRUCT_GET_INDENT_PARAMS(CLI, STRUCT, VSMAP,		\
		DEF_VSMAP, INDENT, OFFSET, NO_SIZE_ZERO_PRINT,		\
		SHORT_OUTPUT, CSV, LABEL) INDENT
#define __XDP2_VSTRUCT_GET_OFFSET_PARAMS(CLI, STRUCT, VSMAP,		\
		DEF_VSMAP, INDENT, OFFSET, NO_SIZE_ZERO_PRINT,		\
		SHORT_OUTPUT, CSV, LABEL) OFFSET
#define __XDP2_VSTRUCT_GET_NO_SIZE_ZERO_PRINT_PARAMS(CLI, STRUCT,	\
		VSMAP, DEF_VSMAP, INDENT, OFFSET, NO_SIZE_ZERO_PRINT,	\
		SHORT_OUTPUT, CSV, LABEL) NO_SIZE_ZERO_PRINT
#define __XDP2_VSTRUCT_GET_SHORT_OUTPUT_PARAMS(CLI, STRUCT, VSMAP,	\
		DEF_VSMAP, INDENT, OFFSET, NO_SIZE_ZERO_PRINT,		\
		SHORT_OUTPUT, CSV, LABEL) SHORT_OUTPUT
#define __XDP2_VSTRUCT_GET_CSV_PARAMS(CLI, STRUCT, VSMAP, DEF_VSMAP,	\
		INDENT,	OFFSET, NO_SIZE_ZERO_PRINT, SHORT_OUTPUT, CSV,	\
		LABEL) CSV
#define __XDP2_VSTRUCT_GET_LABEL_PARAMS(CLI, STRUCT, VSMAP, DEF_VSMAP,	\
		INDENT,	OFFSET, NO_SIZE_ZERO_PRINT, SHORT_OUTPUT, CSV,	\
		LABEL) LABEL
#define __XDP2_VSTRUCT_GET_VSMAP_PARAMS(CLI, STRUCT, VSMAP, DEF_VSMAP,	\
		INDENT,	OFFSET, NO_SIZE_ZERO_PRINT, SHORT_OUTPUT, CSV,	\
		LABEL) VSMAP
#define __XDP2_VSTRUCT_GET_CLI_PARAMS(CLI, STRUCT, VSMAP, DEF_VSMAP,	\
		INDENT,	OFFSET, NO_SIZE_ZERO_PRINT, SHORT_OUTPUT, CSV,	\
		LABEL) CLI

#define __XDP2_VSTRUCT_GET_FIELD_ARGS(FNAME, ALLOC_TYPE, CAST_TYPE,	\
		NUM_ELS, ALIGNMENT) FNAME

/* Print vsmap macro */
#define XDP2_VSTRUCT_PRINT_FIELD_OFFSET(CLI, STRUCT, FIELD, VSMAP,	\
					 DEF_VSMAP, INDENT, OFFSET,	\
					 NO_SIZE_ZERO_PRINT,		\
					 SHORT_OUTPUT, CSV, LABEL)	\
do {									\
	struct xdp2_vstruct_def_generic *_def_vsmap;			\
	struct xdp2_vstruct_generic *_vsmap;				\
	size_t _offset = offsetof(STRUCT, FIELD);			\
									\
	if (NO_SIZE_ZERO_PRINT &&					\
			!sizeof(((STRUCT *)0)->FIELD##_alloc_type))	\
		break;							\
	_def_vsmap = DEF_VSMAP ? &(DEF_VSMAP)->gen : NULL;		\
	_vsmap = VSMAP ? &(VSMAP)->gen : NULL;				\
	xdp2_vstruct_print_one(CLI, INDENT, _vsmap, _def_vsmap, -1,	\
			_offset + OFFSET, #FIELD, OFFSET,		\
			NO_SIZE_ZERO_PRINT, SHORT_OUTPUT, CSV, LABEL);	\
} while (0);

#define XDP2_VSTRUCT_PRINT_VSMAP(CLI, STRUCT, VSMAP, DEF_VSMAP,	\
				  PRINT_INDENT,	NO_ZERO_SIZE_PRINT,	\
				  SHORT_OUTPUT, CSV, LABEL) do {	\
	struct STRUCT##_def_vsmap *_def_vsmap = (DEF_VSMAP);		\
	struct STRUCT##_vsmap *_vsmap = (VSMAP);			\
									\
	xdp2_vstruct_print_vsmap(CLI, PRINT_INDENT,			\
				  &_def_vsmap->gen, &_vsmap->gen,	\
				  #STRUCT, sizeof(struct STRUCT),	\
				  NO_ZERO_SIZE_PRINT,			\
				  SHORT_OUTPUT, CSV, LABEL);		\
} while (0)

/* Print vsconst macros */
#define __XDP2_VSTRUCT_PRINT_VSCONST_ONE(CLI, STRUCT, FIELD, VSMAP,	\
					  DEF_VSMAP, INDENT, OFFSET,	\
					  NO_SIZE_ZERO_PRINT,		\
					  SHORT_OUTPUT, CSV, LABEL)	\
	XDP2_VSTRUCT_PRINT_FIELD_OFFSET(CLI,				\
			struct XDP2_JOIN2(STRUCT, _vsconst), FIELD,	\
			VSMAP, DEF_VSMAP, INDENT, OFFSET,		\
			NO_SIZE_ZERO_PRINT, SHORT_OUTPUT, CSV, LABEL)

#define __XDP2_VSTRUCT_PRINT_VSCONST_ONE_APPLY(PARAMS, ARGS)		\
	__XDP2_VSTRUCT_PRINT_VSCONST_ONE(				\
		__XDP2_VSTRUCT_GET_CLI_PARAMS PARAMS,			\
		__XDP2_VSTRUCT_GET_STRUCT_PARAMS PARAMS,		\
		__XDP2_VSTRUCT_GET_FIELD_ARGS ARGS,			\
		__XDP2_VSTRUCT_GET_VSMAP_PARAMS PARAMS,		\
		__XDP2_VSTRUCT_GET_DEF_VSMAP_PARAMS PARAMS,		\
		__XDP2_VSTRUCT_GET_INDENT_PARAMS PARAMS,		\
		__XDP2_VSTRUCT_GET_OFFSET_PARAMS PARAMS,		\
		__XDP2_VSTRUCT_GET_NO_SIZE_ZERO_PRINT_PARAMS PARAMS,	\
		__XDP2_VSTRUCT_GET_SHORT_OUTPUT_PARAMS PARAMS,		\
		__XDP2_VSTRUCT_GET_CSV_PARAMS PARAMS,			\
		__XDP2_VSTRUCT_GET_LABEL_PARAMS PARAMS)

#define __XDP2_VSTRUCT_MAKE_PRINT_VSCONST(STRUCT, ...)			\
__unused() static inline void STRUCT##_print_vsconst(			\
		void *cli, struct STRUCT##_vsmap *vsmap,		\
		struct STRUCT##_def_vsmap *def_vsmap,			\
		char *indent, size_t offset, bool no_zero_size_print,	\
		bool short_output, bool csv, bool label)		\
{									\
	XDP2_PMACRO_APPLY_ALL_CARG(					\
			__XDP2_VSTRUCT_PRINT_VSCONST_ONE_APPLY,	\
				    (cli, STRUCT, vsmap, def_vsmap,	\
				     indent, offset,			\
				     no_zero_size_print, short_output,	\
				     csv, label), __VA_ARGS__)		\
}

#define XDP2_VSTRUCT_PRINT_VSCONST(CLI, STRUCT, VSMAP, DEF_VSMAP,	\
				    PRINT_INDENT, NO_ZERO_SIZE_PRINT,	\
				    SHORT_OUTPUT, CSV, LABEL)		\
	STRUCT##_print_vsconst(CLI, VSMAP, DEF_VSMAP, PRINT_INDENT,	\
			offsetof(struct STRUCT, vsconst),		\
			NO_ZERO_SIZE_PRINT, SHORT_OUTPUT, CSV, LABEL);

/* Macros to count variable fields */
#define __XDP2_VSTRUCT_COUNT_ONE(STRUCT, FIELD) do {			\
	all_count++;							\
	if (!sizeof(((struct STRUCT##_vsconst *)0)->			\
				XDP2_JOIN2(FIELD, _alloc_type)))	\
		zero_count++;						\
} while (0);

#define __XDP2_VSTRUCT_COUNT_ONE_APPLY(STRUCT, ARGS)			\
	__XDP2_VSTRUCT_COUNT_ONE(STRUCT,				\
		__XDP2_VSTRUCT_GET_FIELD_ARGS ARGS)

#define __XDP2_VSTRUCT_MAKE_COUNT_ZERO_SIZE_VSCONST(STRUCT, ...)	\
__unused() static unsigned int STRUCT##_count_vsconst(			\
					unsigned int *_zero_count)	\
{									\
	unsigned int all_count = 0, zero_count = 0;			\
									\
	XDP2_PMACRO_APPLY_ALL_CARG(__XDP2_VSTRUCT_COUNT_ONE_APPLY,	\
				    STRUCT, __VA_ARGS__)		\
	*_zero_count = zero_count;					\
	return all_count;						\
}

/* Macros to define a XDP2 variable structure. The first argument is a name
 * of a structure type in STRUCT, and this macro creates STRUCT##_vsmap and
 * STRUCT##_def_vsmap. The rest of the arguments are set of pairs with a
 * field name and element type of the variable array
 */
#define XDP2_VSTRUCT_VSMAP(STRUCT, CONFIG_TYPE, ...)			\
	__XDP2_VSTRUCT_MAKE_VSMAP(STRUCT, __VA_ARGS__)			\
	__XDP2_VSTRUCT_MAKE_DEF_VSMAP(STRUCT, __VA_ARGS__)		\
	__XDP2_VSTRUCT_MAKE_INSTANTIATE(STRUCT, CONFIG_TYPE,		\
					 __VA_ARGS__)

/* Macros to define a XDP2 variable structure with constant compilation. The
 * first argument is a name of a structure type in STRUCT, and this macro
 * creates STRUCT##_vsmap and STRUCT##_def_vsmap. The rest of the arguments are
 * set of pairs with a field name and element type of the variable array
 */
#define XDP2_VSTRUCT_VSCONST(STRUCT, CONFIG_TYPE, ...)			\
	XDP2_VSTRUCT_VSMAP(STRUCT, CONFIG_TYPE, __VA_ARGS__)		\
	__XDP2_VSTRUCT_MAKE_VSCONST(STRUCT, __VA_ARGS__)		\
	__XDP2_VSTRUCT_MAKE_PRINT_VSCONST(STRUCT, __VA_ARGS__)		\
	__XDP2_VSTRUCT_MAKE_COUNT_ZERO_SIZE_VSCONST(STRUCT, __VA_ARGS__)

#define __XDP2_VSTRUCT_VFIELD_STRUCT_SET_ENTRY(FNAME, ALLOC_TYPE,	\
		CAST_TYPE, NUM_ELS, ALIGNMENT)				\
	def_vsmap->FNAME##_def.num_els = NUM_ELS;			\
	if (ALIGNMENT)							\
		def_vsmap->FNAME##_def.align = ALIGNMENT;		\
	else								\
		def_vsmap->FNAME##_def.align = offsetof(		\
			typeof(def_vsmap->FNAME##_cast_type), type);	\
	def_vsmap->FNAME##_def.struct_size =				\
			sizeof(def_vsmap->FNAME##_alloc_type[0]);	\
	def_vsmap->FNAME##_def.name = #FNAME;

#define __XDP2_VSTRUCT_VFIELD_STRUCT_SET_ENTRY_DEPAIR(ARG)		\
		__XDP2_VSTRUCT_VFIELD_STRUCT_SET_ENTRY ARG

/* Macros to access variable arrays */
#define __XDP2_VSTRUCT_GETPTR_VSMAP(STRUCT, FIELD, VSMAP)		\
	((typeof(&(VSMAP)->FIELD##_cast_type[0]))			\
		((void *)(STRUCT) + (VSMAP)->FIELD##_offset))

#define XDP2_VSTRUCT_GETPTR_VSMAP(STRUCT, FIELD)			\
	__XDP2_VSTRUCT_GETPTR_VSMAP(STRUCT, FIELD, &(STRUCT)->vsmap)

#define __XDP2_VSTRUCT_GETPTR_VSCONST(STRUCT, FIELD, VSCONST)		\
	((typeof(&(VSCONST)->FIELD##_cast_type[0]))			\
		(&(VSCONST)->FIELD)[0])

#define XDP2_VSTRUCT_GETPTR_VSCONST(STRUCT, FIELD)			\
	__XDP2_VSTRUCT_GETPTR_VSCONST(STRUCT, FIELD, &(STRUCT)->vsconst)

/* Macros to get offset to variable arrays */
#define __XDP2_VSTRUCT_GETOFF_VSMAP(STRUCT, FIELD)			\
	(STRUCT)->FIELD##_offset

#define XDP2_VSTRUCT_GETOFF_VSMAP(STRUCT, FIELD)			\
	__XDP2_VSTRUCT_GETOFF_VSMAP(&STRUCT->vsmap, FIELD)

#define __XDP2_VSTRUCT_GETOFF_VSCONST(STRUCT, FIELD)			\
	offsetof(typeof(*STRUCT), FIELD)

#define XDP2_VSTRUCT_GETOFF_VSCONST(STRUCT, FIELD)			\
	__XDP2_VSTRUCT_GETOFF_VSCONST(STRUCT, vsconst.FIELD)

#endif /* __XDP2_VSTRUCT_H__ */
