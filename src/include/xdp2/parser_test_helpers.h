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

#ifndef __XDP2_PARSE_TEST_HELPERS_H__
#define __XDP2_PARSE_TEST_HELPERS_H__

#include "xdp2/utility.h"
#include "xdp2/parser.h"

extern int verbose;
extern bool use_colors;

#define XDP2_PTH_PRINTFC(SEQNO, ...) do {				\
	if (use_colors)							\
		XDP2_PRINT_COLOR_SEL(SEQNO, __VA_ARGS__);		\
	else								\
		XDP2_CLI_PRINT(NULL, __VA_ARGS__);			\
} while (0)

#define XDP2_PTH_LOC_PRINTFC(CTRL, ...) do {				\
	int _i;								\
									\
	for (_i = 0; _i < (CTRL)->var.tlv_levels; _i++)			\
		XDP2_PTH_PRINTFC((CTRL)->pkt.seqno, "\t\t");		\
	XDP2_PTH_PRINTFC((CTRL)->pkt.seqno, __VA_ARGS__);		\
} while (0)

#define XDP2_PTH_MAKE_SIMPLE_TCP_OPT_HANDLER(NAME, TEXT)		\
static int handler_##NAME(const void *hdr, size_t hdr_len,		\
			  size_t hdr_off, void *metadata, void *frame,	\
			  const struct xdp2_ctrl_data *ctrl)		\
{									\
	const __u8 *tcp_opt = hdr;					\
									\
	if (verbose >= 5)						\
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t"			\
			TEXT " option %u, length %u\n",			\
			tcp_opt[0], tcp_opt[1]);			\
									\
	return 0;							\
}

#define XDP2_PTH_MAKE_SIMPLE_HANDLER(NAME, TEXT)			\
static int handler_##NAME(const void *hdr, size_t hdr_len,		\
			  size_t hdr_off, void *metadata, void *frame,	\
			  const struct xdp2_ctrl_data *ctrl)		\
{									\
									\
	if (verbose >= 5)						\
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t" TEXT "\n");		\
									\
	return 0;							\
}

#define XDP2_PTH_MAKE_SIMPLE_FLAG_FIELD_HANDLER(NAME, TEXT)		\
static int handler_##NAME(const void *hdr, size_t hdr_len,		\
			  size_t hdr_off, void *metadata, void *frame,	\
			  const struct xdp2_ctrl_data *ctrl)		\
{									\
	if (verbose >= 5)						\
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t" TEXT "\n");		\
									\
	return 0;							\
}

#define XDP2_PTH_MAKE_SACK_HANDLER(NUM)					\
static int handler_tcp_sack_##NUM(const void *hdr, size_t hdr_len,	\
				  size_t hdr_off, void *metadata,	\
				  void *frame,				\
				  const struct xdp2_ctrl_data *ctrl)	\
{									\
	const __u8 *tcp_opt = hdr;					\
									\
	if (verbose >= 5)						\
		XDP2_PTH_LOC_PRINTFC(ctrl,				\
			"\t\tTCP SACK_" #NUM " option %u, length %u\n",	\
			tcp_opt[0], tcp_opt[1]);			\
									\
	return 0;							\
}

#define XDP2_PTH_MAKE_SIMPLE_EH_HANDLER(NAME, TEXT)			\
static int handler_##NAME(const void *hdr, size_t hdr_len,		\
			  size_t hdr_off, void *metadata, void *frame,	\
			  const struct xdp2_ctrl_data *ctrl)		\
{									\
	if (verbose >= 5)						\
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t" TEXT " length %u\n",	\
			    ipv6_optlen(				\
				(struct ipv6_opt_hdr *)hdr));		\
	return 0;							\
}

#endif /* __XDP2_PARSE_TEST_HELPERS_H__ */
