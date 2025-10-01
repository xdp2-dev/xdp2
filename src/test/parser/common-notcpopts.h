/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2025 by Tom Herbert
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

#ifndef __PARSER_TEST_COMMON_NOTCPOPTS__
#define __PARSER_TEST_COMMON_NOTCPOPTS__

/* Common parse node definitions for XDP notcpopts parsers */

#include "xdp2/parser_metadata.h"
#include "xdp2/proto_defs_define.h"

/* Meta data functions for parser nodes. Use the canned templates
 * for common metadata
 */
XDP2_METADATA_TEMP_ether(ether_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv4(ipv4_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv6(ipv6_metadata, xdp2_metadata_all)
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
XDP2_MAKE_PARSE_NODE(ip_overlay_node, xdp2_parse_ip, ip_table, ());
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

#endif /* __PARSER_TEST_COMMON_NOTCPOPTS__ */
