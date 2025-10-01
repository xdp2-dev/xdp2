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

#include <getopt.h>
#include <linux/types.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "xdp2/vstruct.h"

/* Test for variable structures
 *
 * Run: ./vstructs
 */

struct my_test_struct {
	int a;
	char *b;
};

XDP2_VSTRUCT_VSMAP(test_struct1, void *,
	      (fifos1, struct my_test_struct, struct my_test_struct, 1, 0),
	      (fifos2, struct my_test_struct, struct my_test_struct, 2, 0),
	      (fifos3, struct my_test_struct, struct my_test_struct, 3, 0)
);

XDP2_VSTRUCT_VSCONST(test_struct2, void *,
	      (fifos1, struct my_test_struct, struct my_test_struct, 1, 0),
	      (fifos2, struct my_test_struct, struct my_test_struct, 2, 0),
	      (fifos3, struct my_test_struct, struct my_test_struct, 3, 0)
);

struct test_struct1 {
	struct test_struct1_vsmap vsmap;
	unsigned int a;
	unsigned int b;
};

struct test_struct2 {
	struct test_struct2_vsconst vsconst;
	unsigned int a;
	unsigned int b;
};

#define CHECK_OFFSET_VSMAP(STRUCT, FIELD) do {				\
	void *v;							\
									\
	v = (void *)XDP2_VSTRUCT_GETPTR_VSMAP(STRUCT, FIELD);		\
	XDP2_ASSERT((uintptr_t)v - (uintptr_t)STRUCT ==		\
		XDP2_VSTRUCT_GETOFF_VSMAP(STRUCT, FIELD),		\
		"Bad: %lu != %u", (uintptr_t)v - (uintptr_t)STRUCT,	\
				XDP2_VSTRUCT_GETOFF_VSMAP(STRUCT,	\
							   FIELD));	\
} while (0)

#define CHECK_OFFSET_VSCONST(STRUCT, FIELD) do {			\
	void *v;							\
									\
	v = (void *)XDP2_VSTRUCT_GETPTR_VSCONST(STRUCT, FIELD);	\
	XDP2_ASSERT((uintptr_t)v - (uintptr_t)STRUCT ==		\
		XDP2_VSTRUCT_GETOFF_VSCONST(STRUCT, FIELD),		\
		"Bad: %lu != %lu", (uintptr_t)v - (uintptr_t)STRUCT,	\
				XDP2_VSTRUCT_GETOFF_VSCONST(STRUCT,	\
							     FIELD));	\
} while (0)

static void run_test(void)
{
	struct test_struct1_def_vsmap def_vsmap1;
	struct test_struct2_def_vsmap def_vsmap2;
	struct test_struct1_vsmap vsmap1;
	struct test_struct2_vsmap vsmap2;
	struct test_struct1 *my_test1;
	struct test_struct2 my_test2;
	struct my_test_struct *pf;
	size_t size;

	size = test_struct1_instantiate_vsmap_from_config(
			NULL, &vsmap1, &def_vsmap1,
			sizeof(struct test_struct1));

	my_test1 = malloc(size);
	my_test1->vsmap = vsmap1;

	CHECK_OFFSET_VSMAP(my_test1, fifos1);
	CHECK_OFFSET_VSMAP(my_test1, fifos2);
	CHECK_OFFSET_VSMAP(my_test1, fifos3);

	pf = XDP2_VSTRUCT_GETPTR_VSMAP(my_test1, fifos3);

	printf("Vsmap: Start of structure: %p, size is %lu, pointer to "
	       "fifo3: %p, Offset to fifo3: %u\n", my_test1, size, pf,
	       XDP2_VSTRUCT_GETOFF_VSMAP(my_test1, fifos3));

	XDP2_VSTRUCT_PRINT_VSMAP(NULL, test_struct1, &vsmap1, &def_vsmap1,
			"\t", false, false, false, false);

	CHECK_OFFSET_VSCONST(&my_test2, fifos1);
	CHECK_OFFSET_VSCONST(&my_test2, fifos2);
	CHECK_OFFSET_VSCONST(&my_test2, fifos3);

	test_struct2_instantiate_vsmap_from_config(NULL, &vsmap2, &def_vsmap2,
						   sizeof(struct test_struct2));

	pf = XDP2_VSTRUCT_GETPTR_VSCONST(&my_test2, fifos3);

	printf("Vsconst: Start of structure: %p, size is %lu, pointer to "
	       "fifo3: %p, Offset to fifo3: %lu\n", &my_test2,
	       sizeof(my_test2), pf,
	       XDP2_VSTRUCT_GETOFF_VSCONST(&my_test2, fifos3));

	XDP2_VSTRUCT_PRINT_VSCONST(NULL, test_struct2, &vsmap2, &def_vsmap2,
			"\t", false, false, false, false);
}

int main(int argc, char *argv[])
{
	run_test();
}
