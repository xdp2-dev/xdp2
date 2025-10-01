/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) by Tom Herbert.
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

#ifndef __PARSER_TEST_COMMON_XDP2_H__
#define __PARSER_TEST_COMMON_XDP2_H__

/* Header file for common functions to run xdp2_parse */

#include <stdbool.h>

#include "xdp2/parsers/parser_big.h"

#include "test-parser-core.h"

struct xdp2_priv {
	struct xdp2_parser_big_metadata_one md;
};

const char *common_core_xdp2_process(struct xdp2_priv *p, void *data,
				     size_t len,
				     struct test_parser_out *out,
				     unsigned int flags, long long *time,
				     const struct xdp2_parser *parser,
				     bool use_fast);

#endif /* __PARSER_TEST_COMMON_XDP2_H__ */
