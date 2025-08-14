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

#ifndef __XDP2_SWITCH_H__
#define __XDP2_SWITCH_H__

#include "xdp2/pmacro.h"

/* Definitions for the XDP2 switch facility. This include functions to
 * compare two structures (pointers) for equality, with a mask for a ternary
 * compare, and a prefix match for some length
 *
 * The XDP2 switch facility allows an rich syntax beyond a C switch statement
 * to allow case values to things other than ordinal types. Also, the
 * evaluation function can be for equality, a ternary match, or a prefix
 * match by prefix length
 *
 *  Here is an example that switches on an Ethernet header:
 *
 * XDP2_SWITCH_START(&ehdr)
 *	XDP2_SWITCH_CASE_EQUAL(ehdr2) {
 *		printf("Hello world\n");
 *		break;
 *	}
 *	XDP2_SWITCH_CASE_EQUAL_CONST(({ 0x0, 0x11, 0x11, 0x11, 0x11, 0x11 },
 *				      { 0x0, 0x11, 0x11, 0x11, 0x11, 0x22 },
 *				      ETH_P_IP)) {
 *		printf("Match 1\n");
 *		break;
 *	}
 *	XDP2_SWITCH_CASE_TERN(ehdr2, ehdr2)
 *		printf("Match 2\n");
 *	XDP2_SWITCH_CASE_TERN_CONST(({ 0x0, 0x11, 0x11, 0x11, 0x11, 0x11 },
 *				     { 0x0, 0x11, 0x11, 0x11, 0x11, 0x22 },
 *				     ETH_P_IP),
 *				    ({ 0x0, 0x11, 0x11, 0x11, 0x11, 0x11 },
 *				     { 0x0, 0x11, 0x11, 0x11, 0x11, 0x22 },
 *				     ETH_P_IP)) break;
 *	XDP2_SWITCH_CASE_PREFIX(ehdr2, 23)
 *		break;
 *	XDP2_SWITCH_CASE_PREFIX_CONST(({ 0x0, 0x11, 0x11, 0x11, 0x11, 0x11 },
 *				       { 0x0, 0x11, 0x11, 0x11, 0x11, 0x22 },
 *				       ETH_P_IP), 43) {
 *		printf("Match 3\n");
 *		break;
 *	}
 *	XDP2_SWITCH_DEFAULT() printf("No hits\n");
 * XDP2_SWITCH_END();
 *
 * An XDP2 switch block starts with XDP2_SWITCH_START with an argument that
 * is the target value. In the case of a pointer to a structure the pointer
 * should be dereferenced as the argument. Note that XDP2_SWITCH_START opens
 * a block, it must be followed by and XDP2_SWITCH_CASE* or XDP2_SWITCH_DEFAULT.
 *
 * Case declarations have the form XDP2_SWITCH_CASE_* or
 * XDP2_SWITCH_CASE_*_CONST where * is one of EQUAL, TERN, or PREFIX. The
 * constant variety allow the case target to be specified by a constant
 * data structure.
 *
 * Unlike a switch statement, the cases in XDP2 switch are well ordered and
 * there may be more than one match for a target. "break" work as expected
 * and will break out of the switch block so that execution continues after
 * XDP2_SWITCH_END. At a fall-through (no break for a case) the next case
 * is evaluated (as opposed to executing the following case code as would
 * be done in a C switch statement.
 *
 * XDP2_SWITCH_DEFAULT executes a block of code in the context of the
 * switch block. This can be thought of a defining a case that always
 * evaluates to true. XDP2_SWITCH_DEFAULT is normally used at the end of
 * the switch block to get the same effect as the default case in a C
 * switch statement, however it may also be inserted else where is a switch
 * block.
 *
 * The switch block is terminated by XDP2_SWITCH_END(). A semicolon should
 * follow it.
 */

#include <string.h>

#include "xdp2/utility.h"

/* Function to compare two fields */

/* Compare two fields for equality */
static inline bool xdp2_compare_equal(const void *f1, const void *f2,
				      size_t len)
{
	return !memcmp(f1, f2, len);
}

/* Compare tenary fields by apply the key mask */
static inline bool xdp2_compare_tern(const void *_f1,
		const void *_f2, const void *_mask, size_t len)
{
	__u8 mask, c1, c2;
	int i;

	for (i = 0; i < len; i++) {
		c1 = ((__u8 *)_f1)[i];
		c2 = ((__u8 *)_f2)[i];
		mask = ((__u8 *)_mask)[i];
		if ((c1 ^ c2) & mask)
			return false;
	}

	return true;
}

/* Compare fields by longest prefix match */
static inline bool xdp2_compare_prefix(const void *_f1,
		const void *_f2, size_t prefix_len)
{
	int div = prefix_len / 8;
	int mod = prefix_len % 8;
	__u8 mask, c1, c2;

	if (memcmp(_f1, _f2, div))
		return false;

	if (!mod)
		return true;

	mask = 0xff << (8 - mod);
	c1 = ((__u8 *)_f1)[div];
	c2 = ((__u8 *)_f2)[div];

	return !((c1 ^ c2) & mask);
}

#define XDP2_SWITCH_START(TARG)					\
do {								\
	typeof(TARG) __xdp2_switch_target __unused() = (TARG);	\
	bool __xdp2_switch_check = false;			\
	{

#define __XDP2_SWITCH_CASE_EQUAL_HELPER()			\
		if (!__xdp2_switch_check)			\
			__xdp2_switch_check =			\
				xdp2_compare_equal(		\
				    __xdp2_switch_target,	\
				    &__xdp2_switch_v,		\
				    sizeof(__xdp2_switch_v));	\
	}							\
	if (__xdp2_switch_check) {

#define XDP2_SWITCH_CASE_EQUAL(V)				\
	} {							\
		typeof(*__xdp2_switch_target)			\
			__xdp2_switch_v = V;			\
								\
		__XDP2_SWITCH_CASE_EQUAL_HELPER()

#define XDP2_SWITCH_CASE_EQUAL_CONST(V)				\
	} {							\
								\
		typeof(*__xdp2_switch_target)			\
			__xdp2_switch_v = { XDP2_DEPAIR(V) };	\
								\
		__XDP2_SWITCH_CASE_EQUAL_HELPER()

#define __XDP2_SWITCH_CASE_TERN_HELPER()			\
		if (!__xdp2_switch_check)			\
			__xdp2_switch_check =			\
				xdp2_compare_tern(		\
				    __xdp2_switch_target,	\
				    &__xdp2_switch_v,		\
				    &__xdp2_switch_mask,	\
				    sizeof(__xdp2_switch_v));	\
	}							\
	if (__xdp2_switch_check) {

#define XDP2_SWITCH_CASE_TERN(V, MASK)				\
	} {							\
		typeof(*__xdp2_switch_target)			\
			__xdp2_switch_v = V;			\
		typeof(*__xdp2_switch_target)			\
			__xdp2_switch_mask = MASK;		\
								\
		__XDP2_SWITCH_CASE_TERN_HELPER()

#define XDP2_SWITCH_CASE_TERN_CONST(V, MASK)			\
	} {							\
		typeof(*__xdp2_switch_target)			\
			__xdp2_switch_v = { XDP2_DEPAIR(V) };	\
		typeof(*__xdp2_switch_target)			\
			__xdp2_switch_mask =			\
					{ XDP2_DEPAIR(MASK) };	\
								\
		__XDP2_SWITCH_CASE_TERN_HELPER()

#define __XDP2_SWITCH_CASE_PREFIX_HELPER()			\
		if (__xdp2_switch_plen >			\
				sizeof(__xdp2_switch_v))	\
			__xdp2_switch_plen =			\
				sizeof(__xdp2_switch_v);	\
		if (!__xdp2_switch_check)			\
			__xdp2_switch_check =			\
				xdp2_compare_prefix(		\
				    __xdp2_switch_target,	\
				    &__xdp2_switch_v,		\
				    __xdp2_switch_plen);		\
	}							\
	if (__xdp2_switch_check) {

#define XDP2_SWITCH_CASE_PREFIX(V, PLEN)			\
	} {							\
		typeof(*__xdp2_switch_target)			\
			__xdp2_switch_v = V;			\
		unsigned int __xdp2_switch_plen =		\
				(unsigned int)(PLEN);		\
								\
		__XDP2_SWITCH_CASE_PREFIX_HELPER()

#define XDP2_SWITCH_CASE_PREFIX_CONST(V, PLEN)			\
	} {							\
		typeof(*__xdp2_switch_target)			\
			__xdp2_switch_v = { XDP2_DEPAIR(V) };	\
		unsigned int __xdp2_switch_plen =		\
				(unsigned int)(PLEN);		\
								\
		__XDP2_SWITCH_CASE_PREFIX_HELPER()

#define XDP2_SWITCH_DEFAULT() }{

#define XDP2_SWITCH_END() } } while (0)

#define __XDP2_SELECT_START_ONE(NARG, VAR)			\
	typeof(VAR) XDP2_JOIN2(__xdp2_select_target, NARG)	\
					__unused() = (VAR);

#define XDP2_SELECT_START(...)					\
	do {							\
		bool __xdp2_select_fallthrough = false;		\
		XDP2_PMACRO_APPLY_ALL_NARG(			\
			__XDP2_SELECT_START_ONE, __VA_ARGS__)	\
	{

#define __XDP2_SELECT_COMPARE_ONE(NARG, V)			\
	&& (!XDP2_PMACRO_NARGS(V) ||				\
	    (XDP2_JOIN2(__xdp2_select_target, NARG) == (V + 0)))

#define XDP2_SELECT_CASE(...)					\
	}							\
	if (__xdp2_select_fallthrough || (true			\
		XDP2_PMACRO_APPLY_ALL_NARG(			\
			__XDP2_SELECT_COMPARE_ONE,		\
			__VA_ARGS__))) {			\
		__xdp2_select_fallthrough = true;

#define __XDP2_SELECT_NARGS(X) XDP2_PMACRO_NARGS(X)

#define __XDP2_SELECT_COMPARE_RANGE_ONE(NARG, PAIR)		\
	&& ((!__XDP2_SELECT_NARGS(XDP2_GET_POS_ARG2_1 PAIR) ||	\
	     (XDP2_JOIN2(__xdp2_select_target, NARG) >=		\
		(XDP2_GET_POS_ARG2_1 PAIR + 0))) &&		\
	    (!__XDP2_SELECT_NARGS(XDP2_GET_POS_ARG2_2 PAIR) ||	\
	     (XDP2_JOIN2(__xdp2_select_target, NARG) >=		\
		(XDP2_GET_POS_ARG2_2 PAIR + 0))))

#define XDP2_SELECT_CASE_RANGE(...)				\
	}							\
	if (__xdp2_select_fallthrough || (true			\
		XDP2_PMACRO_APPLY_ALL_NARG(			\
			__XDP2_SELECT_COMPARE_RANGE_ONE,	\
					__VA_ARGS__))) {	\
		__xdp2_select_fallthrough = true;

#define __XDP2_SELECT_COMPARE_MASK_ONE(NARG, PAIR)		\
	&& (!__XDP2_SELECT_NARGS(XDP2_GET_POS_ARG2_1 PAIR) ||	\
	    !__XDP2_SELECT_NARGS(XDP2_GET_POS_ARG2_2 PAIR) ||	\
	    !((XDP2_JOIN2(__xdp2_select_target, NARG) ^		\
	      (XDP2_GET_POS_ARG2_1 PAIR + 0)) &			\
	      (XDP2_GET_POS_ARG2_2 PAIR + 0)))

#define XDP2_SELECT_CASE_MASK(...)				\
	}							\
	if (__xdp2_select_fallthrough || (true			\
		XDP2_PMACRO_APPLY_ALL_NARG(			\
			__XDP2_SELECT_COMPARE_MASK_ONE,		\
					__VA_ARGS__))) {	\
		__xdp2_select_fallthrough = true;


#define __XDP2_SELECT_COMPARE_MASK_ANY_ONE(NARG, MASK)		\
	&& (!XDP2_PMACRO_NARGS(MASK) ||				\
	    (XDP2_JOIN2(__xdp2_select_target, NARG) & (MASK + 0)))

#define XDP2_SELECT_CASE_MASK_ANY(...)				\
	}							\
	if (__xdp2_select_fallthrough || (true			\
		XDP2_PMACRO_APPLY_ALL_NARG(			\
			__XDP2_SELECT_COMPARE_MASK_ANY_ONE,	\
					__VA_ARGS__))) {	\
		__xdp2_select_fallthrough = true;

#define XDP2_SELECT_DEFAULT() }{

#define XDP2_SELECT_END() } } while (0)

#endif /* __XDP2_SWITCH_H__ */
