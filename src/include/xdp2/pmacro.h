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

#ifndef __XDP2_MACRO_OPS_H__
#define __XDP2_MACRO_OPS_H__

/* Various macro operations */

/* XDP2_PMACRO_NARGS returns the number of argument in the variable list
 * input argument
 */

/* XDP2_PMACRO_APPLY_ALL applies a macro over a list of variable arguments.
 * The first argument is a macro followed by a variable length a list
 * arguments
 *
 * As an example consider:
 *
 *    XDP2_PMACRO_APPLY_ALL(P, a, b, c, d)
 *
 * The logical effect is:
 *    P(a)
 *    P(b)
 *    P(c)
 *    P(d)
 */

#include "_pmacro_gen.h"

#define __XDP2_PMACRO_APPLY_ALL_3(ACT, NUM, ...)			\
			__XDP2_PMACRO_APPLY##NUM(ACT, __VA_ARGS__)

#define __XDP2_PMACRO_APPLY_ALL_2(ACT, NUM, ...)			\
			__XDP2_PMACRO_APPLY_ALL_3(ACT, NUM, __VA_ARGS__)

#define XDP2_PMACRO_APPLY_ALL(ACT, ...)				\
	__XDP2_PMACRO_APPLY_ALL_2(ACT,					\
				   XDP2_PMACRO_NARGS(__VA_ARGS__),	\
				   __VA_ARGS__)

#define __XDP2_PMACRO_APPLY_ALL_3_NARG(ACT, NUM, ...)			\
			__XDP2_PMACRO_APPLY##NUM##_NARG(ACT, __VA_ARGS__)

#define __XDP2_PMACRO_APPLY_ALL_2_NARG(ACT, NUM, ...)			\
			__XDP2_PMACRO_APPLY_ALL_3_NARG(ACT, NUM, __VA_ARGS__)

#define XDP2_PMACRO_APPLY_ALL_NARG(ACT, ...)				\
	__XDP2_PMACRO_APPLY_ALL_2_NARG(ACT,				\
				   XDP2_PMACRO_NARGS(__VA_ARGS__),	\
				   __VA_ARGS__)

#define __XDP2_PMACRO_APPLY_ALL_3_CARG(ACT, ARG, NUM, ...)		\
			__XDP2_PMACRO_APPLY##NUM##_CARG(ACT, ARG,	\
			__VA_ARGS__)

#define __XDP2_PMACRO_APPLY_ALL_2_CARG(ACT, ARG, NUM, ...)		\
			__XDP2_PMACRO_APPLY_ALL_3_CARG(ACT, ARG,	\
							NUM, __VA_ARGS__)

#define XDP2_PMACRO_APPLY_ALL_CARG(ACT, ARG, ...)			\
	__XDP2_PMACRO_APPLY_ALL_2_CARG(ACT, ARG,			\
				   XDP2_PMACRO_NARGS(__VA_ARGS__),	\
				   __VA_ARGS__)

#define __XDP2_PMACRO_APPLY_ALL_3_NARG_CARG(ACT, ARG, NUM, ...)	\
			__XDP2_PMACRO_APPLY##NUM##_NARG_CARG(ACT, ARG,	\
			__VA_ARGS__)

#define __XDP2_PMACRO_APPLY_ALL_2_NARG_CARG(ACT, ARG, NUM, ...)	\
			__XDP2_PMACRO_APPLY_ALL_3_NARG_CARG(ACT, ARG,	\
							NUM, __VA_ARGS__)

#define XDP2_PMACRO_APPLY_ALL_NARG_CARG(ACT, ARG, ...)			\
	__XDP2_PMACRO_APPLY_ALL_2_NARG_CARG(ACT, ARG,			\
				   XDP2_PMACRO_NARGS(__VA_ARGS__),	\
				   __VA_ARGS__)

#define __XDP2_PMACRO_LASTARG_3(NUM, ...)				\
	__XDP2_PMACRO_LASTARG##NUM(__VA_ARGS__)

#define __XDP2_PMACRO_LASTARG_2(NUM, ...)				\
	__XDP2_PMACRO_LASTARG_3(NUM, __VA_ARGS__)

#define XDP2_PMACRO_LASTARG(...)					\
	__XDP2_PMACRO_LASTARG_2(XDP2_PMACRO_NARGS(__VA_ARGS__), __VA_ARGS__)

#define __XDP2_PMACRO_APPLY0(ACT, DUMMY)

#endif /* __XDP2_MACRO_OPS_H__ */
