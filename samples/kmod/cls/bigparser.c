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

/* XDP2 Big Parser
 *
 * Implement flow dissector in XDP2. A protocol parse graph is created and
 * metadata is extracted at various nodes.
 */

#include <linux/types.h>

/* Define protocol nodes that are used below */
#include "xdp2/parser_metadata.h"
#include "xdp2/proto_nodes_def.h"

/* Meta data functions for parser nodes. Use the canned templates
 * for common metadata
 */
XDP2_METADATA_TEMP_ether_off(ether_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv4(ipv4_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv6(ipv6_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ip_overlay(ip_overlay_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv6_eh(ipv6_eh_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv6_frag(ipv6_frag_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ports_off(ports_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_icmp(icmp_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_vlan_8021AD(e8021AD_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_vlan_8021Q(e8021Q_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_mpls(mpls_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_arp_rarp(arp_rarp_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_tipc(tipc_metadata, xdp2_metadata_all)

XDP2_METADATA_TEMP_tcp_option_mss(tcp_opt_mss_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_tcp_option_window_scaling(tcp_opt_window_scaling_metadata,
					      xdp2_metadata_all)
XDP2_METADATA_TEMP_tcp_option_timestamp(tcp_opt_timestamp_metadata,
					 xdp2_metadata_all)

XDP2_METADATA_TEMP_tcp_option_sack_1(tcp_opt_sack_metadata_1,
				      xdp2_metadata_all)
XDP2_METADATA_TEMP_tcp_option_sack_2(tcp_opt_sack_metadata_2,
				      xdp2_metadata_all)
XDP2_METADATA_TEMP_tcp_option_sack_3(tcp_opt_sack_metadata_3,
				      xdp2_metadata_all)
XDP2_METADATA_TEMP_tcp_option_sack_4(tcp_opt_sack_metadata_4,
				      xdp2_metadata_all)

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

XDP2_MAKE_PARSE_NODE(ether_node, xdp2_parse_ether, ether_metadata,
		      NULL, ether_table);
XDP2_MAKE_PARSE_NODE(ip_overlay_node, xdp2_parse_ip, ip_overlay_metadata,
		      NULL, ip_table);
XDP2_MAKE_PARSE_NODE(ipv4_check_node, xdp2_parse_ipv4_check, ipv4_metadata,
		      NULL, ipv4_table);
XDP2_MAKE_PARSE_NODE(ipv4_node, xdp2_parse_ipv4, ipv4_metadata, NULL,
		      ipv4_table);
XDP2_MAKE_PARSE_NODE(ipv6_node, xdp2_parse_ipv6, ipv6_metadata, NULL,
		      ipv6_table);
XDP2_MAKE_PARSE_NODE(ipv6_check_node, xdp2_parse_ipv6_check, ipv6_metadata,
		      NULL, ipv6_table);
XDP2_MAKE_PARSE_NODE(ipv6_eh_node, xdp2_parse_ipv6_eh, ipv6_eh_metadata,
		      NULL, ipv6_table);
XDP2_MAKE_PARSE_NODE(ipv6_frag_node, xdp2_parse_ipv6_frag_eh,
		      ipv6_frag_metadata, NULL, ipv6_table);
XDP2_MAKE_PARSE_NODE(ppp_node, xdp2_parse_ppp, NULL, NULL, ppp_table);
XDP2_MAKE_PARSE_NODE(pppoe_node, xdp2_parse_pppoe, NULL, NULL,
		      pppoe_table);
XDP2_MAKE_PARSE_NODE(gre_base_node, xdp2_parse_gre_base, NULL, NULL,
		      gre_base_table);

XDP2_MAKE_FLAG_FIELDS_PARSE_NODE(gre_v0_node, xdp2_parse_gre_v0,
				  gre_metadata, NULL, gre_v0_table,
				  gre_v0_flag_fields_table);
XDP2_MAKE_FLAG_FIELDS_OVERLAY_PARSE_NODE(gre_v1_node, xdp2_parse_gre_v1,
					  gre_pptp_metadata, NULL, &ppp_node,
					  gre_v1_flag_fields_table);

XDP2_MAKE_PARSE_NODE(e8021AD_node, xdp2_parse_vlan, e8021AD_metadata,
		      NULL, ether_table);
XDP2_MAKE_PARSE_NODE(e8021Q_node, xdp2_parse_vlan, e8021Q_metadata, NULL,
		      ether_table);
XDP2_MAKE_OVERLAY_PARSE_NODE(ipv4ip_node, xdp2_parse_ipv4ip, NULL, NULL,
			      &ipv4_node);
XDP2_MAKE_OVERLAY_PARSE_NODE(ipv6ip_node, xdp2_parse_ipv6ip, NULL, NULL,
			      &ipv6_node);

XDP2_MAKE_PARSE_NODE(batman_node, xdp2_parse_batman, NULL, NULL,
		      ether_table);

XDP2_MAKE_LEAF_PARSE_NODE(ports_node, xdp2_parse_ports, ports_metadata,
			   NULL);
XDP2_MAKE_LEAF_PARSE_NODE(icmpv4_node, xdp2_parse_icmpv4, icmp_metadata,
			   NULL);
XDP2_MAKE_LEAF_PARSE_NODE(icmpv6_node, xdp2_parse_icmpv6, icmp_metadata,
			   NULL);
XDP2_MAKE_LEAF_PARSE_NODE(mpls_node, xdp2_parse_mpls, mpls_metadata,
			   NULL);
XDP2_MAKE_LEAF_PARSE_NODE(arp_node, xdp2_parse_arp, arp_rarp_metadata,
			   NULL);
XDP2_MAKE_LEAF_PARSE_NODE(rarp_node, xdp2_parse_rarp, arp_rarp_metadata,
			   NULL);
XDP2_MAKE_LEAF_PARSE_NODE(tipc_node, xdp2_parse_tipc, tipc_metadata,
			   NULL);
XDP2_MAKE_LEAF_PARSE_NODE(fcoe_node, xdp2_parse_fcoe, NULL, NULL);
XDP2_MAKE_LEAF_PARSE_NODE(igmp_node, xdp2_parse_igmp, NULL, NULL);

XDP2_MAKE_LEAF_TLVS_PARSE_NODE(tcp_node, xdp2_parse_tcp_tlvs,	ports_metadata,
				NULL, tcp_tlv_table);

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_mss_node, xdp2_parse_tcp_option_mss,
			  tcp_opt_mss_metadata, NULL);
XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_window_scaling_node,
			  xdp2_parse_tcp_option_window_scaling,
			  tcp_opt_window_scaling_metadata, NULL);
XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_timestamp_node,
			  xdp2_parse_tcp_option_timestamp,
			  tcp_opt_timestamp_metadata, NULL);

XDP2_MAKE_TLV_OVERLAY_PARSE_NODE(tcp_opt_sack_node, NULL, NULL,
				  tcp_sack_tlv_table, NULL, XDP2_OKAY, NULL);
XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_1, xdp2_parse_tcp_option_sack_1,
			  tcp_opt_sack_metadata_1, NULL);
XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_2, xdp2_parse_tcp_option_sack_2,
			  tcp_opt_sack_metadata_2, NULL);
XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_3, xdp2_parse_tcp_option_sack_3,
			  tcp_opt_sack_metadata_3, NULL);
XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_4, xdp2_parse_tcp_option_sack_4,
			  tcp_opt_sack_metadata_4, NULL);

XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_flag_csum_node, gre_checksum_metadata,
				 NULL);
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_flag_key_node, gre_keyid_metadata, NULL);
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_flag_seq_node, gre_seq_metadata, NULL);

XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_pptp_flag_ack_node, gre_pptp_ack_metadata,
				 NULL);
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_pptp_flag_key_node, gre_pptp_key_metadata,
				 NULL);
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_pptp_flag_seq_node, gre_pptp_seq_metadata,
				 NULL);

/* Protocol tables */

XDP2_MAKE_PROTO_TABLE(ether_table,
	{ __cpu_to_be16(ETH_P_IP), &ipv4_check_node },
	{ __cpu_to_be16(ETH_P_IPV6), &ipv6_check_node },
	{ __cpu_to_be16(ETH_P_8021AD), &e8021AD_node },
	{ __cpu_to_be16(ETH_P_8021Q), &e8021Q_node },
	{ __cpu_to_be16(ETH_P_MPLS_UC), &mpls_node },
	{ __cpu_to_be16(ETH_P_MPLS_MC), &mpls_node },
	{ __cpu_to_be16(ETH_P_ARP), &arp_node },
	{ __cpu_to_be16(ETH_P_RARP), &rarp_node },
	{ __cpu_to_be16(ETH_P_TIPC), &tipc_node },
	{ __cpu_to_be16(ETH_P_BATMAN), &batman_node },
	{ __cpu_to_be16(ETH_P_FCOE), &fcoe_node },
	{ __cpu_to_be16(ETH_P_PPP_SES), &pppoe_node },
);

XDP2_MAKE_PROTO_TABLE(ipv4_table,
	{ IPPROTO_TCP, &tcp_node.parse_node },
	{ IPPROTO_UDP, &ports_node },
	{ IPPROTO_SCTP, &ports_node },
	{ IPPROTO_DCCP, &ports_node },
	{ IPPROTO_GRE, &gre_base_node },
	{ IPPROTO_ICMP, &icmpv4_node },
	{ IPPROTO_IGMP, &igmp_node },
	{ IPPROTO_MPLS, &mpls_node },
	{ IPPROTO_IPIP, &ipv4ip_node },
	{ IPPROTO_IPV6, &ipv6ip_node },
);

XDP2_MAKE_PROTO_TABLE(ipv6_table,
	{ IPPROTO_HOPOPTS, &ipv6_eh_node },
	{ IPPROTO_ROUTING, &ipv6_eh_node },
	{ IPPROTO_DSTOPTS, &ipv6_eh_node },
	{ IPPROTO_FRAGMENT, &ipv6_frag_node },
	{ IPPROTO_TCP, &tcp_node.parse_node },
	{ IPPROTO_UDP, &ports_node },
	{ IPPROTO_SCTP, &ports_node },
	{ IPPROTO_DCCP, &ports_node },
	{ IPPROTO_GRE, &gre_base_node },
	{ IPPROTO_ICMPV6, &icmpv6_node },
	{ IPPROTO_IGMP, &igmp_node },
	{ IPPROTO_MPLS, &mpls_node },
	{ IPPROTO_IPIP, &ipv4ip_node },
	{ IPPROTO_IPV6, &ipv6ip_node },
);

XDP2_MAKE_PROTO_TABLE(ip_table,
	{ 4, &ipv4_node },
	{ 6, &ipv6_node },
);

XDP2_MAKE_PROTO_TABLE(gre_base_table,
	{ 0, &gre_v0_node.parse_node },
	{ 1, &gre_v1_node.parse_node },
);

XDP2_MAKE_PROTO_TABLE(gre_v0_table,
	{ __cpu_to_be16(ETH_P_IP), &ipv4_check_node },
	{ __cpu_to_be16(ETH_P_IPV6), &ipv6_check_node },
	{ __cpu_to_be16(ETH_P_TEB), &ether_node },
);

XDP2_MAKE_PROTO_TABLE(ppp_table,
	{ __cpu_to_be16(PPP_IP), &ipv4_check_node },
	{ __cpu_to_be16(PPP_IPV6), &ipv6_check_node },
);

XDP2_MAKE_PROTO_TABLE(pppoe_table,
	{ __cpu_to_be16(PPP_IP), &ipv4_check_node },
	{ __cpu_to_be16(PPP_IPV6), &ipv6_check_node },
);

XDP2_MAKE_TLV_TABLE(tcp_tlv_table,
	{ TCPOPT_MSS, &tcp_opt_mss_node },
	{ TCPOPT_WINDOW, &tcp_opt_window_scaling_node },
	{ TCPOPT_TIMESTAMP, &tcp_opt_timestamp_node },
	{ TCPOPT_SACK, &tcp_opt_sack_node }
);

/* Keys are possible lengths of the TCP sack option */
XDP2_MAKE_TLV_TABLE(tcp_sack_tlv_table,
	{ 10, &tcp_opt_sack_1 },
	{ 18, &tcp_opt_sack_2 },
	{ 26, &tcp_opt_sack_3 },
	{ 34, &tcp_opt_sack_4 }
);

XDP2_MAKE_FLAG_FIELDS_TABLE(gre_v0_flag_fields_table,
	{ GRE_FLAGS_CSUM_IDX, &gre_flag_csum_node },
	{ GRE_FLAGS_KEY_IDX, &gre_flag_key_node },
	{ GRE_FLAGS_SEQ_IDX, &gre_flag_seq_node }
);

XDP2_MAKE_FLAG_FIELDS_TABLE(gre_v1_flag_fields_table,
	{ GRE_PPTP_FLAGS_CSUM_IDX, &XDP2_FLAG_NODE_NULL },
	{ GRE_PPTP_FLAGS_KEY_IDX, &gre_pptp_flag_key_node },
	{ GRE_PPTP_FLAGS_SEQ_IDX, &gre_pptp_flag_seq_node },
	{ GRE_PPTP_FLAGS_ACK_IDX, &gre_pptp_flag_ack_node }
);

/* Define parsers. Two of them: one for packets starting with an
 * Ethernet header, and one for packets starting with an IP header.
 */
XDP2_PARSER_EXT(xdp2_parser_big_ether, "XDP2 big parser for Ethernet",
		 &ether_node);
