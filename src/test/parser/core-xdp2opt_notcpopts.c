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

/* XDP2 Big Parser
 *
 * Build a variant of the XDP2 big parse where we inline the parser, via
 * XDP2_PARER, and in this variant don't process TCP options. The part about
 * not processing TCP options makes this parser useful for comparing
 * performance to flowdis and parselite that don't use options
 */

#include <arpa/inet.h>
#include <linux/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "siphash/siphash.h"

#include "xdp2/parser_metadata.h"
#include "xdp2/parsers/parser_big.h"
#include "xdp2/proto_defs_define.h"

#include "test-parser-core.h"

/* Meta data functions for parser nodes. Use the canned templates
 * for common metadata
 */
XDP2_METADATA_TEMP_ether(ether_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv4(ipv4_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv6(ipv6_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ip_overlay(ip_overlay_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv6_eh(ipv6_eh_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv6_frag(ipv6_frag_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ports(ports_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_icmp(icmp_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_vlan_8021AD(e8021AD_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_vlan_8021Q(e8021Q_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_mpls(mpls_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_arp_rarp(arp_rarp_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_tipc(tipc_metadata, xdp2_metadata_all)

XDP2_METADATA_TEMP_gre(gre_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_gre_pptp(gre_pptp_metadata, xdp2_metadata_all)

XDP2_METADATA_TEMP_gre_checksum(gre_checksum_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_gre_keyid(gre_keyid_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_gre_seq(gre_seq_metadata, xdp2_metadata_all)

XDP2_METADATA_TEMP_gre_pptp_key(gre_pptp_key_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_gre_pptp_seq(gre_pptp_seq_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_gre_pptp_ack(gre_pptp_ack_metadata, xdp2_metadata_all)

/* Parse nodes. Parse nodes are composed of the common XDP2 Parser protocol
 * nodes, metadata functions defined above, and protocol tables defined
 * below
 */
XDP2_MAKE_PARSE_NODE(ether_node, xdp2_parse_ether, ether_table,
		     (.ops.extract_metadata = ether_metadata));
XDP2_MAKE_PARSE_NODE(ipv4_check_node, xdp2_parse_ip, ipv4_check_table, ());
XDP2_MAKE_PARSE_NODE(ipv4_node, xdp2_parse_ipv4, ipv4_table,
		     (.ops.extract_metadata = ipv4_metadata));
XDP2_MAKE_PARSE_NODE(ipv6_check_node, xdp2_parse_ip, ipv6_check_table, ());
XDP2_MAKE_PARSE_NODE(ipv6_node, xdp2_parse_ipv6, ipv6_table,
		     (.ops.extract_metadata = ipv6_metadata));
XDP2_MAKE_PARSE_NODE(ip_overlay_node, xdp2_parse_ip, ip_table,
		     (.ops.extract_metadata = ip_overlay_metadata));
XDP2_MAKE_PARSE_NODE(ipv6_eh_node, xdp2_parse_ipv6_eh, ipv6_table,
		     (.ops.extract_metadata = ipv6_eh_metadata));
XDP2_MAKE_PARSE_NODE(ipv6_frag_node, xdp2_parse_ipv6_frag_eh, ipv6_table,
		     (.ops.extract_metadata = ipv6_frag_metadata));
XDP2_MAKE_PARSE_NODE(gre_base_node, xdp2_parse_gre_base, gre_base_table, ());

XDP2_MAKE_FLAG_FIELDS_PARSE_NODE(gre_v0_node, xdp2_parse_gre_v0,
				 gre_v0_table, gre_v0_flag_fields_table,
				 (.ops.extract_metadata = gre_metadata), ());
XDP2_MAKE_FLAG_FIELDS_PARSE_NODE(gre_v1_node, xdp2_parse_gre_v1,
				 gre_v1_table, gre_v1_flag_fields_table,
				 (.ops.extract_metadata = gre_pptp_metadata),
				 ());

XDP2_MAKE_PARSE_NODE(e8021AD_node, xdp2_parse_vlan, ether_table,
		     (.ops.extract_metadata = e8021AD_metadata));
XDP2_MAKE_PARSE_NODE(e8021Q_node, xdp2_parse_vlan, ether_table,
		     (.ops.extract_metadata = e8021Q_metadata));
XDP2_MAKE_PARSE_NODE(ppp_node, xdp2_parse_ppp, ppp_table, ());
XDP2_MAKE_PARSE_NODE(pppoe_node, xdp2_parse_pppoe, pppoe_table, ());
XDP2_MAKE_PARSE_NODE(ipv4ip_node, xdp2_parse_ipv4ip, ipv4ip_table, ());
XDP2_MAKE_PARSE_NODE(ipv6ip_node, xdp2_parse_ipv6ip, ipv6ip_table, ());
XDP2_MAKE_PARSE_NODE(batman_node, xdp2_parse_batman, ether_table, ());

XDP2_MAKE_LEAF_PARSE_NODE(ports_node, xdp2_parse_ports,
			  (.ops.extract_metadata = ports_metadata));
XDP2_MAKE_LEAF_PARSE_NODE(icmpv4_node, xdp2_parse_icmpv4,
			  (.ops.extract_metadata = icmp_metadata));
XDP2_MAKE_LEAF_PARSE_NODE(icmpv6_node, xdp2_parse_icmpv6,
			  (.ops.extract_metadata = icmp_metadata));
XDP2_MAKE_LEAF_PARSE_NODE(mpls_node, xdp2_parse_mpls,
			  (.ops.extract_metadata = mpls_metadata));
XDP2_MAKE_LEAF_PARSE_NODE(arp_node, xdp2_parse_arp,
			  (.ops.extract_metadata = arp_rarp_metadata));
XDP2_MAKE_LEAF_PARSE_NODE(rarp_node, xdp2_parse_rarp,
			  (.ops.extract_metadata = arp_rarp_metadata));
XDP2_MAKE_LEAF_PARSE_NODE(tipc_node, xdp2_parse_tipc,
			  (.ops.extract_metadata = tipc_metadata));
XDP2_MAKE_LEAF_PARSE_NODE(fcoe_node, xdp2_parse_fcoe, ());
XDP2_MAKE_LEAF_PARSE_NODE(igmp_node, xdp2_parse_igmp, ());

XDP2_MAKE_LEAF_PARSE_NODE(tcp_node, xdp2_parse_tcp_notlvs,
			  (.ops.extract_metadata = ports_metadata));

XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_flag_csum_node,
				(.ops.extract_metadata =
						gre_checksum_metadata));
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_flag_key_node,
				(.ops.extract_metadata =
						gre_keyid_metadata));
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_flag_seq_node,
				(.ops.extract_metadata =
						gre_seq_metadata));

XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_pptp_flag_ack_node,
				(.ops.extract_metadata =
						gre_pptp_ack_metadata));
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_pptp_flag_key_node,
				(.ops.extract_metadata =
						gre_pptp_key_metadata));
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_pptp_flag_seq_node,
				(.ops.extract_metadata =
						gre_pptp_seq_metadata));

/* Define parsers. Two of them: one for packets starting with an
 * Ethernet header, and one for packets starting with an IP header.
 */
XDP2_PARSER(my_xdp2_parser_big_ether, "XDP2 big parser for Ethernet",
	    ether_node, ());
XDP2_PARSER(my_xdp2_parser_big_ip, "XDP2 big parser for IP",
	    ip_overlay_node, ());

/* Protocol tables */

XDP2_MAKE_PROTO_TABLE(ether_table,
	( __cpu_to_be16(ETH_P_IP), ipv4_check_node ),
	( __cpu_to_be16(ETH_P_IPV6), ipv6_check_node ),
	( __cpu_to_be16(ETH_P_8021AD), e8021AD_node ),
	( __cpu_to_be16(ETH_P_8021Q), e8021Q_node ),
	( __cpu_to_be16(ETH_P_MPLS_UC), mpls_node ),
	( __cpu_to_be16(ETH_P_MPLS_MC), mpls_node ),
	( __cpu_to_be16(ETH_P_ARP), arp_node ),
	( __cpu_to_be16(ETH_P_RARP), rarp_node ),
	( __cpu_to_be16(ETH_P_TIPC), tipc_node ),
	( __cpu_to_be16(ETH_P_BATMAN), batman_node ),
	( __cpu_to_be16(ETH_P_FCOE), fcoe_node ),
	( __cpu_to_be16(ETH_P_PPP_SES), pppoe_node )
);

XDP2_MAKE_PROTO_TABLE(ipv4_check_table,
	( 4, ipv4_node )
);

XDP2_MAKE_PROTO_TABLE(ipv4_table,
	( IPPROTO_TCP, ports_node ),
	( IPPROTO_UDP, ports_node ),
	( IPPROTO_SCTP, ports_node ),
	( IPPROTO_DCCP, ports_node ),
	( IPPROTO_GRE, gre_base_node ),
	( IPPROTO_ICMP, icmpv4_node ),
	( IPPROTO_IGMP, igmp_node ),
	( IPPROTO_MPLS, mpls_node ),
	( IPPROTO_IPIP, ipv4ip_node ),
	( IPPROTO_IPV6, ipv6ip_node )
);

XDP2_MAKE_PROTO_TABLE(ipv6_check_table,
	( 6, ipv6_node )
);

XDP2_MAKE_PROTO_TABLE(ipv6_table,
	( IPPROTO_HOPOPTS, ipv6_eh_node ),
	( IPPROTO_ROUTING, ipv6_eh_node ),
	( IPPROTO_DSTOPTS, ipv6_eh_node ),
	( IPPROTO_FRAGMENT, ipv6_frag_node ),
	( IPPROTO_TCP, ports_node ),
	( IPPROTO_UDP, ports_node ),
	( IPPROTO_SCTP, ports_node ),
	( IPPROTO_DCCP, ports_node ),
	( IPPROTO_GRE, gre_base_node ),
	( IPPROTO_ICMPV6, icmpv6_node ),
	( IPPROTO_IGMP, igmp_node ),
	( IPPROTO_MPLS, mpls_node ),
	( IPPROTO_IPIP, ipv4ip_node ),
	( IPPROTO_IPV6, ipv6ip_node )
);

XDP2_MAKE_PROTO_TABLE(ip_table,
	( 4, ipv4_node ),
	( 6, ipv6_node )
);

XDP2_MAKE_PROTO_TABLE(ipv4ip_table,
	( 0, ipv4_node )
);

XDP2_MAKE_PROTO_TABLE(ipv6ip_table,
	( 0, ipv6_node )
);

XDP2_MAKE_PROTO_TABLE(gre_base_table,
	( 0, gre_v0_node ),
	( 1, gre_v1_node )
);

XDP2_MAKE_PROTO_TABLE(gre_v0_table,
	( __cpu_to_be16(ETH_P_IP), ipv4_check_node ),
	( __cpu_to_be16(ETH_P_IPV6), ipv6_check_node ),
	( __cpu_to_be16(ETH_P_TEB), ether_node )
);

XDP2_MAKE_PROTO_TABLE(gre_v1_table,
	( 0, ppp_node )
);

XDP2_MAKE_PROTO_TABLE(ppp_table,
	( __cpu_to_be16(PPP_IP), ipv4_check_node ),
	( __cpu_to_be16(PPP_IPV6), ipv6_check_node )
);

XDP2_MAKE_PROTO_TABLE(pppoe_table,
	( __cpu_to_be16(PPP_IP), ipv4_check_node ),
	( __cpu_to_be16(PPP_IPV6), ipv6_check_node )
);

XDP2_MAKE_FLAG_FIELDS_TABLE(gre_v0_flag_fields_table,
	( GRE_FLAGS_CSUM_IDX, gre_flag_csum_node ),
	( GRE_FLAGS_KEY_IDX, gre_flag_key_node ),
	( GRE_FLAGS_SEQ_IDX, gre_flag_seq_node )
);

XDP2_MAKE_FLAG_FIELDS_TABLE(gre_v1_flag_fields_table,
	( GRE_PPTP_FLAGS_KEY_IDX, gre_pptp_flag_key_node ),
	( GRE_PPTP_FLAGS_SEQ_IDX, gre_pptp_flag_seq_node ),
	( GRE_PPTP_FLAGS_ACK_IDX, gre_pptp_flag_ack_node )
);

struct xdp2_priv {
	struct xdp2_parser_big_metadata_one md;
};

static void core_xdp2opt_notcpopts_help(void)
{
	fprintf(stderr,
		"For the `xdp2opt_notcpopts' core, arguments must be either "
		"not given or zero length.\n\n"
		"This core uses the compiler tool to optimize a variant of "
		"the xdp2 \"Big parser\" engine for the XDP2 Parser.\n");
}

static void *core_xdp2opt_notcpopts_init(const char *args)
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

XDP2_PARSER_EXTERN(my_xdp2_parser_big_ether_opt);

static const char *core_xdp2opt_notcpopts_process(void *pv, void *data,
					size_t len,
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
		struct xdp2_packet_data pdata;
		unsigned int pflags = 0;

		if (flags & CORE_F_DEBUG)
			pflags |= XDP2_F_DEBUG;

		clock_gettime(CLOCK_MONOTONIC, &begin_tp);

		XDP2_SET_BASIC_PDATA(pdata, data, len);

		err = xdp2_parse(my_xdp2_parser_big_ether_opt, &pdata,
				  &p->md, pflags);
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

static void core_xdp2opt_notcpopts_done(void *pv)
{
	free(pv);
}

CORE_DECL(xdp2opt_notcpopts)
