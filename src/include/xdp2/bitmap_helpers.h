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

#ifndef __XDP2_BITMAP_HELPERS_H__
#define __XDP2_BITMAP_HELPERS_H__

/* Various helper functions for bitmaps. This file is meant to be included
 * from bitmap.h
 */
#include <linux/types.h>
#include <stdbool.h>

#ifndef __KERNEL__
#include <string.h>
#else
#include <linux/string.h>
#endif

#include "xdp2/bitmap_word.h"

#define __XDP2_BITMAP_MOD(POS, NUM_WORD_BITS) ((POS) % NUM_WORD_BITS)

#define __XDP2_BITMAP_DIV(POS, NUM_WORD_BITS) ((POS) / NUM_WORD_BITS)

/* Handle specific test for a word. Note this test does not require
 * endian convsersion for V and MASK which must be the same endianness
 */
#define __XDP2_BITMAP_HANDLE_TEST(TEST, RET, V, MASK) do {		\
	switch (TEST) {							\
	case XDP2_BITMAP_TEST_WEIGHT:					\
		(RET) += xdp2_bitmap_word64_weight(V);			\
		break;							\
	case XDP2_BITMAP_TEST_ANY_SET:					\
		if (V)							\
			(RET) = 1;					\
		break;							\
	case XDP2_BITMAP_TEST_ANY_ZERO:					\
		if (~(V) & (MASK))					\
			(RET) = 1;					\
		break;							\
	default:							\
		break;							\
	}								\
} while (0)

/* Handle specific test for a bit, V does not require any specific
 * endianness
 */
#define __XDP2_BITMAP_HANDLE_TEST_BIT(TEST, RET, V) do {		\
	switch (TEST) {							\
	case XDP2_BITMAP_TEST_WEIGHT:					\
		(RET) += (V);						\
		break;							\
	case XDP2_BITMAP_TEST_ANY_SET:					\
		if (V)							\
			(RET) = 1;					\
		break;							\
	case XDP2_BITMAP_TEST_ANY_ZERO:					\
		if (!(V))						\
			(RET) = 1;					\
		break;							\
	default:							\
		break;							\
	}								\
} while (0)

/* Handle a compare kind of test. Input V must be host endian */
#define __XDP2_BITMAP_DO_COMPARE(TEST, RET, V,ADDER, NUM_WORD_BITS) ({	\
	unsigned int _bit_num;						\
	bool _found = false;						\
									\
	if (V) {							\
		_bit_num = XDP2_JOIN3(xdp2_bitmap_word, NUM_WORD_BITS,	\
				      _find)(V);			\
		(RET) = _bit_num + ADDER;				\
		_found = true;						\
	}								\
									\
	_found;								\
})

/* SHift a value that might not be in host byte order. First convert to
 * host byte order, do the shift, and then convert byte order back
 */
#define __XDP2_BITMAP_SHIFT_CONVERT(V, SHIFT, CONV_FUNC)		\
	(CONV_FUNC(CONV_FUNC(V) SHIFT))

/* Helper macro for dealing with the case that the destination position
 * is not aligned to a word
 */
#define __XDP2_BITMAP_HANDLE_DEST_MOD(NEED, DEST_MOD, SRC_MOD, SRC,	\
				      VAL, NUM_WORD_BITS, SRC_DIR,	\
				      CONV_FUNC) do {			\
	unsigned int _diff, _need_right, _need_left;			\
									\
	if (DEST_MOD == SRC_MOD) {					\
		VAL = *SRC;						\
		SRC_MOD = 0;						\
		SRC += SRC_DIR;						\
	} else  if (DEST_MOD > SRC_MOD) {				\
		_diff = DEST_MOD - SRC_MOD;				\
		VAL = __XDP2_BITMAP_SHIFT_CONVERT(*SRC, << _diff,	\
						  CONV_FUNC);		\
		SRC_MOD += _need;					\
	} else  {							\
		_diff = SRC_MOD - DEST_MOD;				\
		_need_left = xdp2_min(NUM_WORD_BITS - SRC_MOD, _need);	\
		_need_right = _need - _need_left;			\
		VAL = __XDP2_BITMAP_SHIFT_CONVERT(*SRC, >> _diff,	\
						  CONV_FUNC) |		\
		      __XDP2_BITMAP_SHIFT_CONVERT(SRC[SRC_DIR],		\
				<< (DEST_MOD + _need_left), CONV_FUNC);	\
		SRC += SRC_DIR;						\
		SRC_MOD = _need_right;					\
	}								\
} while (0)

/* Create all 1's value for the requested type */
#define __XDP2_BITMAP_1S(TYPE) ((TYPE)-1ULL)

/* Definitions to create word high mask, low mask, or both */
#define __XDP2_BITMAP_HIGH_MASK(START, TYPE)				\
	(__XDP2_BITMAP_1S(TYPE) << (START))

#define __XDP2_BITMAP_LOW_MASK(START, NUM_WORD_BITS, TYPE)		\
	(__XDP2_BITMAP_1S(TYPE) >> ((NUM_WORD_BITS) - (START)))

#define __XDP2_BITMAP_MASK(START, NBITS, NUM_WORD_BITS, TYPE)		\
	(__XDP2_BITMAP_HIGH_MASK(START, TYPE) &				\
	 __XDP2_BITMAP_LOW_MASK((START) + (NBITS), NUM_WORD_BITS, TYPE))

/* Helper to compute a value for a one argument operation */
#define __XDP2_BITMAP_VALUE1(OP, ARG) (OP ARG)

/* Helper to compute a value for a two argument operation */
#define __XDP2_BITMAP_VALUE2(OP, NOT, S1_OP, S2_OP, ARG1, ARG2)		\
				(NOT((S1_OP ARG1) OP (S2_OP ARG2)))

/* Helper to compute a value for a two argument operation based on
 * whether the destination is also a source
 */
#define __XDP2_BITMAP_VALUE(DESTSRC, OP, NOT, S1_OP, S2_OP, ARG1,	\
			    ARG2, TYPE) ({				\
		TYPE _r;						\
									\
		if (DESTSRC)						\
			_r = __XDP2_BITMAP_VALUE2(OP, NOT, S1_OP,	\
						  S2_OP, ARG1, ARG2);	\
		else							\
			_r = __XDP2_BITMAP_VALUE1(S1_OP, ARG1);		\
		_r;							\
})

/* Base macro for processing bitmap operations with one argument.
 *
 * If DO_COMPARE is set a compare or find operation is performed based on
 * the test in TEST. If DO_SET or DO_UNSET are set the bitmap is zeroed or
 * set to all ones respectively. If none of those are set a TEST is
 * performed as specified in TEST (e.g. get Hamming weight)
 */
#define __XDP2_BITMAP_1ARG(OP, SRC, POS, NBITS, DO_SET, DO_UNSET,	\
			   DO_COMPARE, TEST, RET, CONST, _DEST,		\
			   NUM_WORD_BITS, TYPE, CONV_FUNC, NUM_WORDS)	\
							do {		\
	unsigned int _mod = __XDP2_BITMAP_MOD(POS, NUM_WORD_BITS);	\
	unsigned int _div = __XDP2_BITMAP_DIV(POS, NUM_WORD_BITS);	\
	unsigned int _nbits = (NBITS) - (POS), _need = 0;		\
	int _dir2 = (NUM_WORDS) ? 1 : 0;				\
	int _dir = (NUM_WORDS) ? -1 : 1;				\
	TYPE *_dummy_dest = NULL;					\
	TYPE _res, _mask;						\
	CONST TYPE *_src;						\
	int _i;								\
									\
	/* Avoid compiler warning on unused variable */                 \
	if (_dummy_dest)                                                \
		break;							\
									\
	_src = (NUM_WORDS) ? ((SRC) + ((NUM_WORDS) - 1)) - _div :	\
							(SRC) + _div;	\
									\
	/* Handle non-zero modulo dest position */			\
	if (_mod) {							\
		if ((NBITS) <= NUM_WORD_BITS)				\
			_need = _nbits;					\
		else							\
			_need = xdp2_min(NUM_WORD_BITS - _mod, _nbits);	\
		_mask = CONV_FUNC(__XDP2_BITMAP_MASK(_mod, _need,	\
						     NUM_WORD_BITS,	\
						     TYPE));		\
		if (DO_SET) {						\
			*_DEST |= _mask;				\
		} else if (DO_UNSET) {					\
			*_DEST &= ~_mask;				\
		} else {						\
			_res = (OP *_src) & _mask;			\
			if (DO_COMPARE) {				\
				if (__XDP2_BITMAP_DO_COMPARE(TEST,	\
						RET, CONV_FUNC(_res),	\
						(POS) - _mod,		\
						NUM_WORD_BITS))		\
					break;				\
			} else {					\
				__XDP2_BITMAP_HANDLE_TEST(TEST, RET,	\
							  _res, _mask);	\
			}						\
		}							\
		_nbits -= _need;					\
		if (!_nbits)						\
			break;						\
		_src += _dir;						\
	}								\
									\
	/* Handle whole words */					\
	if (DO_SET) {							\
		_i = __XDP2_BITMAP_DIV(_nbits, NUM_WORD_BITS);		\
		memset(_DEST - _dir2 * (_i - 1), 0xff,			\
		       XDP2_BITS_TO_BYTES(_i * NUM_WORD_BITS));		\
	} else if (DO_UNSET) {						\
		_i = __XDP2_BITMAP_DIV(_nbits, NUM_WORD_BITS);		\
		memset(_DEST - _dir2 * (_i - 1), 0,			\
		       XDP2_BITS_TO_BYTES(_i * NUM_WORD_BITS));		\
	} else {							\
		for (_i = 0; _i < __XDP2_BITMAP_DIV(_nbits,		\
						NUM_WORD_BITS); _i++) {	\
			_res = (OP _src[_dir * _i]);			\
			if (DO_COMPARE) {				\
				if (__XDP2_BITMAP_DO_COMPARE(TEST, RET,	\
				    CONV_FUNC(_res), (POS) + _need +	\
					_i * NUM_WORD_BITS,		\
				    NUM_WORD_BITS)) {			\
					_nbits = 0;			\
					break;				\
				}					\
			} else {					\
				__XDP2_BITMAP_HANDLE_TEST(TEST, RET,	\
				  _res, __XDP2_BITMAP_1S(TYPE));	\
			}						\
		}							\
	}								\
									\
	/* Handle leftover */						\
	_nbits = __XDP2_BITMAP_MOD(_nbits, NUM_WORD_BITS);		\
	if (_nbits) {							\
		_mask = CONV_FUNC(__XDP2_BITMAP_LOW_MASK(_nbits,	\
						NUM_WORD_BITS, TYPE));	\
		if (DO_SET) {						\
			_DEST[_dir * _i] |= _mask;			\
		} else if (DO_UNSET) {					\
			_DEST[_dir * _i] &= ~_mask;			\
		} else {						\
			_res = (OP _src[_dir * _i]) & _mask;		\
			if (DO_COMPARE) {				\
				if (__XDP2_BITMAP_DO_COMPARE(TEST,	\
				    RET, CONV_FUNC(_res),		\
				    (NBITS) - _nbits, NUM_WORD_BITS))	\
					break;				\
			} else {					\
				__XDP2_BITMAP_HANDLE_TEST(TEST, RET,	\
							  _res, _mask);	\
			}						\
		}							\
	}								\
} while (0)

/* Base macros for processing bitmap operations with two arguments
 * arguments: either one destination and one source, or two sources.
 * There are two modes: a normal operation that sets a result in DEST and
 * a DO_COMPARE mode that computes numerical result.
 *
 * For normal mode, if DESTSRC is set then the first argument is both a
 * source and destination, and TEST indicates that test result is
 * computed and set in RET. TEST may indicate no test result is returned
 *
 * For DO_COMPARE, if TEST indicates a compare test. RET is set to the
 * result of the test and no bitmaps are modified
 */

#define __XDP2_BITMAP_2ARG(DESTSRC, OP, NOT, S1_OP, S2_OP, DEST, SRC,	\
			   POS, NBITS, DO_COMPARE, TEST, RET,		\
			   CONST, _DEST, NUM_WORD_BITS, TYPE,		\
			   CONV_FUNC, WORD_SRC, NUM_WORDS) do {		\
	unsigned int _mod = __XDP2_BITMAP_MOD(POS, NUM_WORD_BITS);	\
	unsigned int _div = __XDP2_BITMAP_DIV(POS, NUM_WORD_BITS);	\
	int _dir_src = (WORD_SRC) ? 0 : ((NUM_WORDS) ? -1 : 1);		\
	unsigned int _nbits = (NBITS) - (POS), _need = 0;		\
	int _dir = (NUM_WORDS) ? -1 : 1, _i;				\
	TYPE *_dummy_dest = NULL;					\
	CONST TYPE *_dest;						\
	const TYPE *_src;						\
	TYPE _res, _mask;						\
									\
	/* Avoid compiler warning on unused variable */			\
	if (_dummy_dest)						\
		break;							\
									\
	if (NUM_WORDS) {						\
		_src = (WORD_SRC) ? (SRC) :				\
				((SRC) + ((NUM_WORDS) - 1)) - _div;	\
		_dest = ((DEST) + ((NUM_WORDS) - 1)) - _div;		\
	} else {							\
		_src = (WORD_SRC) ? (SRC) : (SRC) + _div;		\
		_dest = (DEST) + _div;					\
	}								\
									\
	/* Handle non-zero modulo dest position */			\
	if (_mod) {							\
		_need = xdp2_min(NUM_WORD_BITS - _mod, _nbits);		\
		_mask = CONV_FUNC(__XDP2_BITMAP_MASK(_mod, _need,	\
						NUM_WORD_BITS, TYPE));	\
		_res = __XDP2_BITMAP_VALUE(DESTSRC, OP, NOT, S1_OP,	\
					   S2_OP, *_src, *_dest,	\
					   TYPE);			\
		_res &= _mask;						\
		if (DO_COMPARE) {					\
			if (__XDP2_BITMAP_DO_COMPARE(TEST, RET,		\
					CONV_FUNC(_res),		\
					(POS) - _mod, NUM_WORD_BITS))	\
				break;					\
		} else {						\
			*_DEST = _res | (*_dest & ~_mask);		\
			__XDP2_BITMAP_HANDLE_TEST(TEST, RET, _res,	\
						  _mask);		\
		}							\
		_nbits -= _need;					\
		if (!_nbits)						\
			break;						\
		_dest += _dir; _src += _dir_src;			\
	}								\
									\
	/* Handle whole words */					\
	for (_i = 0; _i < __XDP2_BITMAP_DIV(_nbits,			\
					NUM_WORD_BITS); _i++) {		\
		_res = __XDP2_BITMAP_VALUE(DESTSRC, OP, NOT,		\
					   S1_OP, S2_OP,		\
					   _src[_dir_src * _i],		\
					   _dest[_dir * _i],		\
					   TYPE);			\
		if (DO_COMPARE) {					\
			if (__XDP2_BITMAP_DO_COMPARE(TEST, RET,		\
			    CONV_FUNC(_res), (POS) + _need + _i *	\
			    NUM_WORD_BITS, NUM_WORD_BITS)) {		\
				_nbits = 0;				\
				break;					\
			}						\
		} else {						\
			_DEST[_dir * _i] = _res;			\
			__XDP2_BITMAP_HANDLE_TEST(TEST, RET, _res,	\
					__XDP2_BITMAP_1S(TYPE));	\
		}							\
	}								\
									\
	/* Handle leftover */						\
	_nbits = __XDP2_BITMAP_MOD(_nbits, NUM_WORD_BITS);		\
	if (_nbits) {							\
		_mask = CONV_FUNC(__XDP2_BITMAP_LOW_MASK(_nbits,	\
						NUM_WORD_BITS, TYPE));	\
		_res = __XDP2_BITMAP_VALUE(DESTSRC, OP, NOT,		\
					   S1_OP, S2_OP,		\
					   _src[_dir_src * _i],		\
					   _dest[_dir * _i], TYPE);	\
		_res &= _mask;						\
		if (DO_COMPARE) {					\
			if (__XDP2_BITMAP_DO_COMPARE(TEST, RET,		\
						CONV_FUNC(_res),	\
						(NBITS) - _nbits,	\
						NUM_WORD_BITS))		\
				break;					\
		} else {						\
			_DEST[_dir * _i] =				\
				_res | (_dest[_dir * _i] & ~_mask);	\
			__XDP2_BITMAP_HANDLE_TEST(TEST, RET, _res,	\
						  _mask);		\
		}							\
	}								\
} while (0)

/* Generic variant of three argument macro where each argument has its
 * own position specified
 */
#define __XDP2_BITMAP_2ARG_GEN(DESTSRC, OP, NOT, S1_OP, S2_OP,		\
				DEST, DEST_POS, SRC, SRC_POS, NBITS,	\
				DO_COMPARE, TEST, RET, CONST,		\
				_DEST, NUM_WORD_BITS, TYPE, CONV_FUNC,	\
				WORD_SRC, NUM_WORDS) do {		\
	unsigned int _dest_mod =					\
			__XDP2_BITMAP_MOD(DEST_POS, NUM_WORD_BITS);	\
	unsigned int _src_mod =						\
			__XDP2_BITMAP_MOD(SRC_POS, NUM_WORD_BITS);	\
	unsigned int _dest_div =					\
			__XDP2_BITMAP_DIV(DEST_POS, NUM_WORD_BITS);	\
	unsigned int _src_div =						\
			__XDP2_BITMAP_DIV(SRC_POS, NUM_WORD_BITS);	\
	int _dir_src = (WORD_SRC) ? 0 : ((NUM_WORDS) ? -1 : 1);		\
	unsigned int _need = 0, _nbits = (NBITS) - (DEST_POS);		\
	unsigned int _left_shift = 0, _right_shift;			\
	TYPE _mask_left_src = 0, _mask_right_src;			\
	int _dir = (NUM_WORDS) ? -1 : 1, _i;				\
	TYPE _v_src_left = 0, _v_src_right;				\
	TYPE *_dummy_dest = NULL;					\
	TYPE _val, _res, _mask;						\
	CONST TYPE *_dest;						\
	const TYPE *_src;						\
									\
	/* Avoid compiler warning on unused variable */			\
	if (_dummy_dest)						\
		break;							\
									\
	if (NUM_WORDS) {						\
		_src = (WORD_SRC) ? (SRC) :				\
				((SRC) + ((NUM_WORDS) - 1)) - _src_div;	\
		_dest = ((DEST) + ((NUM_WORDS) - 1)) - _dest_div;	\
	} else {							\
		_src = (WORD_SRC) ? (SRC) : (SRC) + _src_div;		\
		_dest = (DEST) + _dest_div;				\
	}								\
									\
	/* Handle non-zero modulo dest position */			\
	if (_dest_mod) {						\
		_need = xdp2_min(NUM_WORD_BITS - _dest_mod, _nbits);	\
		__XDP2_BITMAP_HANDLE_DEST_MOD(_need, _dest_mod,		\
					      _src_mod, _src,		\
					      _val, NUM_WORD_BITS,	\
					      _dir_src, CONV_FUNC);	\
		_mask = CONV_FUNC(__XDP2_BITMAP_MASK(_dest_mod,		\
						_need, NUM_WORD_BITS,	\
						TYPE));			\
		_res = __XDP2_BITMAP_VALUE(DESTSRC, OP, NOT, S1_OP,	\
					   S2_OP, _val, *_dest,		\
					   TYPE);			\
		_res &= _mask;						\
		if (DO_COMPARE) {					\
			if (__XDP2_BITMAP_DO_COMPARE(			\
					TEST, RET, CONV_FUNC(_res),	\
					(DEST_POS) - _dest_mod,		\
					NUM_WORD_BITS))			\
				break;					\
		} else {						\
			*_DEST = _res | (*_dest & ~_mask);		\
			__XDP2_BITMAP_HANDLE_TEST(TEST, RET, _res,	\
						  _mask);		\
		}							\
		_nbits -= _need;					\
		if (!_nbits)						\
			break;						\
		/* All bits in first word used, advance */		\
		_dest += _dir;						\
	}								\
									\
	if (_src_mod) {							\
		_mask_left_src = CONV_FUNC(				\
				__XDP2_BITMAP_HIGH_MASK(_src_mod,	\
				TYPE));					\
		_left_shift = _src_mod;					\
		_v_src_left = __XDP2_BITMAP_SHIFT_CONVERT(		\
			*_src & _mask_left_src, >> _left_shift,		\
			CONV_FUNC);					\
		if (_nbits + _src_mod > NUM_WORD_BITS) {		\
			/* Only advance if rest of bits are used */	\
			_src += _dir_src;				\
		}							\
	} else {							\
		_v_src_left = *_src;					\
	}								\
	_mask_right_src = ~_mask_left_src;				\
	_right_shift =  NUM_WORD_BITS - _left_shift;			\
									\
	/* Handle whole words */					\
	for (_i = 0; _i < __XDP2_BITMAP_DIV(_nbits,			\
					NUM_WORD_BITS); _i++) {		\
		_v_src_right = _right_shift < NUM_WORD_BITS ?		\
			__XDP2_BITMAP_SHIFT_CONVERT(			\
				_src[_dir_src * _i] &			\
				_mask_right_src, << _right_shift,	\
				CONV_FUNC) : _src[_dir_src * _i];	\
		_val = _v_src_left | _v_src_right;			\
		_res = __XDP2_BITMAP_VALUE(DESTSRC, OP, NOT, S1_OP,	\
					   S2_OP, _val,			\
					   _dest[_dir * _i],		\
					   TYPE);			\
		if (DO_COMPARE) {					\
			if (__XDP2_BITMAP_DO_COMPARE(TEST, RET,		\
			    CONV_FUNC(_res),				\
			    (DEST_POS) + _need + _i * NUM_WORD_BITS,	\
						NUM_WORD_BITS)) {	\
				_nbits = 0;				\
				break;					\
			}						\
		} else {						\
			_DEST[_dir * _i] = _res;			\
			__XDP2_BITMAP_HANDLE_TEST(TEST, RET, _res,	\
				__XDP2_BITMAP_1S(TYPE));		\
		}							\
		_v_src_left = __XDP2_BITMAP_SHIFT_CONVERT(		\
			_src[_dir_src * _i] & _mask_left_src,		\
			 >> _left_shift, CONV_FUNC);			\
	}								\
									\
	/* Handle leftover */						\
	_nbits = __XDP2_BITMAP_MOD(_nbits, NUM_WORD_BITS);		\
	if (_nbits) {							\
		_mask = CONV_FUNC(__XDP2_BITMAP_LOW_MASK(_nbits,	\
						NUM_WORD_BITS, TYPE));	\
		_v_src_right = _right_shift < NUM_WORD_BITS ?		\
			__XDP2_BITMAP_SHIFT_CONVERT(			\
			_src[_dir_src * _i] & _mask_right_src,		\
			 << _right_shift, CONV_FUNC) :			\
						_src[_dir_src * _i];	\
		_val = _v_src_left | _v_src_right;			\
		_res = __XDP2_BITMAP_VALUE(DESTSRC, OP, NOT, S1_OP,	\
					   S2_OP, _val,			\
					   _dest[_dir * _i],		\
					   TYPE);			\
		_res &= _mask;						\
		if (DO_COMPARE) {					\
			if (__XDP2_BITMAP_DO_COMPARE(TEST, RET,		\
				    CONV_FUNC(_res), (NBITS) - _nbits,	\
				    NUM_WORD_BITS))			\
				break;					\
		} else {						\
			_DEST[_dir * _i] =				\
				_res | (_dest[_dir * _i] & ~_mask);	\
			__XDP2_BITMAP_HANDLE_TEST(TEST, RET, _res,	\
						  _mask);		\
		}							\
	}								\
} while (0)

/* Base macros for processing bitmap operations with three arguments:
 * one destination and two sources. TEST optionally indicates a test
 * and any result is set in RET
 */

/* Variant of three argument macro where each argument shares a position */
#define __XDP2_BITMAP_3ARG(OP, NOT, S1_OP, S2_OP, DEST, SRC1, SRC2,	\
			   POS, NBITS, TEST, RET, NUM_WORD_BITS,	\
			   TYPE, CONV_FUNC, WORD_SRC1, NUM_WORDS) do {	\
	unsigned int _mod = __XDP2_BITMAP_MOD(POS, NUM_WORD_BITS);	\
	unsigned int _div = __XDP2_BITMAP_DIV(POS, NUM_WORD_BITS);	\
	int _dir_src1 = (WORD_SRC1) ? 0 : ((NUM_WORDS) ? -1 : 1);	\
	unsigned int _nbits = (NBITS) - (POS), _need;			\
	int _dir = (NUM_WORDS) ? -1 : 1, _i;				\
	const TYPE *_src1, *_src2;					\
	TYPE _res, _mask, *_dest;					\
									\
	if (NUM_WORDS) {						\
		_src1 = (WORD_SRC1) ? (SRC1) :				\
			((SRC1) + ((NUM_WORDS) - 1)) - _div;		\
		_src2 = ((SRC2) + ((NUM_WORDS) - 1)) - _div;		\
		_dest = ((DEST) + ((NUM_WORDS) - 1)) - _div;		\
	} else {							\
		_src1 = (WORD_SRC1) ? (SRC1) : (SRC1) + _div;		\
		_src2 = (SRC2) + _div;					\
		_dest = (DEST) + _div;					\
	}								\
									\
	/* Handle non-zero modulo dest position */			\
	if (_mod) {							\
		_need = xdp2_min(NUM_WORD_BITS - _mod, _nbits);		\
		_mask = CONV_FUNC(__XDP2_BITMAP_MASK(_mod, _need,	\
						     NUM_WORD_BITS,	\
						     TYPE));		\
		_res = __XDP2_BITMAP_VALUE2(OP, NOT, S1_OP, S2_OP,	\
					     *_src1, *_src2);		\
		_res &= _mask;						\
		__XDP2_BITMAP_HANDLE_TEST(TEST, RET, _res, _mask);	\
		*_dest = _res | (*_dest & ~_mask);			\
		_nbits -= _need;					\
		if (!_nbits)						\
			break;						\
		/* All bits in first word used, advance */		\
		_dest += _dir, _src1 += _dir_src1, _src2 += _dir;	\
	}								\
									\
	/* Handle whole words */					\
	for (_i = 0; _i < __XDP2_BITMAP_DIV(_nbits,			\
					    NUM_WORD_BITS); _i++) {	\
		_res = __XDP2_BITMAP_VALUE2(OP, NOT, S1_OP, S2_OP,	\
					     _src1[_dir_src1 * _i],	\
					     _src2[_dir * _i]);		\
		_dest[_dir * _i] = _res;				\
		__XDP2_BITMAP_HANDLE_TEST(TEST, RET, _res,		\
				__XDP2_BITMAP_1S(TYPE));		\
	}								\
									\
	/* Handle leftover */						\
	_nbits = __XDP2_BITMAP_MOD(_nbits, NUM_WORD_BITS);		\
	if (_nbits) {							\
		_mask = CONV_FUNC(__XDP2_BITMAP_LOW_MASK(_nbits,	\
				  NUM_WORD_BITS, TYPE));		\
		_res = __XDP2_BITMAP_VALUE2(OP, NOT, S1_OP, S2_OP,	\
					     _src1[_dir_src1 * _i],	\
					     _src2[_dir * _i]);		\
		_res &= _mask;						\
		__XDP2_BITMAP_HANDLE_TEST(TEST, RET, _res, _mask);	\
		_dest[_dir * _i] = (_dest[_dir * _i] & ~_mask) | _res;	\
	}								\
} while (0)

/* Generic variant of three argument macro where each argument has its own
 * position specified
 */
#define __XDP2_BITMAP_3ARG_GEN(OP, NOT, S1_OP, S2_OP, DEST, DEST_POS,	\
			       SRC1, SRC1_POS, SRC2, SRC2_POS,		\
			       NBITS, TEST, RET, NUM_WORD_BITS,		\
			       TYPE, CONV_FUNC, WORD_SRC1,		\
			       NUM_WORDS) do {				\
	const TYPE *_src1 =						\
		(SRC1) + __XDP2_BITMAP_DIV(SRC1_POS, NUM_WORD_BITS);	\
	const TYPE *_src2 =						\
		(SRC2) + __XDP2_BITMAP_DIV(SRC2_POS, NUM_WORD_BITS);	\
	TYPE *_dest =							\
		(DEST) + __XDP2_BITMAP_DIV(DEST_POS, NUM_WORD_BITS);	\
	unsigned int _src1_mod =					\
		__XDP2_BITMAP_MOD(SRC1_POS, NUM_WORD_BITS);		\
	unsigned int _src2_mod =					\
		__XDP2_BITMAP_MOD(SRC2_POS, NUM_WORD_BITS);		\
	unsigned int _dest_mod =					\
		__XDP2_BITMAP_MOD(DEST_POS, NUM_WORD_BITS);		\
	unsigned int _src1_div =					\
		__XDP2_BITMAP_DIV(SRC1_POS, NUM_WORD_BITS);		\
	unsigned int _src2_div =					\
		__XDP2_BITMAP_DIV(SRC2_POS, NUM_WORD_BITS);		\
	unsigned int _dest_div =					\
		__XDP2_BITMAP_DIV(DEST_POS, NUM_WORD_BITS);		\
	int _dir_src1 = (WORD_SRC1) ? 0 : ((NUM_WORDS) ? -1 : 1);	\
	unsigned int _need, _nbits = (NBITS) - (DEST_POS);		\
	TYPE _mask_left_src1 = 0, _mask_left_src2 = 0;			\
	unsigned int _left_shift1 = 0, _right_shift1;			\
	unsigned int _left_shift2 = 0, _right_shift2;			\
	TYPE _mask_right_src1, _mask_right_src2;			\
	TYPE _v_src1_left = 0, _v_src1_right;				\
	TYPE _v_src2_left = 0, _v_src2_right;				\
	int _dir = (NUM_WORDS) ? -1 : 1, _i;				\
	TYPE _val1, _val2, _res, _mask;					\
									\
	if (NUM_WORDS) {						\
		_src1 = (WORD_SRC1) ? (SRC1) :				\
			((SRC1) + ((NUM_WORDS) - 1)) - _src1_div;	\
		_src2 = ((SRC2) + ((NUM_WORDS) - 1)) - _src2_div;	\
		_dest = ((DEST) + ((NUM_WORDS) - 1)) - _dest_div;	\
	} else {							\
		_src1 = (WORD_SRC1) ? (SRC1) : (SRC1) + _src1_div;	\
		_src2 = (SRC2) + _src2_div;				\
		_dest = (DEST) + _dest_div;				\
	}								\
									\
	/* Handle non-zero modulo dest position */			\
	if (_dest_mod) {						\
		_need = xdp2_min(NUM_WORD_BITS - _dest_mod, _nbits);	\
		__XDP2_BITMAP_HANDLE_DEST_MOD(_need, _dest_mod,		\
					      _src1_mod, _src1,		\
					      _val1, NUM_WORD_BITS,	\
					      _dir_src1, CONV_FUNC);	\
		__XDP2_BITMAP_HANDLE_DEST_MOD(_need, _dest_mod,		\
					      _src2_mod, _src2,		\
					      _val2, NUM_WORD_BITS,	\
					      _dir, CONV_FUNC);		\
		_mask = CONV_FUNC(__XDP2_BITMAP_MASK(_dest_mod,		\
						     _need,		\
						     NUM_WORD_BITS,	\
						     TYPE));		\
		_res = __XDP2_BITMAP_VALUE2(OP, NOT, S1_OP,		\
					     S2_OP, _val1, _val2);	\
		_res &= _mask;						\
		__XDP2_BITMAP_HANDLE_TEST(TEST, RET, _res, _mask);	\
		*_dest = _res | (*_dest & ~_mask);			\
		_nbits -= _need;					\
		if (!_nbits)						\
			break;						\
		/* All bits in first word used, advance */		\
		_dest += _dir;						\
	}								\
									\
	if (_src1_mod) {						\
		_mask_left_src1 = CONV_FUNC(				\
				__XDP2_BITMAP_HIGH_MASK(_src1_mod,	\
				TYPE));					\
		_left_shift1 = _src1_mod;				\
		_v_src1_left =						\
			__XDP2_BITMAP_SHIFT_CONVERT(			\
			*_src1 & _mask_left_src1, >> _left_shift1,	\
			CONV_FUNC);					\
		if (_nbits + _src1_mod > NUM_WORD_BITS) {		\
			/* Only advance if rest of bits are used */	\
			_src1 += _dir_src1;				\
		}							\
	} else {							\
		_v_src1_left = *_src1;					\
	}								\
	_right_shift1 =  NUM_WORD_BITS - _left_shift1;			\
	_mask_right_src1 = ~_mask_left_src1;				\
									\
	if (_src2_mod) {						\
		_mask_left_src2 = CONV_FUNC(				\
				__XDP2_BITMAP_HIGH_MASK(_src2_mod,	\
				TYPE));					\
		_left_shift2 = _src2_mod;				\
		_v_src2_left =						\
			__XDP2_BITMAP_SHIFT_CONVERT(			\
			*_src2 & _mask_left_src2, >> _left_shift2,	\
			CONV_FUNC);					\
		if (_nbits + _src2_mod > NUM_WORD_BITS) {		\
			/* Only advance if rest of bits are used */	\
			_src2 += _dir;					\
		}							\
	} else {							\
		_v_src2_left = *_src2;					\
	}								\
	_right_shift2 =  NUM_WORD_BITS - _left_shift2;			\
	_mask_right_src2 = ~_mask_left_src2;				\
									\
	/* Handle whole words */					\
	for (_i = 0; _i < __XDP2_BITMAP_DIV(_nbits, NUM_WORD_BITS);	\
							     _i++) {	\
		_v_src1_right = _right_shift1 <	NUM_WORD_BITS ?		\
			__XDP2_BITMAP_SHIFT_CONVERT(			\
			_src1[_dir_src1 * _i] & _mask_right_src1,	\
				 << _right_shift1, CONV_FUNC) :		\
			_src1[_dir_src1 * _i];				\
		_val1 = _v_src1_left | _v_src1_right;			\
		_v_src2_right = _right_shift2 <	NUM_WORD_BITS ?		\
			__XDP2_BITMAP_SHIFT_CONVERT(			\
			_src2[_dir * _i] & _mask_right_src2,		\
					<< _right_shift2, CONV_FUNC) :	\
			_src2[_dir * _i];				\
		_val2 = _v_src2_left | _v_src2_right;			\
		_res = __XDP2_BITMAP_VALUE2(OP, NOT, S1_OP, S2_OP,	\
					     _val1, _val2);		\
		_dest[_dir * _i] = _res;				\
		__XDP2_BITMAP_HANDLE_TEST(TEST, RET, _res,		\
				__XDP2_BITMAP_1S(TYPE));		\
		_v_src1_left = __XDP2_BITMAP_SHIFT_CONVERT(		\
			_src1[_dir_src1 * _i] & _mask_left_src1,	\
			 >> _left_shift1, CONV_FUNC);			\
		_v_src2_left = __XDP2_BITMAP_SHIFT_CONVERT		\
			(_src2[_dir * _i] & _mask_left_src2,		\
			 >> _left_shift2, CONV_FUNC);			\
	}								\
									\
	/* Handle leftover */						\
	_nbits = __XDP2_BITMAP_MOD(_nbits, NUM_WORD_BITS);		\
	if (_nbits) {							\
		_mask = CONV_FUNC(__XDP2_BITMAP_LOW_MASK(_nbits,	\
						NUM_WORD_BITS, TYPE));	\
		_v_src1_right = _right_shift1 <	NUM_WORD_BITS ?		\
			__XDP2_BITMAP_SHIFT_CONVERT(			\
			_src1[_dir_src1 * _i] & _mask_right_src1,	\
				 << _right_shift1, CONV_FUNC) :		\
			_src1[_dir_src1 * _i];				\
		_val1 = _v_src1_left | _v_src1_right;			\
		_v_src2_right = _right_shift2 < NUM_WORD_BITS ?		\
			__XDP2_BITMAP_SHIFT_CONVERT(			\
			_src2[_dir * _i] & _mask_right_src2,		\
			 << _right_shift2, CONV_FUNC) :			\
			_src2[_dir * _i];				\
		_val2 = _v_src2_left | _v_src2_right;			\
		_res = __XDP2_BITMAP_VALUE2(OP, NOT, S1_OP,		\
					     S2_OP, _val1, _val2);	\
		_res &= _mask;						\
		__XDP2_BITMAP_HANDLE_TEST(TEST, RET, _res, _mask);	\
		_dest[_dir * _i] = _res | (_dest[_dir * _i] & ~_mask);	\
	}								\
} while (0)

/******** Helper macros for single argument functions */

/* Helper macro to create a test function. The result of the test is
 * returned (e.g. return Hamming weight, check for all zeroes)
 */
#define __XDP2_BITMAP_1ARG_TEST_FUNCT(NAME, OP, NUM_WORD_BITS,		\
				      TYPE, SUFFIX, CONV_FUNC)		\
static inline unsigned int XDP2_JOIN4(xdp2_bitmap, SUFFIX, _, NAME)(	\
				      TYPE *addr,			\
				      unsigned int pos,			\
				      unsigned int nbits,		\
				      unsigned int test)		\
{									\
	unsigned int w = 0;						\
									\
	__XDP2_BITMAP_1ARG(OP, addr, pos, nbits, 0, 0, 0, test, w,	\
			   const, _dummy_dest, NUM_WORD_BITS, TYPE,	\
			   CONV_FUNC, 0);				\
									\
	return w;							\
}									\
									\
static inline unsigned int XDP2_JOIN4(xdp2_rbitmap, SUFFIX, _, NAME)(	\
				      TYPE *addr,			\
				      unsigned int pos,			\
				      unsigned int nbits,		\
				      unsigned int test,		\
				      unsigned int num_words)		\
{									\
	unsigned int w = 0;						\
									\
	__XDP2_BITMAP_1ARG(OP, addr, pos, nbits, 0, 0, 0, test, w,	\
			   const, _dummy_dest, NUM_WORD_BITS, TYPE,	\
			   CONV_FUNC, num_words);			\
									\
	return w;							\
}

/* Helper macro to create a set function (set bitmap to all zeroes or
 * all ones)
 */
#define __XDP2_BITMAP_1ARG_SET_FUNCT(NAME, DO_SET, DO_UNSET,		\
				     NUM_WORD_BITS, TYPE, SUFFIX,	\
				     CONV_FUNC)				\
static inline void XDP2_JOIN4(xdp2_bitmap, SUFFIX, _, NAME)(		\
			      TYPE *addr, unsigned int pos,		\
			      unsigned int nbits)			\
{									\
	unsigned int w;							\
									\
	__XDP2_BITMAP_1ARG(, addr, pos, nbits, DO_SET, DO_UNSET, 1,	\
			   0, w,, _src, NUM_WORD_BITS, TYPE,		\
			   CONV_FUNC, 0);				\
}									\
									\
static inline void XDP2_JOIN4(xdp2_rbitmap, SUFFIX, _, NAME)(		\
				TYPE *addr, unsigned int pos,		\
				unsigned int nbits,			\
				unsigned int num_words)			\
{									\
	unsigned int w;							\
									\
	__XDP2_BITMAP_1ARG(, addr, pos, nbits, DO_SET, DO_UNSET, 1,	\
			   0, w,, _src, NUM_WORD_BITS, TYPE, CONV_FUNC,	\
			   num_words);					\
}


/* Helper macro to create a find function. The position of the
 * first bit found, either first one or first zero. If ROLL is set
 * and find reaches the end then a second find is done starting from
 * the first bit
 */
#define __XDP2_BITMAP_1ARG_FIND_FUNCT(NAME, OP, POS_ADD, NUM_WORD_BITS,	\
				      TYPE, SUFFIX, ROLL, CONV_FUNC)	\
static inline unsigned int XDP2_JOIN4(xdp2_bitmap, SUFFIX, _, NAME)(	\
				      const TYPE *addr,			\
				      unsigned int pos,			\
				      unsigned int nbits)		\
{									\
	unsigned int w = nbits;						\
									\
	__XDP2_BITMAP_1ARG(OP, addr, pos + (POS_ADD), nbits,		\
			   0, 0, 1, 0, w, const, _dummy_dest,		\
			   NUM_WORD_BITS, TYPE, CONV_FUNC, 0);		\
									\
	if (ROLL && w >= nbits)	{					\
		w = nbits;						\
		__XDP2_BITMAP_1ARG(OP, addr, 0, nbits,			\
				   0, 0, 1, 0, w, const, _dummy_dest,	\
				   NUM_WORD_BITS, TYPE, CONV_FUNC, 0);	\
	}								\
									\
	return w;							\
}									\
									\
static inline unsigned int XDP2_JOIN4(xdp2_rbitmap, SUFFIX, _, NAME)(	\
				      const TYPE *addr,			\
				      unsigned int pos,			\
				      unsigned int nbits,		\
				      unsigned int num_words)		\
{									\
	unsigned int w = nbits;						\
									\
	__XDP2_BITMAP_1ARG(OP, addr, pos + (POS_ADD), nbits,		\
			   0, 0, 1, 0, w, const, _dummy_dest,		\
			   NUM_WORD_BITS, TYPE, CONV_FUNC, num_words);	\
									\
	if (ROLL && w >= nbits)						\
		__XDP2_BITMAP_1ARG(OP, addr, 0, nbits,			\
				   0, 0, 1, 0, w, const, _dummy_dest,	\
				   NUM_WORD_BITS, TYPE, CONV_FUNC,	\
				   num_words);				\
	return w;							\
}


/******** Helper macros for two argument functions */

/* Helper macro to create a function for an operation on two arguments.
 * If DESTSRC is set, then dest is both a source and destination
 */
#define __XDP2_BITMAP_2ARG_FUNCT(NAME, DESTSRC, OP, NOT, S1_OP, S2_OP,	\
				 NUM_WORD_BITS, TYPE, SUFFIX,		\
				 CONV_FUNC, SCONST, WNAME, SPTR,	\
				 SAMP, WSRC)				\
static inline void XDP2_JOIN5(xdp2_bitmap, SUFFIX, _, WNAME, NAME)(	\
			TYPE *dest, SCONST TYPE SPTR src,		\
			unsigned int pos, unsigned int nbits)		\
{									\
	unsigned int w;							\
									\
	__XDP2_BITMAP_2ARG(DESTSRC, OP, NOT, S1_OP, S2_OP, dest,	\
			   SAMP src, pos, nbits, 0, 0, w,, _dest,	\
			   NUM_WORD_BITS, TYPE, CONV_FUNC, WSRC, 0);	\
}									\
									\
static inline void XDP2_JOIN5(xdp2_rbitmap, SUFFIX, _, WNAME, NAME)(	\
			TYPE *dest, SCONST TYPE SPTR src,		\
			unsigned int pos, unsigned int nbits,		\
			unsigned int num_words)				\
{									\
	unsigned int w;							\
									\
	__XDP2_BITMAP_2ARG(DESTSRC, OP, NOT, S1_OP, S2_OP, dest,	\
			   SAMP src, pos, nbits, 0, 0, w,, _dest,	\
			   NUM_WORD_BITS, TYPE, CONV_FUNC, WSRC,	\
			   num_words);					\
}

/* Helper macro to create a function for an operation on two arguments
 * that also returns a test result (e.g. resulting weight). If DESTSRC
 * is set, then dest is both a source and destination
 */
#define __XDP2_BITMAP_2ARG_TEST_FUNCT(NAME, DESTSRC, OP, NOT, S1_OP,	\
				      S2_OP, NUM_WORD_BITS, TYPE,	\
				      SUFFIX, CONV_FUNC, SCONST,	\
				      WNAME, SPTR, SAMP, WSRC)		\
static inline unsigned int						\
		XDP2_JOIN6(xdp2_bitmap, SUFFIX, _, WNAME, NAME, _test)(	\
			TYPE *dest, SCONST TYPE SPTR src,		\
			unsigned int pos, unsigned int nbits,		\
			unsigned int test)				\
{									\
	unsigned int w = 0;						\
									\
	__XDP2_BITMAP_2ARG(DESTSRC, OP, NOT, S1_OP, S2_OP, dest,	\
			   SAMP src, pos, nbits, 0, test, w,, _dest,	\
			   NUM_WORD_BITS, TYPE, CONV_FUNC, WSRC, 0);	\
	return w;							\
}									\
									\
static inline unsigned int						\
		XDP2_JOIN6(xdp2_rbitmap, SUFFIX, _, WNAME, NAME,	\
			   _test)(					\
			TYPE *dest, SCONST TYPE SPTR src,		\
			unsigned int pos, unsigned int nbits,		\
			unsigned int test, unsigned int num_words)	\
{									\
	unsigned int w = 0;						\
									\
	__XDP2_BITMAP_2ARG(DESTSRC, OP, NOT, S1_OP, S2_OP, dest,	\
			   SAMP src, pos, nbits, 0, test, w,, _dest,	\
			   NUM_WORD_BITS, TYPE, CONV_FUNC, WSRC,	\
			   num_words);					\
	return w;							\
}

/* Helper macro to create a function for a compare operation between
 * two arguments that returns a position for the bit that meets the
 * test characteristics
 */
#define __XDP2_BITMAP_2ARG_CMP_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,	\
				     NUM_WORD_BITS, TYPE, SUFFIX,	\
				     CONV_FUNC,	SCONST, WNAME, SPTR,	\
				     SAMP, WSRC)			\
static inline unsigned int XDP2_JOIN5(xdp2_bitmap, SUFFIX, _, WNAME,	\
				      NAME)(				\
			SCONST TYPE SPTR src1, const TYPE *src2,	\
			unsigned int pos, unsigned int nbits)		\
{									\
	unsigned int w = nbits;						\
									\
	__XDP2_BITMAP_2ARG(1, OP, NOT, S1_OP, S2_OP, SAMP src1, src2,	\
			   pos, nbits, 1, 0, w, const, _dummy_dest,	\
			   NUM_WORD_BITS, TYPE, CONV_FUNC, WSRC, 0);	\
	return w;							\
}									\
									\
static inline unsigned int XDP2_JOIN5(xdp2_rbitmap, SUFFIX, _,		\
				      WNAME, NAME)(			\
			SCONST TYPE SPTR src1, const TYPE *src2,	\
			unsigned int pos, unsigned int nbits,		\
			unsigned int num_words)				\
{									\
	unsigned int w = nbits;						\
									\
	__XDP2_BITMAP_2ARG(1, OP, NOT, S1_OP, S2_OP, SAMP src1, src2,	\
			   pos, nbits, 1, 0, w, const, _dummy_dest,	\
			   NUM_WORD_BITS, TYPE, CONV_FUNC, WSRC,	\
			   num_words);					\
	return w;							\
}


/* Helper macro to create a function for an operation on two arguments,
 * where a position is specified for each argument. If DESTSRC is set,
 * then dest is both a source and destination
 */
#define __XDP2_BITMAP_2ARG_GEN_FUNCT(NAME, DESTSRC, OP, NOT, S1_OP,	\
				     S2_OP, NUM_WORD_BITS, TYPE,	\
				     SUFFIX, CONV_FUNC, SCONST, WNAME,	\
				     SPTR, SAMP, WSRC)			\
static inline void XDP2_JOIN6(xdp2_bitmap, SUFFIX, _, WNAME, NAME,	\
			      _gen)(					\
			TYPE *dest, unsigned int dest_pos,		\
			SCONST TYPE SPTR src, unsigned int src_pos,	\
			unsigned int nbits)				\
{									\
	unsigned int w;							\
									\
	__XDP2_BITMAP_2ARG_GEN(DESTSRC, OP, NOT, S1_OP, S2_OP, dest,	\
			       dest_pos, SAMP src, src_pos, nbits, 0,	\
			       0, w,, _dest, NUM_WORD_BITS, TYPE,	\
			       CONV_FUNC, WSRC, 0);			\
}									\
									\
static inline void XDP2_JOIN6(xdp2_rbitmap, SUFFIX, _, WNAME,		\
			      NAME, _gen)(				\
			TYPE *dest, unsigned int dest_pos,		\
			SCONST TYPE SPTR src, unsigned int src_pos,	\
			unsigned int nbits, unsigned int num_words)	\
{									\
	unsigned int w;							\
									\
	__XDP2_BITMAP_2ARG_GEN(DESTSRC, OP, NOT, S1_OP, S2_OP, dest,	\
			       dest_pos, SAMP src, src_pos, nbits, 0,	\
			       0, w,, _dest, NUM_WORD_BITS, TYPE,	\
			       CONV_FUNC, WSRC, num_words);		\
}

/* Helper macro to create a function for an operation on two arguments,
 * where a position is specified for each argument, that also returns a
 * test result (e.g. resulting weight). If DESTSRC is set, then dest is
 * both a source and destination
 */
#define __XDP2_BITMAP_2ARG_GEN_TEST_FUNCT(NAME, DESTSRC, OP, NOT,	\
					  S1_OP, S2_OP, NUM_WORD_BITS,	\
					  TYPE, SUFFIX, CONV_FUNC,	\
					  SCONST, WNAME, SPTR, SAMP,	\
					  WSRC)				\
static inline unsigned int XDP2_JOIN6(xdp2_bitmap, SUFFIX,		\
				      _, WNAME, NAME, _test_gen)(	\
			TYPE *dest, unsigned int dest_pos,		\
			SCONST TYPE SPTR src, unsigned int src_pos,	\
			unsigned int nbits, unsigned int test)		\
{									\
	unsigned int w = 0;						\
									\
	__XDP2_BITMAP_2ARG_GEN(DESTSRC, OP, NOT, S1_OP, S2_OP, dest,	\
				dest_pos, SAMP src, src_pos, nbits,	\
				0, test, w,, _dest, NUM_WORD_BITS,	\
				TYPE, CONV_FUNC, WSRC, 0);		\
	return w;							\
}									\
									\
static inline unsigned int XDP2_JOIN6(xdp2_rbitmap, SUFFIX,		\
				      _, WNAME, NAME, _test_gen)(	\
			TYPE *dest, unsigned int dest_pos,		\
			SCONST TYPE SPTR src, unsigned int src_pos,	\
			unsigned int nbits, unsigned int test,		\
			unsigned int num_words)				\
{									\
	unsigned int w = 0;						\
									\
	__XDP2_BITMAP_2ARG_GEN(DESTSRC, OP, NOT, S1_OP, S2_OP, dest,	\
				dest_pos, SAMP src, src_pos, nbits,	\
				0, test, w,, _dest, NUM_WORD_BITS,	\
				TYPE, CONV_FUNC, WSRC, num_words);	\
	return w;							\
}

/* Helper macro to create a function for a compare operation between
 * two arguments where a position is specified for each argument,  that
 * returns a position for the bit that meets the test characteristics
 */
#define __XDP2_BITMAP_2ARG_CMP_GEN_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,	\
					 NUM_WORD_BITS, TYPE,		\
					 SUFFIX, CONV_FUNC)		\
static inline unsigned int XDP2_JOIN5(xdp2_bitmap, SUFFIX,		\
				      _, NAME, _gen)(			\
			const TYPE *src1, unsigned int src1_pos,	\
			const TYPE *src2, unsigned int src2_pos,	\
			unsigned int nbits)				\
{									\
	unsigned int w = nbits;						\
									\
	__XDP2_BITMAP_2ARG_GEN(1, OP, NOT, S1_OP, S2_OP,		\
			       src1, src1_pos, src2, src2_pos, nbits,	\
			       1, 0, w, const, _dummy_dest,		\
			       NUM_WORD_BITS, TYPE, CONV_FUNC,		\
			       false, 0);				\
	return w;							\
}									\
									\
static inline unsigned int XDP2_JOIN5(xdp2_rbitmap, SUFFIX,		\
				      _, NAME, _gen)(			\
			const TYPE *src1, unsigned int src1_pos,	\
			const TYPE *src2, unsigned int src2_pos,	\
			unsigned int nbits, unsigned int num_words)	\
{									\
	unsigned int w = nbits;						\
									\
	__XDP2_BITMAP_2ARG_GEN(1, OP, NOT, S1_OP, S2_OP,		\
			       src1, src1_pos, src2, src2_pos, nbits,	\
			       1, 0, w, const, _dummy_dest,		\
			       NUM_WORD_BITS, TYPE, CONV_FUNC,		\
			       false, num_words);			\
	return w;							\
}

/******** Helper macros for three argument functions */

/* Helper macro to create a two source and one destination function
 * where the bitmaps have the same characteristics (that is same start
 * positions)
 */
/* Helper macro to create a function for an operation on three arguments
 * (one destination and two sources)
 */
#define __XDP2_BITMAP_3ARG_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,		\
				 NUM_WORD_BITS, TYPE, SUFFIX,		\
				 CONV_FUNC, SCONST, WNAME, SPTR, SAMP,	\
				 WSRC)					\
static inline void XDP2_JOIN5(xdp2_bitmap, SUFFIX, _, WNAME, NAME)(	\
			TYPE *dest, SCONST TYPE SPTR src1,		\
			const TYPE *src2, unsigned int pos,		\
			unsigned int nbits)				\
{									\
	unsigned int w;							\
									\
	__XDP2_BITMAP_3ARG(OP, NOT, S1_OP, S2_OP, dest, SAMP src1,	\
			   src2, pos, nbits, 0, w, NUM_WORD_BITS,	\
			   TYPE, CONV_FUNC, WSRC, 0);			\
}									\
									\
static inline void XDP2_JOIN5(xdp2_rbitmap, SUFFIX, _, WNAME, NAME)(	\
			TYPE *dest, SCONST TYPE SPTR src1,		\
			const TYPE *src2, unsigned int pos,		\
			unsigned int nbits, unsigned int num_words)	\
{									\
	unsigned int w;							\
									\
	__XDP2_BITMAP_3ARG(OP, NOT, S1_OP, S2_OP, dest, SAMP src1,	\
			   src2, pos, nbits, 0, w, NUM_WORD_BITS,	\
			   TYPE, CONV_FUNC, WSRC, num_words);		\
}

/* Helper macro to create a function for an operation on three arguments
 * that also returns a test result (e.g. resulting weight).
 */
#define __XDP2_BITMAP_3ARG_TEST_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,	\
				      NUM_WORD_BITS, TYPE, SUFFIX,	\
				      CONV_FUNC, SCONST, WNAME, SPTR,	\
				      SAMP, WSRC)			\
static inline unsigned int XDP2_JOIN6(xdp2_bitmap,			\
				      SUFFIX, _, WNAME, NAME, _test)(	\
		TYPE *dest, SCONST TYPE SPTR src1, TYPE *src2,		\
		unsigned int pos, unsigned int nbits,			\
		unsigned int test)					\
{									\
	unsigned int w = 0;						\
									\
	__XDP2_BITMAP_3ARG(OP, NOT, S1_OP, S2_OP, dest, SAMP src1,	\
			   src2, pos, nbits, test, w,			\
			   NUM_WORD_BITS, TYPE, CONV_FUNC, WSRC, 0);	\
									\
	return w;							\
}									\
									\
static inline unsigned int XDP2_JOIN6(xdp2_rbitmap,			\
				      SUFFIX, _, WNAME, NAME, _test)(	\
		TYPE *dest, SCONST TYPE SPTR src1,			\
		const TYPE *src2, unsigned int pos, unsigned int nbits,	\
		unsigned int test, unsigned int num_words)		\
{									\
	unsigned int w = 0;						\
									\
	__XDP2_BITMAP_3ARG(OP, NOT, S1_OP, S2_OP, dest, SAMP src1,	\
			   src2, pos, nbits, test, w, NUM_WORD_BITS,	\
			   TYPE, CONV_FUNC, WSRC, num_words);		\
									\
	return w;							\
}

/* Helper macro to create a two source and one destination function for
 * bitmaps that can have different characteristics (that is different start
 * positions).
 */
#define __XDP2_BITMAP_3ARG_GEN_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,	\
				     NUM_WORD_BITS, TYPE, SUFFIX,	\
				     CONV_FUNC,	SCONST, WNAME, SPTR,	\
				     SAMP, WSRC)			\
static inline void XDP2_JOIN6(xdp2_bitmap, SUFFIX, _, WNAME, NAME,	\
			      _gen)(					\
		TYPE *dest, unsigned int dest_pos,			\
		SCONST TYPE SPTR src1, unsigned int src1_pos,		\
		const TYPE *src2, unsigned int src2_pos,		\
		unsigned int nbits)					\
{                                                                       \
	unsigned int w;							\
									\
	__XDP2_BITMAP_3ARG_GEN(OP, NOT, S1_OP, S2_OP, dest, dest_pos,	\
			       SAMP src1, src1_pos, src2, src2_pos,	\
			       nbits, 0, w, NUM_WORD_BITS, TYPE,	\
			       CONV_FUNC, WSRC, 0);			\
}									\
									\
static inline void XDP2_JOIN6(xdp2_rbitmap, SUFFIX, _, WNAME,		\
			      NAME, _gen)(				\
		TYPE *dest, unsigned int dest_pos,			\
		SCONST TYPE SPTR src1, unsigned int src1_pos,		\
		const TYPE *src2, unsigned int src2_pos,		\
		unsigned int nbits, unsigned int num_words)		\
{                                                                       \
	unsigned int w;							\
									\
	__XDP2_BITMAP_3ARG_GEN(OP, NOT, S1_OP, S2_OP, dest, dest_pos,	\
			       SAMP src1, src1_pos, src2, src2_pos,	\
			       nbits, 0, w, NUM_WORD_BITS, TYPE,	\
			       CONV_FUNC, WSRC, num_words);		\
}

/* Helper macro to create a function for an operation on three arguments,
 * where a position is specified for each argument
 */
#define __XDP2_BITMAP_3ARG_TEST_GEN_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,	\
					  NUM_WORD_BITS, TYPE, SUFFIX,	\
					  CONV_FUNC, SCONST, WNAME,	\
					  SPTR, SAMP, WSRC)		\
static inline unsigned int XDP2_JOIN6(xdp2_bitmap, SUFFIX, _,		\
				      WNAME, NAME, _test_gen)(		\
		TYPE *dest, unsigned int dest_pos,			\
		SCONST TYPE SPTR src1, unsigned int src1_pos,		\
		const TYPE *src2, unsigned int src2_pos,		\
		unsigned int nbits, unsigned int test)			\
{                                                                       \
	unsigned int w = 0;						\
									\
	__XDP2_BITMAP_3ARG_GEN(OP, NOT, S1_OP, S2_OP, dest, dest_pos,	\
			       SAMP src1, src1_pos, src2, src2_pos,	\
			       nbits, test, w, NUM_WORD_BITS,		\
			       TYPE, CONV_FUNC, WSRC, 0);		\
	return w;							\
}									\
									\
static inline unsigned int XDP2_JOIN6(xdp2_rbitmap, SUFFIX, _,		\
				      WNAME, NAME, _test_gen)(		\
		TYPE *dest, unsigned int dest_pos,			\
		SCONST TYPE SPTR src1, unsigned int src1_pos,		\
		const TYPE *src2, unsigned int src2_pos,		\
		unsigned int nbits, unsigned int test,			\
		unsigned int num_words)					\
{                                                                       \
	unsigned int w = 0;						\
									\
	__XDP2_BITMAP_3ARG_GEN(OP, NOT, S1_OP, S2_OP, dest, dest_pos,	\
			       SAMP src1, src1_pos, src2, src2_pos,	\
			       nbits, test, w, NUM_WORD_BITS,		\
			       TYPE, CONV_FUNC, WSRC, num_words);	\
	return w;							\
}

/* Macros and function for bitmap shift left */

#define __XDP2_BITMAP_SHIFT_LEFT(DEST, SRC, NUM_SHIFT, NBITS,		\
				 NUM_WORD_BITS, TYPE, CONV_FUNC,	\
				 WORDS_IN_BYTES) do {			\
	unsigned int num_words = 8 * (WORDS_IN_BYTES) / (NUM_WORD_BITS);\
	unsigned int _num_bit_shift = num_shift % NUM_WORD_BITS;	\
	unsigned int _src_offset, _dest_offset;				\
	int _dir = num_words ? -1 : 1, _i;				\
	const TYPE *_src;						\
	TYPE *_dest;							\
									\
	if (!(NBITS))							\
		break;							\
	_src_offset = ((NBITS - 1) / NUM_WORD_BITS) -			\
					(NUM_SHIFT / NUM_WORD_BITS);	\
	_dest_offset = (NBITS - 1) / NUM_WORD_BITS;			\
									\
	if (num_words) {						\
		_src = ((SRC) + ((num_words) - 1)) - _src_offset;	\
		_dest = ((DEST) + ((num_words) - 1)) - _dest_offset;	\
	} else {							\
		_src = (SRC) + _src_offset;				\
		_dest = (DEST) + _dest_offset;				\
	}								\
	/* Shift is perfectly aligned */				\
	if (!_num_bit_shift) {						\
		for (_i = 0; _i < (NBITS - NUM_SHIFT) /			\
						NUM_WORD_BITS; _i++) {	\
			*_dest = *_src;					\
			_dest -= _dir, _src -= _dir;			\
		}							\
	} else {							\
		/* Handle whole word shifts */				\
		for (_i = 0; _i < (NBITS - NUM_SHIFT) / NUM_WORD_BITS;	\
		     _src -= _dir, _i++) {				\
			*_dest = __XDP2_BITMAP_SHIFT_CONVERT(		\
				*_src, << _num_bit_shift, CONV_FUNC) |	\
			    __XDP2_BITMAP_SHIFT_CONVERT(		\
				_src[-_dir], >>	(NUM_WORD_BITS -	\
					_num_bit_shift), CONV_FUNC);	\
			_dest -= _dir;					\
		}							\
									\
		/* Final partial word */				\
		*_dest = __XDP2_BITMAP_SHIFT_CONVERT(*_src,		\
			<< _num_bit_shift, CONV_FUNC);			\
		_dest -= _dir;						\
		_i++;							\
	}								\
									\
	/* Zero out words shifted out */				\
	for (; _i < __XDP2_BITMAP_DIV(NBITS, NUM_WORD_BITS); _i++) {	\
		*_dest = 0;						\
		_dest -= _dir;						\
	}								\
} while (0)

#define __XDP2_BITMAP_SHIFT_LEFT_FUNC(NUM_WORD_BITS, TYPE, SUFFIX,	\
				      CONV_FUNC)			\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX, _shift_gen_left)(	\
		TYPE *dest, const TYPE *src,				\
		unsigned int num_shift, unsigned int nbits,		\
		unsigned int num_words)					\
{									\
	unsigned int words_in_bytes = 0;				\
									\
	if (num_words)							\
		words_in_bytes = XDP2_BITS_TO_BYTES(num_words *		\
						    NUM_WORD_BITS);	\
									\
	XDP2_ASSERT(!(nbits % NUM_WORD_BITS), "Number of bits for "	\
		    "shift must be a multiple of words");		\
									\
	__XDP2_BITMAP_SHIFT_LEFT(dest, src, num_shift, nbits,		\
			NUM_WORD_BITS, TYPE, CONV_FUNC,			\
			words_in_bytes);				\
}									\
									\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX, _shift_left)(	\
		TYPE *dest, const TYPE *src,				\
		unsigned int num_shift, unsigned int nbits)		\
{									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _shift_gen_left)(dest, src,	\
		   num_shift, nbits, 0);				\
}									\
									\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX, _destsrc_shift_left)(\
		TYPE *dest, unsigned int num_shift, unsigned int nbits)	\
{									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _shift_left)(dest, dest,	\
		   num_shift, nbits);					\
}									\
									\
static inline void XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _shift_left)(	\
		TYPE *dest, const TYPE *src,				\
		unsigned int num_shift, unsigned int nbits,		\
		unsigned int num_words)					\
{									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _shift_gen_left)(dest, src,	\
		   num_shift, nbits, num_words);			\
}									\
									\
static inline void XDP2_JOIN3(xdp2_rbitmap, SUFFIX,			\
			      _destsrc_shift_left)(			\
		TYPE *dest, unsigned int num_shift, unsigned int nbits,	\
		unsigned int num_words)					\
{									\
	XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _shift_left)(dest, dest,	\
		   num_shift, nbits, num_words);			\
}

/* Macros and function for bitmap shift right */

#define __XDP2_BITMAP_SHIFT_RIGHT(DEST, SRC, NUM_SHIFT, NBITS,		\
				  NUM_WORD_BITS, TYPE, CONV_FUNC,	\
				  WORDS_IN_BYTES) do {			\
	unsigned int num_words = (8 * WORDS_IN_BYTES) / (NUM_WORD_BITS);\
	unsigned int _num_bit_shift = NUM_SHIFT % NUM_WORD_BITS;	\
	int _dir = num_words ? -1 : 1, _i;				\
	unsigned int _offset;						\
	const TYPE *_src;						\
	TYPE *_dest;							\
									\
	if (!(NBITS))							\
		break;							\
	_offset = ((NBITS - 1) / NUM_WORD_BITS) -			\
			((NBITS - NUM_SHIFT - 1) / NUM_WORD_BITS);	\
									\
	if (num_words) {						\
		_src = ((SRC) + ((num_words) - 1)) - _offset;		\
		_dest = ((DEST) + ((num_words) - 1));			\
	} else {							\
		_src = (SRC) + _offset;					\
		_dest = (DEST);						\
	}								\
									\
	/* Shift is perfectly aligned */				\
	if (!_num_bit_shift) {						\
		for (_i = (NBITS - 1) / NUM_WORD_BITS; _i >=		\
				(int)(NUM_SHIFT) / NUM_WORD_BITS;	\
				_i--) {					\
			*_dest = *_src;					\
			_dest += _dir; _src += _dir;			\
		}							\
	} else {							\
		/* Handle whole word shifts */				\
		for (_i = (NBITS - 1) / NUM_WORD_BITS;			\
		     _i >= (NUM_SHIFT + NUM_WORD_BITS) /		\
				NUM_WORD_BITS; _src += _dir, _i--) {	\
			*_dest = __XDP2_BITMAP_SHIFT_CONVERT(		\
				*_src, >> _num_bit_shift, CONV_FUNC) |	\
				 __XDP2_BITMAP_SHIFT_CONVERT(		\
				_src[_dir], <<	(NUM_WORD_BITS -	\
					_num_bit_shift), CONV_FUNC);	\
			_dest += _dir;					\
		}							\
									\
		/* Final partial word */				\
		*_dest = __XDP2_BITMAP_SHIFT_CONVERT(*_src,		\
				>> _num_bit_shift, CONV_FUNC);		\
		_dest += _dir;						\
		_i--;							\
	}								\
	/* Zero out any words shifted out */				\
	for (; _i >= 0; _i--, _dest += _dir)				\
		*_dest = 0;						\
} while (0)

#define __XDP2_BITMAP_SHIFT_RIGHT_FUNC(NUM_WORD_BITS, TYPE,		\
				       SUFFIX, CONV_FUNC)		\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX, _shift_gen_right)(	\
		TYPE *dest, const TYPE *src,				\
		unsigned int num_shift, unsigned int nbits,		\
		unsigned int num_words)					\
{									\
	unsigned int words_in_bytes = 0;				\
									\
	if (num_words)							\
		words_in_bytes = XDP2_BITS_TO_BYTES(num_words *		\
						    NUM_WORD_BITS);	\
									\
	XDP2_ASSERT(!(nbits % NUM_WORD_BITS), "Number of bits for "	\
		    "shift must be a multiple of words");		\
									\
	__XDP2_BITMAP_SHIFT_RIGHT(dest, src, num_shift, nbits,		\
			NUM_WORD_BITS, TYPE, CONV_FUNC, words_in_bytes);\
}									\
									\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX, _shift_right)(	\
		TYPE *dest, const TYPE *src,				\
		unsigned int num_shift, unsigned int nbits)		\
{									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _shift_gen_right)(dest, src,	\
		   num_shift, nbits, 0);				\
}									\
									\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX,			\
			      _destsrc_shift_right)(			\
		TYPE *dest,						\
		unsigned int num_shift, unsigned int nbits)		\
{									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _shift_right)(dest, dest,	\
		   num_shift, nbits);					\
}									\
									\
static inline void XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _shift_right)(	\
		TYPE *dest, const TYPE *src,				\
		unsigned int num_shift, unsigned int nbits,		\
		unsigned int num_words)					\
{									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _shift_gen_right)(dest, src,	\
		   num_shift, nbits, num_words);			\
}									\
									\
static inline void XDP2_JOIN3(xdp2_rbitmap, SUFFIX,			\
			      _destsrc_shift_right)(			\
		TYPE *dest,			\
		unsigned int num_shift, unsigned int nbits,		\
		unsigned int num_words)					\
{									\
	XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _shift_right)(dest, dest,	\
		   num_shift, nbits, num_words);			\
}

/* Main logic for rotating a multi-word bitmap. Rotate is a bit on the
 * tricky side since we need to be careful to not scribble over data
 * as we place it. We do the rotate in two loops.
 *
 * In the first loop, we rotate by the number of rotate bits modulo
 * the word size. This aligns the rotate data to their proper offset
 * relative to word size.
 *
 * In the second loop we rotate the data by the number of rotate bits
 * divided by word size. We do this by looping over the rings formed
 * by the rotate. For instance, suppose we have four words and are rotating
 * by two words. That make two rings to loop over by moving words as
 * 0->2->0 amd 1->3->1. If we have five words, two is relatively prime so
 * there's only 0->2->4->1->3->0
 */

#define __XDP2_BITMAP_MAKE_ROTATE(DEST, SRC, ROTATE_BITS, NBITS,	\
				  NUM_WORD_BITS, TYPE, CONV_FUNC,	\
				  NUM_WORDS) do {			\
	unsigned int _rotate_words = ROTATE_BITS / NUM_WORD_BITS;	\
	unsigned int _rotate_bits = ROTATE_BITS %  NUM_WORD_BITS;	\
	unsigned int _opp_rotate_bits = NUM_WORD_BITS - _rotate_bits;	\
	unsigned int _nbits_words = NBITS / NUM_WORD_BITS, _count;	\
	TYPE _word, _word_next, *_idest, _mask_low, _mask_high;		\
	int _dir = (NUM_WORDS) ? -1 : 1, _index, _base, _i;		\
	const TYPE *_src = (SRC), *_fsrc;				\
	TYPE *_dest = (DEST), *_odest;					\
									\
	if (!(NBITS))							\
		break;							\
	if (NUM_WORDS) {						\
		_src = ((SRC) + ((NUM_WORDS) - 1));			\
		_dest = (DEST) + ((NUM_WORDS) - 1);			\
	} else {							\
		_src = (SRC);						\
		_dest = (DEST);						\
	}								\
									\
	_odest = _dest;							\
	_fsrc = _src;							\
									\
	if (_rotate_bits) {						\
		/* Rotate by number less than word size to align */	\
		_mask_low = CONV_FUNC(__XDP2_BITMAP_1S(TYPE) <<		\
							_rotate_bits);	\
		_mask_high = CONV_FUNC(__XDP2_BITMAP_1S(TYPE) >>	\
						_opp_rotate_bits);	\
									\
		for (_idest = _dest, _word = *_src,			\
		     _i = 0; _i < _nbits_words - 1; _i++,		\
		     _word = _word_next, _idest += _dir, _src += _dir) {\
			*_idest = (*_idest & ~_mask_low) |		\
			__XDP2_BITMAP_SHIFT_CONVERT(_word,		\
				 << _rotate_bits, CONV_FUNC);		\
			_word_next = _src[_dir];			\
			_idest[_dir] = (_idest[_dir] & ~_mask_high) |	\
			__XDP2_BITMAP_SHIFT_CONVERT(_word,		\
				 >> _opp_rotate_bits, CONV_FUNC);	\
									\
			_word = _word_next;				\
		}							\
		*_idest = (*_idest & ~_mask_low) |			\
			__XDP2_BITMAP_SHIFT_CONVERT(_word,		\
				 << _rotate_bits, CONV_FUNC);		\
		*_odest = (*_odest & ~_mask_high) |			\
			__XDP2_BITMAP_SHIFT_CONVERT(_word,		\
				>> _opp_rotate_bits, CONV_FUNC);	\
		_fsrc = _dest;						\
	}								\
									\
	if (!_rotate_words)						\
		return;							\
									\
	/* Now rotate complete words by looping over each ring (number	\
	 * of rings is number of words divided by number of words to	\
	 * rotate)							\
	 */								\
	for (_base = 0, _count = _nbits_words; _count; _base++) {	\
		_word = _fsrc[_dir * _base];				\
		_index = _base;						\
		/* Loop over ring */					\
		do {							\
			_index = (_index + _rotate_words) %		\
							_nbits_words;	\
			_word_next = _fsrc[_dir * _index];		\
			_dest[_dir * _index] = _word;			\
			_word = _word_next;				\
			_count--;					\
		} while (_index != _base);				\
	}								\
} while (0)

#define __XDP2_BITMAP_ROTATE_FUNC(NUM_WORD_BITS, TYPE, SUFFIX,		\
				  CONV_FUNC)				\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX, _rotate_gen_left)(	\
		TYPE *dest, const TYPE *src,				\
		unsigned int num_rotate, unsigned int nbits,		\
		unsigned int num_words)					\
{									\
	if (!nbits)							\
		return;							\
									\
	XDP2_ASSERT(!(nbits % NUM_WORD_BITS),				\
		    "Number of bits %u needs to be a multiple of "	\
		    "words for rotate: %u bits",			\
		    num_rotate, NUM_WORD_BITS);				\
									\
	num_rotate = num_rotate % nbits;				\
									\
	if (!num_rotate) {						\
		if (src != dest) {					\
			if (num_words) {				\
				dest = &dest[num_words - nbits /	\
							NUM_WORD_BITS];	\
				src = &src[num_words - nbits /		\
							NUM_WORD_BITS];	\
			}						\
			memcpy(dest, src, XDP2_BITS_TO_BYTES(nbits));	\
		}							\
		return;							\
	}								\
	if (nbits == NUM_WORD_BITS) {					\
		if (!num_words)						\
			dest[0] = CONV_FUNC(XDP2_JOIN2(			\
				xdp2_bitmap_word_rol,			\
				NUM_WORD_BITS)(CONV_FUNC(src[0]),	\
				num_rotate));				\
		else							\
			dest[num_words - 1] = CONV_FUNC(XDP2_JOIN2(	\
				xdp2_bitmap_word_rol,			\
				NUM_WORD_BITS)(				\
				CONV_FUNC(src[num_words - 1]),		\
							num_rotate));	\
		return;							\
	}								\
	__XDP2_BITMAP_MAKE_ROTATE(dest, src, num_rotate, nbits,		\
				  NUM_WORD_BITS, TYPE, CONV_FUNC,	\
				  num_words);				\
}									\
									\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX, _rotate_left)(	\
		TYPE *dest, const TYPE *src,				\
		unsigned int num_rotate, unsigned int nbits)		\
{									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX,_rotate_gen_left)(dest, src,	\
					     num_rotate, nbits, 0);	\
}									\
									\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX,			\
			      _destsrc_rotate_left)(			\
		TYPE *dest, unsigned int num_rotate,			\
		unsigned int nbits)					\
{									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _rotate_left)(dest, dest,	\
						      num_rotate,	\
						      nbits);		\
}									\
									\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX, _rotate_right)(	\
		TYPE *dest, const TYPE *src,				\
		unsigned int num_rotate, unsigned int nbits)		\
{									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX,_rotate_left)(dest, src,		\
						     nbits - num_rotate,\
						     nbits);		\
}									\
									\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX,			\
			      _destsrc_rotate_right)(			\
		TYPE *dest, unsigned int num_rotate,			\
		unsigned int nbits)					\
{									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _rotate_right)(dest, dest,	\
						       num_rotate,	\
						       nbits);		\
}									\
									\
static inline void XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _rotate_left)(	\
		TYPE *dest, const TYPE *src,				\
		unsigned int num_rotate, unsigned int nbits,		\
		unsigned int num_words)					\
{									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _rotate_gen_left)(dest, src,	\
				num_rotate, nbits, num_words);		\
}									\
									\
static inline void XDP2_JOIN3(xdp2_rbitmap, SUFFIX,			\
			      _destsrc_rotate_left)(			\
		TYPE *dest, unsigned int num_rotate,			\
		unsigned int nbits, unsigned int num_words)		\
{									\
	XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _rotate_left)(dest, dest,	\
				num_rotate, nbits, num_words);		\
}									\
									\
static inline void XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _rotate_right)(	\
		TYPE *dest, const TYPE *src,				\
		unsigned int num_rotate, unsigned int nbits,		\
		unsigned int num_words)					\
{									\
	XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _rotate_left)(dest, src,	\
		     nbits - num_rotate, nbits, num_words);		\
}									\
									\
static inline void XDP2_JOIN3(xdp2_rbitmap, SUFFIX,			\
			      _destsrc_rotate_right)(			\
		TYPE *dest, unsigned int num_rotate,			\
		unsigned int nbits, unsigned int num_words)		\
{									\
	XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _rotate_right)(dest, dest,	\
		      num_rotate, nbits, num_words);			\
}


/* Macros and functions for read/write words */

/* Swap bytes, half words or words. This is called when reading or
 * writing words with byte swap.
 *
 * If target bits is less than number of word bits then set the
 * zeroth target words to its swapped buddy
 *
 * If target bits is greater than number of word bits then swap the
 * target words
 *
 * If target bits equals number of word bits then nothing to do
 */
static inline __u64 __xdp2_bitmap_do_swap(__u64 val,
					 unsigned int num_word_bits,
					 unsigned int targ_bits)
{
	union {
		__u64 v64;
		__u32 v32[2];
		__u16 v16[4];
		__u8 v8[8];
	} v = { .v64 = val };
	__u64 tmp;

	switch (num_word_bits) {
	case 64:
		switch (targ_bits) {
		case 32:
			v.v32[0] = v.v32[1];
			break;
		case 16:
			v.v16[0] = v.v16[3];
			break;
		case 8:
			v.v8[0] = v.v8[7];
			break;
		case 64:
		default:
			break;
		}
		break;
	case 32:
		switch (targ_bits) {
		case 64:
			/* Reading two 32-bit words, swap them */
			tmp = v.v32[0];
			v.v32[0] = v.v32[1];
			v.v32[1] = tmp;

			break;
		case 16:
			v.v16[0] = v.v16[1];
			break;
		case 8:
			v.v8[0] = v.v8[3];
			break;
		case 32:
		default:
			break;
		}
		break;
	case 16:
		switch (targ_bits) {
		case 64:
			/* Reading four 16-bit words, swap them */
			tmp = v.v16[0];
			v.v16[0] = v.v16[3];
			v.v16[3] = tmp;

			tmp = v.v16[1];
			v.v16[1] = v.v16[2];
			v.v16[2] = tmp;

			break;
		case 32:
			/* Reading two 16-bit words, swap them */
			tmp = v.v16[0];
			v.v16[0] = v.v16[1];
			v.v16[1] = tmp;

			break;
		case 8:
			v.v8[0] = v.v8[1];
			break;
		case 16:
		default:
			break;
		}
		break;
	case 8: {
		switch (targ_bits) {
		case 64:
			/* Reading eight bytes, do full bswap */
			v.v64 = bswap_64(v.v64);
			break;
		case 32:
			/* Reading four bytes, just do bswap */
			v.v32[0] = bswap_32(v.v32[0]);
			break;
		case 16:
			/* Reading two bytes, just do bswap */
			v.v16[0] = bswap_16(v.v16[0]);
			break;
		case 8:
		default:
		}
	}
	default:
		break;
	}

	/* Only the low order number of target bits is relevant */
	return v.v64;
}

/* Swap bytes for reverse bitmaps. This is called when reading or
 * writing words with byte swap.
 *
 * If target bits is less than number of word bits then set the
 * leftmost target words to its swapped buddy
 *
 * If target bits is less than number of word bits then set the
 * last target bytes to the it's swapped buddy
 *
 * If target bits is greater than number of word bits then swap the
 * target words
 *
 * If target bits equals number of word bits then nothing to do
 */
static inline __u64 __xdp2_bitmap_do_rev_swap(__u64 val,
					      unsigned int num_word_bits,
					      unsigned int targ_bits)
{
	union {
		__u64 v64;
		__u32 v32[2];
		__u16 v16[4];
		__u8 v8[8];
	} v = { .v64 = val };
	__u64 tmp;

	switch (num_word_bits) {
	case 64:
		switch (targ_bits) {
		case 32:
			v.v32[1] = v.v32[0];
			break;
		case 16:
			v.v16[3] = v.v16[0];
			break;
		case 8:
			v.v8[7] = v.v8[0];
			break;
		case 64:
		default:
			break;
		}
		break;
	case 32:
		switch (targ_bits) {
		case 64:
			tmp = v.v32[0];
			v.v32[0] = v.v32[1];
			v.v32[1] = tmp;

			break;
		case 16:
			v.v16[1] = v.v16[0];
			break;
		case 8:
			v.v8[3] = v.v8[0];
			break;
		case 32:
		default:
			break;
		}
		break;
	case 16:
		switch (targ_bits) {
		case 64:
			tmp = v.v16[0];
			v.v16[0] = v.v16[3];
			v.v16[3] = tmp;

			tmp = v.v16[1];
			v.v16[1] = v.v16[2];
			v.v16[2] = tmp;

			break;
		case 32:
			tmp = v.v16[0];
			v.v16[0] = v.v16[1];
			v.v16[1] = tmp;

			break;
		case 8:
			v.v8[1] = v.v8[0];
			break;
		case 16:
		default:
			break;
		}
		break;
	case 8:
		switch (targ_bits) {
		case 64:
			v.v64 = bswap_64(v.v64);
			break;
		case 32:
			v.v32[0] = bswap_32(v.v32[0]);
			break;
		case 16:
			v.v16[0] = bswap_16(v.v16[0]);
			break;
		case 8:
		default:
			break;
		}
	default:
		break;
	}

	/* Only the high order number of target bits is relevant */
	return v.v64;
}

/* Macros to define functions to handle read and write word operations.
 * Note that these may not work on a big endian system (we are casting
 * _u64s to __8 arrarys and assuming v[0] is low order byte
 */
#define __XDP2_BITMAP_READ_WRITE_FUNC(NUM_WORD_BITS, TYPE, SUFFIX,	\
				      SWAP, TARGET_WORD_BITS)		\
static inline XDP2_JOIN2(__u, TARGET_WORD_BITS)				\
	XDP2_JOIN4(xdp2_bitmap, SUFFIX, _read,				\
		   TARGET_WORD_BITS)(					\
		const TYPE *src, unsigned int src_pos)			\
{									\
	union {								\
		__u64 v64;						\
		TYPE v[64 / NUM_WORD_BITS];				\
		XDP2_JOIN2(__u, TARGET_WORD_BITS)			\
					vt[64 / TARGET_WORD_BITS];	\
	} data = {};							\
									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _copy_gen)(			\
		data.v, 0, src, src_pos, TARGET_WORD_BITS);		\
									\
	if (SWAP)							\
		data.v64 = __xdp2_bitmap_do_swap(data.v64,		\
				NUM_WORD_BITS, TARGET_WORD_BITS);	\
									\
	return data.vt[0];						\
}									\
									\
static inline void XDP2_JOIN4(xdp2_bitmap, SUFFIX, _write,		\
			      TARGET_WORD_BITS)(			\
		XDP2_JOIN2(__u, TARGET_WORD_BITS) targ,			\
		TYPE *dest, unsigned int dest_pos)			\
{									\
	union {								\
		__u64 v64;						\
		TYPE v[64 / NUM_WORD_BITS];				\
		XDP2_JOIN2(__u, TARGET_WORD_BITS)			\
					vt[64 / TARGET_WORD_BITS];	\
	} data;								\
									\
	data.v64 = 0;							\
	data.vt[0] = targ;						\
	if (SWAP)							\
		data.v64 = __xdp2_bitmap_do_rev_swap(data.v64,		\
				NUM_WORD_BITS, TARGET_WORD_BITS);	\
									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _copy_gen)(dest,		\
		   dest_pos, data.v, 0, dest_pos + TARGET_WORD_BITS);	\
}									\
									\
static inline XDP2_JOIN2(__u, TARGET_WORD_BITS)				\
	XDP2_JOIN4(xdp2_rbitmap, SUFFIX, _read,				\
		   TARGET_WORD_BITS)(					\
		const TYPE *src, unsigned int src_pos,			\
		unsigned int num_words)					\
{									\
	union {								\
		__u64 v64;						\
		TYPE v[64 / NUM_WORD_BITS];				\
		XDP2_JOIN2(__u, TARGET_WORD_BITS)			\
					vt[64 / TARGET_WORD_BITS];	\
	} data = {};							\
	unsigned int adv;						\
									\
	adv = (TARGET_WORD_BITS - 1) / NUM_WORD_BITS;			\
	XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _copy_gen)(			\
		data.v + adv - (num_words - 1), 0, src, src_pos,	\
					TARGET_WORD_BITS,		\
		num_words);						\
									\
	if (SWAP && (NUM_WORD_BITS <= TARGET_WORD_BITS))		\
		;							\
	else if (!SWAP)							\
		data.v64 = __xdp2_bitmap_do_rev_swap(data.v64,		\
				NUM_WORD_BITS, TARGET_WORD_BITS);	\
	else {								\
		/* Data in input target word is not formatted as a	\
		 * reverse bitmap. Just oo regulary swap to handle	\
		 * endian conversion					\
		 */							\
		data.v64 = __xdp2_bitmap_do_swap(data.v64,		\
				NUM_WORD_BITS, TARGET_WORD_BITS);	\
	}								\
									\
	return data.vt[0];						\
}									\
									\
static inline void XDP2_JOIN4(xdp2_rbitmap, SUFFIX, _write,		\
			      TARGET_WORD_BITS)(			\
		XDP2_JOIN2(__u, TARGET_WORD_BITS) targ,			\
		TYPE *dest, unsigned int dest_pos,			\
		unsigned int num_words)					\
{									\
	union {								\
		__u64 v64;						\
		TYPE v[64 / NUM_WORD_BITS];				\
		XDP2_JOIN2(__u, TARGET_WORD_BITS)			\
					vt[64 / TARGET_WORD_BITS];	\
	} data;								\
	unsigned int adv;						\
									\
	data.vt[0] = targ;						\
	if (!SWAP || (NUM_WORD_BITS > TARGET_WORD_BITS))		\
		data.v64 = __xdp2_bitmap_do_rev_swap(data.v64,		\
				NUM_WORD_BITS, TARGET_WORD_BITS);	\
									\
	adv = (TARGET_WORD_BITS - 1) / NUM_WORD_BITS;			\
	XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _copy_gen)(dest,		\
		   dest_pos, data.v + adv - (num_words - 1), 0,		\
		   dest_pos + TARGET_WORD_BITS, num_words);		\
}

/******** Permute macros ********
 *
 * Convenience to generate various classes of bitmap operation functions
 */

/* Permute macro for single source and destination functions where the
 * arguments share a position. Permutes over test and non-test variants
 */
#define __XDP2_BITMAP_2ARG_PERMUTE_FUNCT(NAME, OP,  NUM_WORD_BITS,	\
					 TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_FUNCT(NAME, 0, &,, OP,, NUM_WORD_BITS,	\
				 TYPE, SUFFIX, CONV_FUNC, const,, *,,	\
				 false)					\
	__XDP2_BITMAP_2ARG_TEST_FUNCT(NAME, 0, &,, OP,, NUM_WORD_BITS,	\
				      TYPE, SUFFIX, CONV_FUNC, const,,	\
				      *,, false)			\
	__XDP2_BITMAP_2ARG_FUNCT(NAME, 0, &,, OP,, NUM_WORD_BITS,	\
				 TYPE, SUFFIX, CONV_FUNC,, wsrc_,, &,	\
				 true)					\
	__XDP2_BITMAP_2ARG_TEST_FUNCT(NAME, 0, &,, OP,, NUM_WORD_BITS,	\
				      TYPE, SUFFIX, CONV_FUNC,, wsrc_,,	\
				      &, true)

/* Permute macro for single source and destination functions where the
 * arguments share a position, and the destination is also a source
 * argument. Permutes over test and non-test variants
 */
#define __XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(NAME, OP, NOT,		\
						 S1_OP, S2_OP,		\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_FUNCT(NAME, 1, OP, NOT, S1_OP, S2_OP,	\
				 NUM_WORD_BITS, TYPE, SUFFIX,		\
				 CONV_FUNC, const,, *,, false)		\
	__XDP2_BITMAP_2ARG_TEST_FUNCT(NAME, 1, OP, NOT, S1_OP, S2_OP,	\
				      NUM_WORD_BITS, TYPE, SUFFIX,	\
				      CONV_FUNC, const,, *,, false)	\
	__XDP2_BITMAP_2ARG_FUNCT(NAME, 1, OP, NOT, S1_OP, S2_OP,	\
				 NUM_WORD_BITS, TYPE, SUFFIX,		\
				 CONV_FUNC,, wsrc_,, &, true)		\
	__XDP2_BITMAP_2ARG_TEST_FUNCT(NAME, 1, OP, NOT, S1_OP, S2_OP,	\
				      NUM_WORD_BITS, TYPE, SUFFIX,	\
				      CONV_FUNC,, wsrc_,, &, true)

/* Permute macro for single source and destination functions where the
 * arguments each have their own position. Permutes over test and
 * non-test variants
 */
#define __XDP2_BITMAP_2ARG_GEN_PERMUTE_FUNCT(NAME, OP, NUM_WORD_BITS,	\
					     SUFFIX, TYPE, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_GEN_FUNCT(NAME, 0, &,, OP,, NUM_WORD_BITS,	\
				     TYPE, SUFFIX, CONV_FUNC,		\
				     const,, *,, false)			\
	__XDP2_BITMAP_2ARG_GEN_TEST_FUNCT(NAME, 0, &,, OP,,		\
					  NUM_WORD_BITS, TYPE, SUFFIX,	\
					  CONV_FUNC, const,, *,, false) \
	__XDP2_BITMAP_2ARG_GEN_FUNCT(NAME, 0, &,, OP,, NUM_WORD_BITS,	\
				     TYPE, SUFFIX, CONV_FUNC,,		\
				     wsrc_,, &, true)			\
	__XDP2_BITMAP_2ARG_GEN_TEST_FUNCT(NAME, 0, &,, OP,,		\
					  NUM_WORD_BITS, TYPE, SUFFIX,	\
					  CONV_FUNC,, wsrc_,, &, true)

/* Permute macro for single source and destination functions where the
 * arguments each have their own position, and the destination is also a
 * source argument. Permutes over test and non-test variants
 */
#define __XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(NAME, OP, NOT,	\
						     S1_OP, S2_OP,	\
						     NUM_WORD_BITS,	\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_GEN_FUNCT(NAME, 1, OP, NOT, S1_OP, S2_OP,	\
				     NUM_WORD_BITS, TYPE, SUFFIX,	\
				     CONV_FUNC, const,, *,, false)	\
	__XDP2_BITMAP_2ARG_GEN_TEST_FUNCT(NAME, 1, OP, NOT, S1_OP,	\
					  S2_OP, NUM_WORD_BITS,		\
					  TYPE, SUFFIX, CONV_FUNC,	\
					  const,, *,, false)		\
	__XDP2_BITMAP_2ARG_GEN_FUNCT(NAME, 1, OP, NOT, S1_OP, S2_OP,	\
				     NUM_WORD_BITS, TYPE, SUFFIX,	\
				     CONV_FUNC,, wsrc_,, &, true)	\
	__XDP2_BITMAP_2ARG_GEN_TEST_FUNCT(NAME, 1, OP, NOT, S1_OP,	\
					  S2_OP, NUM_WORD_BITS,	TYPE,	\
					  SUFFIX, CONV_FUNC,,		\
					  wsrc_,, &, true)

/* Permute macro for two source and one destination functions where the
 * arguments each have their own position. Permutes over test and
 * non-test variants
 */
#define __XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(NAME, OP, NOT, S1_OP,	\
					     S2_OP, NUM_WORD_BITS,	\
					     TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_GEN_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,	\
				     NUM_WORD_BITS, TYPE, SUFFIX,	\
				     CONV_FUNC,	const,, *,, false)	\
	__XDP2_BITMAP_3ARG_TEST_GEN_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,	\
					  NUM_WORD_BITS, TYPE, SUFFIX,	\
					  CONV_FUNC, const,, *,, false)	\
	__XDP2_BITMAP_3ARG_GEN_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,	\
				     NUM_WORD_BITS, TYPE, SUFFIX,	\
				     CONV_FUNC,, wsrc_,, &, true)	\
	__XDP2_BITMAP_3ARG_TEST_GEN_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,	\
					  NUM_WORD_BITS, TYPE, SUFFIX,	\
					  CONV_FUNC,, wsrc_,, &, true)

/* Permute macro for two source and one destination functions where the
 * arguments share a position. Permutes over test and non-test variants
 */
#define __XDP2_BITMAP_3ARG_PERMUTE_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,	\
					 NUM_WORD_BITS, TYPE, SUFFIX,	\
					 CONV_FUNC)			\
	__XDP2_BITMAP_3ARG_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,		\
				 NUM_WORD_BITS, TYPE, SUFFIX,		\
				 CONV_FUNC, const,, *,, false)		\
	__XDP2_BITMAP_3ARG_TEST_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,	\
				      NUM_WORD_BITS, TYPE, SUFFIX,	\
				      CONV_FUNC, const,, *,, false)	\
	__XDP2_BITMAP_3ARG_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,		\
				 NUM_WORD_BITS, TYPE, SUFFIX,		\
				 CONV_FUNC,, wsrc_,, &, true)		\
	__XDP2_BITMAP_3ARG_TEST_FUNCT(NAME, OP, NOT, S1_OP, S2_OP,	\
				      NUM_WORD_BITS, TYPE, SUFFIX,	\
				      CONV_FUNC,, wsrc_,, &, true)

#endif /* __XDP2_BITMAP_HELPERS_H__ */
