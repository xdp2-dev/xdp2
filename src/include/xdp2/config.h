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

#ifndef __XDP2_CONFIG_H__
#define __XDP2_CONFIG_H__

/* Definitions for XDP2 configuration system
 *
 * This utility allows a common method to define and manage numeric
 * configuration. The set of configuration parameters are declared in a
 * "configuration structure". An instance of the structure is use by the
 * client to pass a set of configuration between functions.
 *
 * The configuration subsystem is described in documentation/config.md
 */

#include <linux/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#include "xdp2/pmacro.h"

/* Descriptor for one configuration parameter */
struct xdp2_config_desc {
	size_t offset;
	size_t size;
	__u64 min;
	__u64 max;
	const char *conf_name;
	const char *text;
	__u64 (*derived_func)(const void *arg);
};

/* Configuration descriptor table */
struct xdp2_config_desc_table {
	const struct xdp2_config_desc *table;
	size_t table_size;
	size_t config_struct_size;
	void (*set_default_config)(void *config);
};

/* Macros to extract values from a list of parameters.
 *
 * A list contains a tuple of:
 *
 *	<prefix>, <struct-name>, <priv>, <use-const>
 */
#define __XDP2_CONFIG_GET_PREFIX(PREFIX, STRUCT_NAME, PRIV, USE_CONST)	\
								PREFIX

#define __XDP2_CONFIG_GET_STRUCT_NAME(PREFIX, STRUCT_NAME, PRIV,	\
				       USE_CONST) STRUCT_NAME

#define __XDP2_CONFIG_GET_PRIV(PREFIX, STRUCT_NAME, PRIV, USE_CONST)	\
								PRIV

#define __XDP2_CONFIG_GET_USE_CONST(PREFIX, STRUCT_NAME, PRIV,		\
				     USE_CONST)	USE_CONST

/* Macros to extract values from a list of attributes for one configuration
 * parameter
 *
 * A list contains:
 *
 *	<name>, <default-value>, <value-type>, <minimum-value>,
 *	<maximum-value>, <text-description>, <derived-function>
 */
#define __XDP2_CONFIG_GET_NAME(NAME, DEFAULT, TYPE, MIN, MAX, TEXT,	\
				DERIVED_FUNC) NAME

#define __XDP2_CONFIG_GET_DEFAULT(NAME, DEFAULT, TYPE, MIN, MAX, TEXT,	\
				   DERIVED_FUNC) DEFAULT

#define __XDP2_CONFIG_GET_TYPE(NAME, DEFAULT, TYPE, MIN, MAX, TEXT,	\
				DERIVED_FUNC) TYPE

#define __XDP2_CONFIG_GET_MIN(NAME, DEFAULT, TYPE, MIN, MAX, TEXT,	\
			       DERIVED_FUNC) MIN

#define __XDP2_CONFIG_GET_MAX(NAME, DEFAULT, TYPE, MIN, MAX, TEXT,	\
			       DERIVED_FUNC) MAX

#define __XDP2_CONFIG_GET_TEXT(NAME, DEFAULT, TYPE, MIN, MAX, TEXT,	\
				DERIVED_FUNC) TEXT

#define __XDP2_CONFIG_GET_DERIVED_FUNC(NAME, DEFAULT, TYPE, MIN, MAX,	\
					TEXT, DERIVED_FUNC) DERIVED_FUNC

/* Output enum values for one parameter */
#define __XDP2_CONFIG_ENUM_ONE(NAME, DEFAULT, TYPE, MIN, MAX, PREFIX)	\
	XDP2_JOIN3(PREFIX, _DEFAULT_, NAME) = DEFAULT,			\
	XDP2_JOIN3(PREFIX, _MIN_, NAME) = MIN,				\
	XDP2_JOIN3(PREFIX, _MAX_,  NAME) = MAX,			\

#define __XDP2_CONFIG_ENUM_ONE_APPLY(PARAMS, ARGS)			\
	__XDP2_CONFIG_ENUM_ONE(					\
		__XDP2_CONFIG_GET_NAME ARGS,				\
		__XDP2_CONFIG_GET_DEFAULT ARGS,			\
		__XDP2_CONFIG_GET_TYPE ARGS,				\
		__XDP2_CONFIG_GET_MIN ARGS,				\
		__XDP2_CONFIG_GET_MAX ARGS,				\
		__XDP2_CONFIG_GET_PREFIX PARAMS)

/* Output enums for plain parameters */
#define __XDP2_CONFIG_ENUM(PARAMS, ...)				\
	XDP2_PMACRO_APPLY_ALL_CARG(__XDP2_CONFIG_ENUM_ONE_APPLY,	\
				    PARAMS, __VA_ARGS__)

/* Macros to create all enums */
#define __XDP2_CONFIG_MAKE_ENUMS(PARAMS, CONFIGS)			\
	enum {								\
		__XDP2_CONFIG_ENUM(PARAMS,				\
				    XDP2_DEPAIR(CONFIGS))		\
	};

/* Output one field for a plain parameter in the configuration structure */
#define __XDP2_CONFIG_STRUCT_ONE(NAME, TYPE)				\
	TYPE NAME;

#define __XDP2_CONFIG_STRUCT_ONE_APPLY(PARAMS, ARGS)			\
	__XDP2_CONFIG_STRUCT_ONE(					\
		__XDP2_CONFIG_GET_NAME ARGS,				\
		__XDP2_CONFIG_GET_TYPE ARGS)

/* Output fields for the plain parameters in the configuration structure */
#define __XDP2_CONFIG_STRUCT(PARAMS, ...)				\
	XDP2_PMACRO_APPLY_ALL_CARG(__XDP2_CONFIG_STRUCT_ONE_APPLY,	\
				    PARAMS, __VA_ARGS__)

/* Macros to create all configuration structures */
#define __XDP2_CONFIG_MAKE_CONFIG_STRUCT_X(PARAMS, CONFIGS,		\
					    STRUCT_NAME, PRIV)		\
	struct STRUCT_NAME {						\
		void *priv;						\
		__XDP2_CONFIG_STRUCT(PARAMS, XDP2_DEPAIR(CONFIGS))	\
	};

/* Output fields for the plain parameters in the configuration structure */
#define __XDP2_CONFIG_MAKE_CONFIG_STRUCT(PARAMS, CONFIGS)		\
	__XDP2_CONFIG_MAKE_CONFIG_STRUCT_X(PARAMS, CONFIGS,		\
				__XDP2_CONFIG_GET_STRUCT_NAME PARAMS,	\
				__XDP2_CONFIG_GET_PRIV PARAMS)

/* Output setting one entry in configuration structure to a default value */
#define __XDP2_CONFIG_DEFAULT_ONE(NAME, PREFIX)			\
	config->NAME = XDP2_JOIN3(PREFIX, _DEFAULT_, NAME);

#define __XDP2_CONFIG_DEFAULT_ONE_APPLY(PARAMS, ARGS)			\
	__XDP2_CONFIG_DEFAULT_ONE(					\
		__XDP2_CONFIG_GET_NAME ARGS,				\
		__XDP2_CONFIG_GET_PREFIX PARAMS)

/* Output setting entries in configuration structure to default values */
#define __XDP2_CONFIG_DEFAULT_X(PARAMS, ...)				\
	XDP2_PMACRO_APPLY_ALL_CARG(__XDP2_CONFIG_DEFAULT_ONE_APPLY,	\
				    PARAMS, __VA_ARGS__)

#define __XDP2_CONFIG_DEFAULT(PARAMS, CONFIGS)				\
	__XDP2_CONFIG_DEFAULT_X(PARAMS, XDP2_DEPAIR(CONFIGS))

/* Macros for creating set_default_config function */
#define __XDP2_CONFIG_MAKE_CONFIG_DEFAULT_X_PREAMBLE(PREFIX,		\
						      STRUCT_NAME)	\
	__unused() static inline void XDP2_JOIN2(PREFIX,		\
				       _set_default_config_priv)(	\
		 void *config_x, void *priv)				\
	{								\
		struct STRUCT_NAME *config = config_x;			\
									\
		config->priv = priv;

#define __XDP2_CONFIG_MAKE_CONFIG_DEFAULT_X_POSTAMBLE(PREFIX)		\
	}								\
	__unused() static inline void XDP2_JOIN2(PREFIX,		\
						_set_default_config)(	\
				 void *config_x)			\
	{								\
		XDP2_JOIN2(PREFIX, _set_default_config_priv)(		\
						config_x, NULL);	\
	}

#define __XDP2_CONFIG_MAKE_CONFIG_DEFAULT_X(PARAMS, CONFIGS,		\
					     PREFIX, STRUCT_NAME)	\
	__XDP2_CONFIG_MAKE_CONFIG_DEFAULT_X_PREAMBLE(PREFIX,		\
						      STRUCT_NAME)	\
		__XDP2_CONFIG_DEFAULT(PARAMS, CONFIGS)			\
	__XDP2_CONFIG_MAKE_CONFIG_DEFAULT_X_POSTAMBLE(PREFIX)

#define __XDP2_CONFIG_MAKE_CONFIG_DEFAULT(PARAMS, CONFIGS)		\
	__XDP2_CONFIG_MAKE_CONFIG_DEFAULT_X(PARAMS, CONFIGS,		\
			   __XDP2_CONFIG_GET_PREFIX PARAMS,		\
			   __XDP2_CONFIG_GET_STRUCT_NAME PARAMS)

/* Macros to create function to get a configuration parameter value. If
 * USE_CONST is set then the constant default value is returned, else the
 * value in the configuration parameter structure or the derived value
 * is returned
 */
#define __XDP2_CONFIG_GET_FUNC_ONE(NAME, TYPE, DERIVED_FUNC,		\
				    PREFIX, STRUCT_NAME, USE_CONST)	\
	__unused() static inline TYPE XDP2_JOIN3(PREFIX, _, NAME)(	\
			const struct STRUCT_NAME *config)		\
	{								\
		if (USE_CONST)						\
			return XDP2_JOIN3(PREFIX, _DEFAULT_, NAME);	\
		else if (config->NAME)					\
			return config->NAME;				\
		else							\
			return XDP2_JOIN3(PREFIX, _DERIVED_FUNC_,	\
				   DERIVED_FUNC)(config);		\
	}

#define __XDP2_CONFIG_GET_FUNC_ONE_APPLY(PARAMS, ARGS)			\
	__XDP2_CONFIG_GET_FUNC_ONE(					\
		__XDP2_CONFIG_GET_NAME ARGS,				\
		__XDP2_CONFIG_GET_TYPE ARGS,				\
		__XDP2_CONFIG_GET_DERIVED_FUNC ARGS,			\
		__XDP2_CONFIG_GET_PREFIX PARAMS,			\
		__XDP2_CONFIG_GET_STRUCT_NAME PARAMS,			\
		__XDP2_CONFIG_GET_USE_CONST PARAMS)

/* Macro to create the functions for getting configuration parameters */
#define __XDP2_CONFIG_GET_FUNC(PARAMS, ...)				\
	XDP2_PMACRO_APPLY_ALL_CARG(__XDP2_CONFIG_GET_FUNC_ONE_APPLY,	\
				    PARAMS, __VA_ARGS__)

/* Macro to create all the parameter get functions */
#define __XDP2_CONFIG_MAKE_GET_FUNC(PARAMS, CONFIGS)			\
	__XDP2_CONFIG_GET_FUNC(PARAMS, XDP2_DEPAIR(CONFIGS))

/* Macro for creating one configuration parameter descriptor entry */
#define __XDP2_CONFIG_TABLE_ONE(NAME, TYPE, MIN, MAX, DERIVED_FUNC,	\
				 TEXT, PREFIX, STRUCT_NAME)		\
{									\
	.offset = offsetof(struct STRUCT_NAME, NAME),			\
	.min = MIN,							\
	.max = MAX,							\
	.conf_name = XDP2_STRING_IT(NAME),				\
	.text = TEXT,							\
	.size = sizeof(TYPE),						\
	.derived_func = XDP2_JOIN3(PREFIX, _DERIVED_FUNC_,		\
				    DERIVED_FUNC),			\
},

#define __XDP2_CONFIG_TABLE_ONE_APPLY(PARAMS, ARGS)			\
	__XDP2_CONFIG_TABLE_ONE(					\
		__XDP2_CONFIG_GET_NAME ARGS,				\
		__XDP2_CONFIG_GET_TYPE ARGS,				\
		__XDP2_CONFIG_GET_MIN ARGS,				\
		__XDP2_CONFIG_GET_MAX ARGS,				\
		__XDP2_CONFIG_GET_DERIVED_FUNC ARGS,			\
		__XDP2_CONFIG_GET_TEXT ARGS,				\
		__XDP2_CONFIG_GET_PREFIX PARAMS,			\
		__XDP2_CONFIG_GET_STRUCT_NAME PARAMS)

/* Macro to create configuration descriptors for parameters */
#define __XDP2_CONFIG_TABLE(PARAMS, ...)				\
	XDP2_PMACRO_APPLY_ALL_CARG(					\
			__XDP2_CONFIG_TABLE_ONE_APPLY,			\
			PARAMS, __VA_ARGS__)

/* Macros for creating the configuration descriptors and the configuration
 * descriptors table
 */
#define __XDP2_CONFIG_MAKE_CONFIG_TABLE_X_PREAMBLE(STRUCT_NAME)	\
static const struct xdp2_config_desc XDP2_JOIN2(STRUCT_NAME,		\
						  _entries)[]		\
						__unused() = {

#define __XDP2_CONFIG_MAKE_CONFIG_TABLE_X_POSTAMBLE(STRUCT_NAME,	\
						     PREFIX)		\
};									\
static const struct xdp2_config_desc_table				\
					XDP2_JOIN2(STRUCT_NAME,	\
						    _table) = {		\
	.table = XDP2_JOIN2(STRUCT_NAME, _entries),			\
	.table_size = ARRAY_SIZE(XDP2_JOIN2(STRUCT_NAME, _entries)),	\
	.config_struct_size = sizeof(struct STRUCT_NAME),		\
	.set_default_config = XDP2_JOIN2(PREFIX, _set_default_config),	\
};

#define __XDP2_CONFIG_MAKE_CONFIG_TABLE_X(PARAMS, CONFIGS,		\
					   STRUCT_NAME,	PREFIX,		\
					   GET_DERIVED_BY_NAME)		\
	__XDP2_CONFIG_MAKE_CONFIG_TABLE_X_PREAMBLE(STRUCT_NAME)	\
		__XDP2_CONFIG_TABLE(PARAMS, XDP2_DEPAIR(CONFIGS))	\
	__XDP2_CONFIG_MAKE_CONFIG_TABLE_X_POSTAMBLE(STRUCT_NAME, PREFIX)

/* Macro for making all the configuration tables */
#define __XDP2_CONFIG_MAKE_CONFIG_TABLE(PARAMS, CONFIGS)		\
	__XDP2_CONFIG_MAKE_CONFIG_TABLE_X(PARAMS, CONFIGS,		\
				   __XDP2_CONFIG_GET_STRUCT_NAME	\
							PARAMS,		\
				   __XDP2_CONFIG_GET_PREFIX PARAMS,	\
				   __XDP2_CONFIG_GET_DERIVED_BY_NAME	\
							PARAMS)

/* Null derived function */
#define __XDP2_CONFIG_MAKE_NULL_DERIVED_FUNC(PREFIX)			\
	__unused() static inline __u64 XDP2_JOIN2(PREFIX,	\
				_DERIVED_FUNC_)(const void *config) {	\
		return 0;						\
	}

#define __XDP2_CONFIG_MAKE_NULL_DERIVED_FUNCX(PARAMS)			\
	__XDP2_CONFIG_MAKE_NULL_DERIVED_FUNC(				\
			__XDP2_CONFIG_GET_PREFIX PARAMS)

/* Main macro to make all structures and functions for configuration
 *
 * There are three arguments:
 *
 * <params> A list of parameter tuples
 * <configs> A list of plain parameter configuration tuples
 */
#define XDP2_CONFIG_MAKE_ALL_CONFIGS(PARAMS, CONFIGS)			\
	__XDP2_CONFIG_MAKE_ENUMS(PARAMS, CONFIGS)			\
	__XDP2_CONFIG_MAKE_CONFIG_STRUCT(PARAMS, CONFIGS)		\
	__XDP2_CONFIG_MAKE_NULL_DERIVED_FUNCX(PARAMS)			\
	__XDP2_CONFIG_MAKE_CONFIG_DEFAULT(PARAMS, CONFIGS)		\
	__XDP2_CONFIG_MAKE_GET_FUNC(PARAMS, CONFIGS)			\
	__XDP2_CONFIG_MAKE_CONFIG_TABLE(PARAMS, CONFIGS)

/* Prototypes for utility functions in config_functions.c */

/* Prototypes for parse config functions */

int xdp2_config_parse_options(const struct xdp2_config_desc_table
								*config_table,
			       void *config, char *text);
int xdp2_config_parse_options_file(const struct xdp2_config_desc_table
								*config_table,
				    void *config, const char *filename);
int xdp2_config_parse_options_with_defaults(
		const struct xdp2_config_desc_table *config_table,
		void *config, char *text);

/* Prototype for check config function */

void xdp2_config_check_config_one(const struct xdp2_config_desc_table
								*config_table,
				   const char *text, bool *rval,
				   const char *conf_name, unsigned int value);
/* Prototypes for print config functions */

void xdp2_config_print_config(const struct xdp2_config_desc_table
							*config_table,
			       void *cli, char *indent, const void *config);
void xdp2_config_print_config_csv(const struct xdp2_config_desc_table
								*config_table,
				   void *cli, const void *config, bool key);
void xdp2_config_print_config_csv_short(const struct xdp2_config_desc_table
								*config_table,
					 void *cli, const void *config,
					 char sep, bool key);
void xdp2_config_print_config_info(const struct xdp2_config_desc_table
							*config_table,
				    void *cli, char *indent, bool highlight);
void xdp2_config_print_config_help(const struct xdp2_config_desc_table
							*config_table,
				    void *cli, bool highlight);
void xdp2_print_config_csv(const struct xdp2_config_desc_table *config_table,
			    void *cli, void *config, bool key);

void xdp2_config_print_derived_config(
		const struct xdp2_config_desc_table *config_table,
		void *cli, char *indent, const void *config, char *conf_name,
		unsigned long long derived_val,
		unsigned int derived_low_water_mark);

void xdp2_config_print_config_info(const struct xdp2_config_desc_table
							*config_table,
				    void *cli, char *indent, bool highlight);

#endif /* __XDP2_CONFIG_H__ */
