// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
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

/* Tool to create parameterized macros include file
 *
 * Run as 'pmacro_gen <num>' where <num> is the maximum number of arguments
 * for the parameterized macros
 */

#include <stdio.h>
#include <stdlib.h>

#define LAST_POS 72
#define TAB_STOP 8
#define DEFAULT_NUM_PARAMS 64
#define END_POS 63

/* Make tabs for indentation */
void make_tabs(int chars)
{
	int i;

	for (i = 0; i < (LAST_POS - chars + (TAB_STOP - 1)) / TAB_STOP;
	     i++)
		printf("\t");
}

/* Make indentation. This is some number of tabs followed by spaces */
static void make_indent(int indents)
{
	unsigned int tabs = indents / TAB_STOP;
	unsigned int spaces = indents - (tabs * TAB_STOP);
	int i;

	for (i = 0; i < tabs; i++)
		printf("\t");

	for (i = 0; i < spaces; i++)
		printf(" ");
}

/* Output one macro argument */
static int make_one_arg(int chars, int i, char *string, int args,
			int indents, int check)
{
	if (i != args && chars >= END_POS) {
		chars += printf(string, i);
		chars += printf(",");
		make_tabs(chars);
		printf("\\\n");
		make_indent(indents);
		chars = indents;
	} else if (i != check) {
		chars += printf(string, i);
		chars += printf(", ");
	} else {
		chars += printf(string, i);
	}
	return chars;
}

/* Make the argument list. A1, A2, A3, ... */
static int make_args(int args, int indents, int chars, char *string)
{
	int tabs = indents / TAB_STOP;
	int spaces = indents - (tabs * TAB_STOP);
	int i, j;

	for (i = 1; i <= args; i++)
		chars = make_one_arg(chars, i, string, args, indents, args);

	return chars;
}

/* Make the argument list in reverse. A64, A63, A62, ... */
static int make_rev_args(int args, int indents, int chars, char *string)
{
	unsigned int tabs = indents / TAB_STOP;
	unsigned int spaces = indents - (tabs * TAB_STOP);
	int i, j;

	for (i = args; i >= 0; i--)
		chars = make_one_arg(chars, i, string, args, indents, 0);

	return chars;
}

/* One apply invocation */
static int make_one_apply(int chars, int i, int check,
			  const char *format, const char *string2)
{
	int indents;

	chars += printf(format, i);
	indents = chars;
	chars += printf("%s", string2);
	chars = make_args(i, indents, chars, "A%u");
	chars += printf(")");
	make_tabs(chars);
	printf("\\\n\t");
	chars = TAB_STOP;
	chars += printf(format, i - 1);
	chars += printf("%s", string2);
	chars = make_args(i - 1, indents, chars, "A%u");
	if (chars >= 60 && i != check) {
		make_tabs(chars);
		printf("\\\n");
		make_indent(indents);
	}
}

/* Make apply invocations for plain macros */
static void make_pmacro_apply(int num)
{
	int i, chars, indents;

	printf("#define __XDP2_PMACRO_APPLY1(ACT, A1) ACT(A1)\n\n");

	for (i = 2; i <= num; i++) {
		chars = printf("#define ");
		make_one_apply(chars, i, i,
				"__XDP2_PMACRO_APPLY%u(", "ACT, ");
		printf(") ACT(A%u)\n\n", i);
	}
}

/* Make apply invocations for plain macros */
static void make_pmacro_lastarg(int num)
{
	int i, chars, indents;

	printf("#define __XDP2_PMACRO_LASTARG(A1) A1\n\n");

	for (i = 2; i <= num; i++) {
		chars = printf("#define __XDP2_PMACRO_LASTARG%u(", i);
		make_args(i, TAB_STOP, chars, "A%u");
#if 0
		make_one_apply(chars, i, i,
				"__XDP2_PMACRO_LASTARG%u(", "");
#endif
		printf(") A%u\n\n", i);
	}
}

/* Make apply invocations for macros with number argument */
static void make_pmacro_apply_num(int num)
{
	int i, chars, indents;

	printf("#define __XDP2_PMACRO_APPLY1_NARG(ACT, A1) ACT(0, A1)\n\n");

	for (i = 2; i <= num; i++) {
		chars = printf("#define ");
		chars = make_one_apply(chars, i, i,
				"__XDP2_PMACRO_APPLY%u_NARG(", "ACT, ");
		printf(") ACT(%u, A%u)\n\n", i - 1, i);
	}
}

/* Make apply invocations for macros with user argument */
static void make_pmacro_apply_arg(int num)
{
	int i, chars, indents;

	printf("#define __XDP2_PMACRO_APPLY1_CARG(ACT, ARG, A1) "
	       "ACT(ARG, A1)\n\n");

	for (i = 2; i <= num; i++) {
		chars = printf("#define ");
		make_one_apply(chars, i, i,
			       "__XDP2_PMACRO_APPLY%u_CARG(", "ACT, ARG, ");
		printf(") ACT(ARG, A%u)\n\n", i);
	}
}

/* Make apply invocations for macros with user argument and number argument */
static void make_pmacro_apply_arg_num(int num)
{
	int i, chars, indents;

	printf("#define __XDP2_PMACRO_APPLY1_NARG_CARG(ACT, ARG, A1) "
	       "ACT(0, ARG, A1)\n\n");

	for (i = 2; i <= num; i++) {
		chars = printf("#define ");
		chars = make_one_apply(chars, i, i,
				"__XDP2_PMACRO_APPLY%u_NARG_CARG(",
				"ACT, ARG, ");
		printf(") ACT(%u, ARG, A%u)\n\n", i - 1, i);
	}
}

/* Make backend macro for counting number of arguments */
static void make_pmacro_nargs_x(int num)
{
	int chars, indents;

	chars = printf("#define __XDP2_PMACRO_NARGS_X(");
	indents = chars;
	chars += printf("dummy, ");
	chars = make_rev_args(num, indents, chars, "A%u");
	printf(", ...) A0\n\n");
}

/* Make frontend macro for counting number of arguments */
static void make_pmacro_nargs(int num)
{
	int chars, indents;

	chars = printf("#define XDP2_PMACRO_NARGS(...) "
			   "__XDP2_PMACRO_NARGS_X(dummy,");
	make_tabs(chars);
	printf("\\\n");

	printf("\t");
	chars = 8;
	indents = chars;

	chars += printf("##__VA_ARGS__, ");
	chars = make_rev_args(num, indents, chars, "%u");
	printf(")\n\n");
}

/* Run as 'pmacro_gen [<num>]' */
int main(int argc, char *argv[])
{
	int i, j, k, chars, indents, num = DEFAULT_NUM_PARAMS;

	if (argc > 1)
		num = strtol(argv[1], NULL, 10);

	printf("/* Generated file from \"pmacro_gen %d\"*/\n\n", num);

	make_pmacro_apply(num);
	make_pmacro_apply_arg(num);
	make_pmacro_apply_num(num);
	make_pmacro_apply_arg_num(num);
	make_pmacro_nargs_x(num);
	make_pmacro_nargs(num);
	make_pmacro_lastarg(num);

	exit(0);
}
