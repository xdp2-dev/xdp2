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

#ifndef __SUNH_PARSER_TEST_H__
#define __SUNH_PARSER_TEST_H__

/* Defines Scale Up Network Header (SUNH) EtheRnet protocol parser nodes for
 * common parser test framework
 */

#include "sunh/sunh.h"

#define XDP2_DEFINE_PARSE_NODE
#include "sunh/proto_sunh.h"
#undef XDP2_DEFINE_PARSE_NODE

#include "xdp2/parser_test_helpers.h"

/* Parser definitions in parse_dump for SUPERp */

static int handler_sunh(const void *hdr, size_t hdr_len, size_t hdr_off,
			void *metadata, void *frame,
			const struct xdp2_ctrl_data *ctrl)
{
	const struct sunh_hdr *sunh = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tSNUH\n");

	if (verbose < 10)
		return 0;

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSource address: %u (0x%x)\n",
			     ntohs(sunh->saddr), ntohs(sunh->saddr));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tDestination address: %u (0x%x)\n",
			    ntohs(sunh->daddr), ntohs(sunh->daddr));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tNext header: %u (0x%x)\n",
			    sunh->next_header, sunh->next_header);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tTraffic class: %u (0x%x)\n",
			     sunh->traffic_class, sunh->traffic_class);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t\tDiffServ: %u (0x%x), "
				   "ECN: %u (0x%x)\n",
			     sunh->diff_serv, sunh->diff_serv,
			     sunh->ecn, sunh->ecn);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tHop limit %u (0x%x)\n",
			     sunh->hop_limit, sunh->hop_limit);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tFlow label %u (0x%x)\n",
			     sunh_get_flow_label(sunh),
			     sunh_get_flow_label(sunh));

	return 0;
}

XDP2_MAKE_PARSE_NODE(sunh_node, sunh_parse, ip6_table,
		     (.ops.handler = handler_sunh));

#endif /* __SUNH_PARSER_TEST_H__ */
