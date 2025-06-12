// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2020 Tom Herbert
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

/* Create array entries for bitmap functions to tested. Generates include
 * file bitmap_funcs.h which is included by test_bitmap.c
 */

#include <stdio.h>

#define __MAKE_FUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		  ARGMODE, FLAGS, SUFFIX, NUM_BIT_WORDS) do {		\
	printf("\t{ xdp2_bitmap" #SUFFIX "_" DESTSRC OP WEIGHT GEN ",");\
	printf(" \"" DESTSRC OP WEIGHT GEN #SUFFIX "\",");		\
	printf(" " OPTYPE ", " ARGMODE ", " FLAGS ", ");		\
	printf(#NUM_BIT_WORDS " },\n");					\
									\
	printf("\t{ xdp2_rbitmap" #SUFFIX "_"				\
	       DESTSRC OP WEIGHT GEN ",");				\
	printf(" \"rev_" DESTSRC OP WEIGHT GEN #SUFFIX "\",");		\
	printf(" " OPTYPE ", " ARGMODE ", " FLAGS " | FLAG_REV, ");	\
	printf(#NUM_BIT_WORDS " },\n");					\
} while (0)

#define __MAKE_WFUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		  ARGMODE, FLAGS, SUFFIX, NUM_BIT_WORDS) do {		\
	printf("\t{ xdp2_bitmap" #SUFFIX "_wsrc_"			\
					DESTSRC OP WEIGHT GEN ",");	\
	printf(" \"" DESTSRC OP WEIGHT GEN #SUFFIX "_wsrc\",");		\
	printf(" " OPTYPE ", " ARGMODE "W, " FLAGS);			\
	printf(" | FLAG_WORD_SRC, ");					\
	printf(#NUM_BIT_WORDS " },\n");					\
									\
	printf("\t{ xdp2_rbitmap" #SUFFIX "_wsrc_"			\
	       DESTSRC OP WEIGHT GEN ",");				\
	printf(" \"rev_" DESTSRC OP WEIGHT GEN #SUFFIX "_wsrc\",");	\
	printf(" " OPTYPE ", " ARGMODE "W, " FLAGS);			\
	printf(" | FLAG_WORD_SRC | FLAG_REV, ");			\
	printf(#NUM_BIT_WORDS " },\n");					\
} while (0)

#define MAKE_FUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		  ARGMODE, FLAGS) do {					\
	__MAKE_FUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,                   \
		    ARGMODE, FLAGS,, 0);				\
	__MAKE_FUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,                   \
		    ARGMODE, FLAGS, 8, 8);				\
	__MAKE_FUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,                   \
		    ARGMODE, FLAGS, 16, 16);				\
	__MAKE_FUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,                   \
		    ARGMODE, FLAGS, 32, 32);				\
	__MAKE_FUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,                   \
		    ARGMODE, FLAGS, 64, 64);				\
	__MAKE_FUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,                   \
		    ARGMODE, FLAGS "| FLAG_BSWAP", 16swp, 16);		\
	__MAKE_FUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,                   \
		    ARGMODE, FLAGS "| FLAG_BSWAP", 32swp, 32);		\
	__MAKE_FUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,                   \
		    ARGMODE, FLAGS "| FLAG_BSWAP", 64swp, 64);		\
} while (0)

#define MAKE_WFUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		   ARGMODE, FLAGS) do {					\
	__MAKE_WFUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		    ARGMODE, FLAGS,, 0);				\
	__MAKE_WFUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		    ARGMODE, FLAGS, 8, 8);				\
	__MAKE_WFUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		    ARGMODE, FLAGS, 16, 16);				\
	__MAKE_WFUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		    ARGMODE, FLAGS, 32, 32);				\
	__MAKE_WFUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		    ARGMODE, FLAGS, 64, 64);				\
	__MAKE_WFUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		    ARGMODE, FLAGS "| FLAG_BSWAP", 16swp, 16);		\
	__MAKE_WFUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		    ARGMODE, FLAGS "| FLAG_BSWAP", 32swp, 32);		\
	__MAKE_WFUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		    ARGMODE, FLAGS "| FLAG_BSWAP", 64swp, 64);		\
} while (0)

#define MAKE_XFUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		  ARGMODE, FLAGS) do {					\
	MAKE_FUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		   ARGMODE, FLAGS);					\
	MAKE_WFUNC(OP, OPTYPE, WEIGHT, DESTSRC, GEN,			\
		   ARGMODE, FLAGS);					\
} while (0)

#define PERMUTE_FOREACH_FUNC(NAME, ARGMODE, FLAGS)			\
	MAKE_FUNC(NAME, "TEST_XOR",,,, ARGMODE, FLAGS)

#define PERMUTE_OPS(WEIGHT, DESTSRC, GEN, ARGMODE, FLAGS) do {		\
	MAKE_XFUNC("and", "TEST_AND", WEIGHT, DESTSRC, GEN,		\
		  ARGMODE, FLAGS);					\
	MAKE_XFUNC("or", "TEST_OR", WEIGHT, DESTSRC, GEN,		\
		  ARGMODE, FLAGS);					\
	MAKE_XFUNC("xor", "TEST_XOR", WEIGHT, DESTSRC, GEN,		\
		  ARGMODE, FLAGS);					\
	MAKE_XFUNC("nand", "TEST_NAND", WEIGHT, DESTSRC, GEN,		\
		  ARGMODE, FLAGS);					\
	MAKE_XFUNC("nor", "TEST_NOR", WEIGHT, DESTSRC, GEN,		\
		  ARGMODE, FLAGS);					\
	MAKE_XFUNC("nxor", "TEST_NXOR", WEIGHT, DESTSRC, GEN,		\
		  ARGMODE, FLAGS);					\
	MAKE_XFUNC("and_not", "TEST_AND_NOT", WEIGHT, DESTSRC, GEN,	\
		  ARGMODE, FLAGS);					\
	MAKE_XFUNC("or_not", "TEST_OR_NOT", WEIGHT, DESTSRC, GEN,	\
		  ARGMODE, FLAGS);					\
	MAKE_XFUNC("xor_not", "TEST_XOR_NOT", WEIGHT, DESTSRC, GEN,	\
		  ARGMODE, FLAGS);					\
	MAKE_XFUNC("nand_not", "TEST_NAND_NOT", WEIGHT,			\
		  DESTSRC, GEN, ARGMODE, FLAGS);			\
	MAKE_XFUNC("nor_not", "TEST_NOR_NOT", WEIGHT, DESTSRC, GEN,	\
		  ARGMODE, FLAGS);					\
	MAKE_XFUNC("nxor_not", "TEST_NXOR_NOT", WEIGHT,			\
		  DESTSRC, GEN, ARGMODE, FLAGS);				\
} while (0)

#define PERMUTE_1SRC_OPS(WEIGHT, GEN, ARGMODE, FLAGS) do {		\
	MAKE_XFUNC("copy", "TEST_COPY", WEIGHT,, GEN,			\
		   ARGMODE, FLAGS);					\
	MAKE_XFUNC("not", "TEST_NOT", WEIGHT,, GEN,			\
		   ARGMODE, FLAGS);					\
} while (0)

#define PERMUTE_CMP_OPS(GEN, ARGMODE, FLAGS) do {			\
	MAKE_FUNC("cmp", "TEST_CMP",,, GEN, ARGMODE, FLAGS);		\
	MAKE_FUNC("first_and", "TEST_FIRST_AND",,, GEN, ARGMODE,	\
		   FLAGS);						\
	MAKE_FUNC("first_or", "TEST_FIRST_OR",,, GEN, ARGMODE, FLAGS);	\
	MAKE_FUNC("first_and_zero", "TEST_FIRST_AND_ZERO",,,		\
		  GEN, ARGMODE, FLAGS);					\
	MAKE_FUNC("first_or_zero", "TEST_FIRST_OR_ZERO",,, GEN,		\
		  ARGMODE, FLAGS);					\
} while (0)

#define PERMUTE_1ARG_CMP(ARGMODE, FLAGS) do {				\
	MAKE_FUNC("find", "TEST_FIRST_AND",,,, ARGMODE, FLAGS);		\
	MAKE_FUNC("find_zero", "TEST_FIRST_AND_ZERO",,,, ARGMODE,	\
		  FLAGS);						\
	MAKE_FUNC("find_next", "TEST_FIRST_AND",,,, ARGMODE,		\
		  FLAGS "|FLAG_FIND_NEXT");				\
	MAKE_FUNC("find_next_zero", "TEST_FIRST_AND_ZERO",,,,		\
		  ARGMODE, FLAGS "|FLAG_FIND_NEXT");			\
	MAKE_FUNC("find_roll", "TEST_FIRST_AND",,,, ARGMODE,		\
		  FLAGS "|FLAG_ROLL");					\
	MAKE_FUNC("find_roll_zero", "TEST_FIRST_AND_ZERO",,,, ARGMODE,	\
		  FLAGS "|FLAG_ROLL");					\
	MAKE_FUNC("find_roll_next", "TEST_FIRST_AND",,,, ARGMODE,	\
		  FLAGS "|FLAG_FIND_NEXT|FLAG_ROLL");			\
	MAKE_FUNC("find_roll_next_zero", "TEST_FIRST_AND_ZERO",,,,	\
		  ARGMODE, FLAGS "|FLAG_FIND_NEXT|FLAG_ROLL");		\
} while (0)

#define PERMUTE_1ARG_TEST(ARGMODE, FLAGS)				\
	MAKE_FUNC("test", "TEST_AND",,,, ARGMODE, FLAGS)

#define PERMUTE_1ARG_SET(ARGMODE, FLAGS) do {				\
	MAKE_FUNC("set_zeroes", "TEST_AND_NOT",,,, ARGMODE, FLAGS);	\
	MAKE_FUNC("set_ones", "TEST_OR_NOT",,,, ARGMODE, FLAGS);	\
} while (0)

#define PERMUTE_3ARG(WEIGHT, DESTSRC, GEN, FLAGS)			\
	PERMUTE_OPS(WEIGHT, DESTSRC, GEN, "ARGMODE_BBB", FLAGS)

#define PERMUTE_2ARG(WEIGHT, DESTSRC, GEN, FLAGS)			\
	PERMUTE_OPS(WEIGHT, DESTSRC, GEN, "ARGMODE_BB", FLAGS)

#define PERMUTE_1SRC(WEIGHT, GEN, FLAGS)				\
	PERMUTE_1SRC_OPS(WEIGHT, GEN, "ARGMODE_BB", FLAGS)

#define PERMUTE_CMP(GEN, FLAGS)						\
	PERMUTE_CMP_OPS(GEN, "ARGMODE_BB", FLAGS)

#define PERMUTE_SHROT(FLAGS) do {					\
	MAKE_FUNC("shift_left", "TEST_SHIFT_LEFT",,,,			\
						"ARGMODE_BB", FLAGS);	\
	MAKE_FUNC("shift_right", "TEST_SHIFT_RIGHT",,,,			\
						"ARGMODE_BB", FLAGS);	\
	MAKE_FUNC("shift_left", "TEST_SHIFT_LEFT",, "destsrc_",,	\
				"ARGMODE_B", "FLAG_DESTSRC|" FLAGS);	\
	MAKE_FUNC("shift_right", "TEST_SHIFT_RIGHT",, "destsrc_",,	\
				"ARGMODE_B", "FLAG_DESTSRC|" FLAGS);	\
	MAKE_FUNC("rotate_left", "TEST_ROTATE_LEFT",,,,			\
						"ARGMODE_BB", FLAGS);	\
	MAKE_FUNC("rotate_right", "TEST_ROTATE_RIGHT",,,,		\
						"ARGMODE_BB", FLAGS);	\
	MAKE_FUNC("rotate_left", "TEST_ROTATE_LEFT",, "destsrc_",,	\
				"ARGMODE_B", "FLAG_DESTSRC|" FLAGS);	\
	MAKE_FUNC("rotate_right", "TEST_ROTATE_RIGHT",, "destsrc_",,	\
				"ARGMODE_B", "FLAG_DESTSRC|" FLAGS);	\
} while (0)

#define PERMUTE_READ_WRITE() do {					\
	MAKE_FUNC("read8_test", "TEST_READ8",,,, "ARGMODE_BB",		\
		  "FLAG_READ_WRITE");					\
	MAKE_FUNC("read16_test", "TEST_READ16",,,, "ARGMODE_BB",	\
		  "FLAG_READ_WRITE");					\
	MAKE_FUNC("read32_test", "TEST_READ32",,,, "ARGMODE_BB",	\
		  "FLAG_READ_WRITE");					\
	MAKE_FUNC("read64_test", "TEST_READ64",,,, "ARGMODE_BB",	\
		  "FLAG_READ_WRITE");					\
	MAKE_FUNC("write8_test", "TEST_WRITE8",,,, "ARGMODE_BB",	\
		  "FLAG_READ_WRITE");					\
	MAKE_FUNC("write16_test", "TEST_WRITE16",,,, "ARGMODE_BB",	\
		  "FLAG_READ_WRITE");					\
	MAKE_FUNC("write32_test", "TEST_WRITE32",,,, "ARGMODE_BB",	\
		  "FLAG_READ_WRITE");					\
	MAKE_FUNC("write64_test", "TEST_WRITE64",,,, "ARGMODE_BB",	\
		  "FLAG_READ_WRITE");					\
} while (0)

int main(int argc, char *argv[])
{
	printf("/* Autogenerated file by make_bitmap_funcs */\n\n");

	PERMUTE_3ARG(,,, "0");
	PERMUTE_3ARG("_test",,, "FLAG_TEST");
	PERMUTE_3ARG(,, "_gen", "FLAG_GEN");
	PERMUTE_3ARG("_test",, "_gen", "FLAG_TEST|FLAG_GEN");

	PERMUTE_2ARG(, "destsrc_",, "FLAG_DESTSRC");
	PERMUTE_2ARG("_test", "destsrc_",,"FLAG_DESTSRC|FLAG_TEST");
	PERMUTE_2ARG(, "destsrc_", "_gen", "FLAG_DESTSRC|FLAG_GEN");
	PERMUTE_2ARG("_test", "destsrc_", "_gen",
		     "FLAG_DESTSRC|FLAG_TEST|FLAG_GEN");
	PERMUTE_2ARG(, "destsrc_",, "FLAG_DESTSRC");
	PERMUTE_2ARG("_test", "destsrc_",,"FLAG_DESTSRC|FLAG_TEST");

	PERMUTE_1SRC(,, "0");
	PERMUTE_1SRC("_test",, "FLAG_TEST");
	PERMUTE_1SRC(, "_gen", "FLAG_GEN");
	PERMUTE_1SRC("_test", "_gen", "FLAG_TEST|FLAG_GEN");

	PERMUTE_CMP(, "FLAG_CMP|FLAG_DESTSRC");
	PERMUTE_CMP("_gen", "FLAG_CMP|FLAG_DESTSRC|FLAG_GEN");

	PERMUTE_1ARG_TEST("ARGMODE_B", "FLAG_TEST");

	PERMUTE_1ARG_CMP("ARGMODE_B", "FLAG_CMP");

	PERMUTE_1ARG_SET("ARGMODE_B", "FLAG_SET");

	PERMUTE_FOREACH_FUNC("foreach_xor_test", "ARGMODE_BBB",
			     "FLAG_GEN");

	PERMUTE_SHROT("FLAG_SHROT");

	PERMUTE_READ_WRITE();
}
