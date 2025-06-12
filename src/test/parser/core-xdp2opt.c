// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2020, 2021 by Mojatatu Networks.
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

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include "test-parser-core.h"

#include "xdp2/parser_metadata.h"
#include "xdp2/parsers/parser_big.h"
#include <time.h>

int xdp2_parser_big_ether_xdp2_parse_ether_node(
		const struct xdp2_parser *parser,
		const struct xdp2_parse_node *parse_node, const void *hdr,
		size_t len, struct xdp2_metadata *metadata,
		unsigned int flags, unsigned int max_encaps);

struct xdp2_priv {
	struct xdp2_parser_big_metadata_one md;
};

static void core_xdp2opt_help(void)
{
	fprintf(stderr,
		"For the `xdp2opt' core, arguments must be either not given or "
		"zero length.\n\n"
		"This core uses the compiler tool to optimize xdp2 \"Big parser\" "
		"engine for the XDP2 Parser.\n");
}

static void *core_xdp2opt_init(const char *args)
{
	struct xdp2_priv *p;

	if (args && *args) {
		fprintf(stderr, "The xdp2 core takes no arguments.\n");
		exit(-1);
	}

	p = calloc(1, sizeof(struct xdp2_priv));
	if (!p || xdp2_parser_init() < 0) {
		fprintf(stderr, "xdp2_parser_init failed\n");
		exit(-11);
	}

	return p;
}

static const char *core_xdp2opt_process(void *pv, void *data, size_t len,
				      struct test_parser_out *out,
				      unsigned int flags, long long *time)
{
	struct xdp2_priv *p = pv;
	int i, err;

	memset(&p->md, 0, sizeof(p->md));
	memset(out, 0, sizeof(*out));

	err = (int)XDP2_OKAY;

	if (!(flags & CORE_F_NOCORE)) {
		struct timespec begin_tp, now_tp;

		clock_gettime(CLOCK_MONOTONIC, &begin_tp);
		err = xdp2_parse(xdp2_parser_big_ether_opt, data, len,
				  &p->md.xdp2_data, 0,
				  XDP2_PARSER_BIG_ENCAP_DEPTH);
		clock_gettime(CLOCK_MONOTONIC, &now_tp);
		*time += (now_tp.tv_sec - begin_tp.tv_sec) * 1000000000 +
					(now_tp.tv_nsec - begin_tp.tv_nsec);
	}

	switch (err) {
	case XDP2_OKAY:
		// printf("XDP2 status OKAY\n");
		break;
	case XDP2_STOP_OKAY:
		// printf("XDP2 status STOP_OKAY\n");
		break;
	case XDP2_STOP_FAIL:
		return "XDP2: parse failed";
	case XDP2_STOP_LENGTH:
		return "XDP2: STOP_LENGTH";
	case XDP2_STOP_UNKNOWN_PROTO:
		return "XDP2: STOP_UNKNOWN_PROTO";
	case XDP2_STOP_ENCAP_DEPTH:
		return "XDP2: STOP_ENCAP_DEPTH";
	}
#if 0
	if (p->md.xdp2_data.encaps)
		printf("XDP2 encaps %u\n",
		       (unsigned int)p->md.xdp2_data.encaps);
	if (p->md.xdp2_data.max_frame_num)
		printf("XDP2 max_frame_num %u\n",
		       (unsigned int)p->md.xdp2_data.max_frame_num);
	if (p->md.xdp2_data.frame_size)
		printf("XDP2 frame_size %u\n",
		      (unsigned int)p->md.xdp2_data.frame_size);
#endif

	switch (p->md.frame.addr_type) {
	case 0:
		break;
	case XDP2_ADDR_TYPE_IPV4:
		out->k_control.addr_type = ADDR_TYPE_IPv4;
		break;
	case XDP2_ADDR_TYPE_IPV6:
		out->k_control.addr_type = ADDR_TYPE_IPv6;
		break;
	case XDP2_ADDR_TYPE_TIPC:
		out->k_control.addr_type = ADDR_TYPE_TIPC;
		break;
	default:
		out->k_control.addr_type = ADDR_TYPE_OTHER;
		break;
	}

	/* The out struct has no represantation for fragments. We need
	 * to add it. For now coment out the printing because it seems
	 * to confuse AFL
	 */
#if 0
	if (p->md.frame.is_fragment)
		printf("XDP2 is_fragment %d\n", (int)p->md.frame.is_fragment);
	if (p->md.frame.first_frag)
		printf("XDP2 first_frag %d\n", (int)p->md.frame.first_frag);

	if (p->md.frame.vlan_count)
		printf("XDP2 vlan_count %d\n", (int)p->md.frame.vlan_count);
#endif

	if (ARRAY_SIZE(p->md.frame.eth_addrs) !=
	    ARRAY_SIZE(out->k_eth_addrs.src) +
	    ARRAY_SIZE(out->k_eth_addrs.dst)) {
		fprintf(stderr, "XDP2 and output struct disagree on Ethernet "
			"address size\n");
		exit(-1);
	}

	memcpy(out->k_eth_addrs.dst, p->md.frame.eth_addrs,
	       ARRAY_SIZE(out->k_eth_addrs.dst));
	memcpy(out->k_eth_addrs.src,
	       &p->md.frame.eth_addrs[ARRAY_SIZE(out->k_eth_addrs.dst)],
	       ARRAY_SIZE(out->k_eth_addrs.src));

	out->k_mpls.mpls_ttl = p->md.frame.mpls.ttl;
	out->k_mpls.mpls_bos = p->md.frame.mpls.bos;
	out->k_mpls.mpls_tc = p->md.frame.mpls.tc;
	out->k_mpls.mpls_label = p->md.frame.mpls.label;
	out->k_arp.s_ip = p->md.frame.arp.sip;
	out->k_arp.t_ip = p->md.frame.arp.tip;
	out->k_arp.op = p->md.frame.arp.op;

	out->k_gre.flags = p->md.frame.gre.flags;
	out->k_gre.csum = p->md.frame.gre.csum;
	out->k_gre.keyid = p->md.frame.gre.keyid;
	out->k_gre.seq = p->md.frame.gre.seq;
	out->k_gre.routing = p->md.frame.gre.routing;

	out->k_gre_pptp.flags = p->md.frame.gre_pptp.flags;
	out->k_gre_pptp.length = p->md.frame.gre_pptp.length;
	out->k_gre_pptp.callid = p->md.frame.gre_pptp.callid;
	out->k_gre_pptp.seq = p->md.frame.gre_pptp.seq;
	out->k_gre_pptp.ack = p->md.frame.gre_pptp.ack;

	memcpy(out->k_arp.s_hw, p->md.frame.arp.sha,
	       xdp2_min(ARRAY_SIZE(p->md.frame.arp.sha),
			 ARRAY_SIZE(out->k_arp.s_hw)));
	memcpy(out->k_arp.t_hw, p->md.frame.arp.tha,
	       xdp2_min(ARRAY_SIZE(p->md.frame.arp.tha),
			 ARRAY_SIZE(out->k_arp.t_hw)));

	out->k_tcp_opt.mss = p->md.frame.tcp_options.mss;
	out->k_tcp_opt.ws = p->md.frame.tcp_options.window_scaling;
	out->k_tcp_opt.ts_val = p->md.frame.tcp_options.timestamp.value;
	out->k_tcp_opt.ts_echo = p->md.frame.tcp_options.timestamp.echo;

	/* We assume that the first SACK element with both edges zero
	 * indicates the end of the SACK list.
	 */
	for (i = 0; (i < ARRAY_SIZE(p->md.frame.tcp_options.sack)) &&
	     (i < ARRAY_SIZE(out->k_tcp_opt.sack)) &&
	     (p->md.frame.tcp_options.sack[i].left_edge ||
	      p->md.frame.tcp_options.sack[i].right_edge); i++) {
		out->k_tcp_opt.sack[i].l =
		    p->md.frame.tcp_options.sack[i].left_edge;
		out->k_tcp_opt.sack[i].r =
		    p->md.frame.tcp_options.sack[i].right_edge;
	}

	if (i < ARRAY_SIZE(out->k_tcp_opt.sack)) {
		out->k_tcp_opt.sack[i].l = 0;
		out->k_tcp_opt.sack[i].r = 0;
	}
	out->k_basic.n_proto = p->md.frame.eth_proto;
	out->k_basic.ip_proto = p->md.frame.ip_proto;
	out->k_flow_label.flow_label = p->md.frame.flow_label;

	switch (p->md.frame.vlan_count) {
	case 0:
		break;
	case 1:
		out->k_vlan.vlan_id = p->md.frame.vlan[0].id;
		out->k_vlan.vlan_dei = p->md.frame.vlan[0].dei;
		out->k_vlan.vlan_priority = p->md.frame.vlan[0].priority;
		out->k_vlan.vlan_tpid = p->md.frame.vlan[0].tpid;
		break;
	default:
#if 0
		printf("XDP2 vlan_count %d\n", (int)p->md.frame.vlan_count);
#endif
		break;
	}

#if 0
	if (p->md.frame.keyid)
		printf("XDP2 keyid %08lx\n", (unsigned long)p->md.frame.keyid);
#endif

	out->k_ports.src = p->md.frame.src_port;
	out->k_ports.dst = p->md.frame.dst_port;
	out->k_icmp.type = p->md.frame.icmp.type;
	out->k_icmp.code = p->md.frame.icmp.code;
	out->k_icmp.id = p->md.frame.icmp.id;

	switch (p->md.frame.addr_type) {
	case XDP2_ADDR_TYPE_IPV4:
		out->k_ipv4_addrs.src = p->md.frame.addrs.v4_addrs[0];
		out->k_ipv4_addrs.dst = p->md.frame.addrs.v4_addrs[1];
		break;
	case XDP2_ADDR_TYPE_IPV6:
		memcpy(out->k_ipv6_addrs.src, p->md.frame.addrs.v6_addrs, 16);
		memcpy(out->k_ipv6_addrs.dst, &p->md.frame.addrs.v6_addrs[1],
		       16);
		break;
	case XDP2_ADDR_TYPE_TIPC:
		out->k_tipc.key = p->md.frame.addrs.tipckey;
		break;
	}

	if (flags & CORE_F_HASH)
		out->k_hash.hash = xdp2_parser_big_hash_frame(&p->md.frame);

	return 0;
}

static void core_xdp2opt_done(void *pv)
{
	free(pv);
}

CORE_DECL(xdp2opt)
