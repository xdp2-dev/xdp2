// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2020, 2021 by Mojatatu Networks.
 * Copyright (c) 2025, by Tom Herbert.
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

/* XDP2-tcpnopts core. Run tcpnopts parser via xdp2_parse */

#include "common-notcpopts.h"
#include "common-xdp2.h"
#include "test-parser-core.h"

XDP2_PARSER_STATIC(my_parser_big_ether, "XDP2 big parser for Ethernet",
		   ether_node,
		   (
		    .max_frames = XDP2_PARSER_BIG_NUM_FRAMES,
		    .metameta_size = 0,
		    .frame_size = sizeof(struct xdp2_parser_big_metadata_one)
		   )
);

static void core_xdp2_notcpopts_help(void)
{
	fprintf(stderr,
		"For the `xdp2_notcpopts' core, arguments must be either "
		"not given or zero length.\n\n"
		"This core uses the compiler tool to optimize a variant of "
		"the xdp2 \"Big parser\" engine for the XDP2 Parser.\n");
}

static void *core_xdp2_notcpopts_init(const char *args)
{
	struct xdp2_priv *p;

	if (args && *args) {
		fprintf(stderr, "The xdp2 core takes no arguments.\n");
		exit(-1);
	}

	p = calloc(1, sizeof(struct xdp2_priv));
	if (!p) {
		fprintf(stderr, "xdp2_parser_init failed\n");
		exit(-11);
	}

	return p;
}

static const char *core_xdp2_notcpopts_process(void *pv, void *data,
					size_t len,
					struct test_parser_out *out,
					unsigned int flags, long long *time)
{
	return common_core_xdp2_process((struct xdp2_priv *)pv, data, len,
					out, flags, time,
					my_parser_big_ether, false);
}

static void core_xdp2_notcpopts_done(void *pv)
{
	free(pv);
}

CORE_DECL(xdp2_notcpopts)
