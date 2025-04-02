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

#include <arpa/inet.h>
#include <linux/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "siphash/siphash.h"

#include "common.h"

/* Meta data functions for parser nodes. Use the canned templates
 * for common metadata
 */
XDP2_METADATA_TEMP_ether(ether_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv4(ipv4_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ports(ports_metadata, xdp2_metadata_all)
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

/* Parse nodes. Parse nodes are composed of the common XDP2 Parser protocol
 * nodes, metadata functions defined above, and protocol tables defined
 * below
 */

XDP2_MAKE_PARSE_NODE(ether_node, xdp2_parse_ether, ether_table,
		     (.ops.extract_metadata = ether_metadata));

XDP2_MAKE_PARSE_NODE(ipv4_check_node, xdp2_parse_ip, ipv4_check_table, ());
XDP2_MAKE_PARSE_NODE(ipv4_node, xdp2_parse_ipv4, ipv4_table,
		     (.ops.extract_metadata = ipv4_metadata));
XDP2_MAKE_LEAF_PARSE_NODE(ports_node, xdp2_parse_ports,
			  (.ops.extract_metadata = ports_metadata));

XDP2_MAKE_LEAF_TLVS_PARSE_NODE(tcp_node, xdp2_parse_tcp_tlvs, tcp_tlv_table,
			       (.ops.extract_metadata = ports_metadata), ());

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_mss_node,
			 xdp2_parse_tcp_option_mss,
			 (.tlv_ops.extract_metadata =
					tcp_opt_window_scaling_metadata));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_window_scaling_node,
			 xdp2_parse_tcp_option_window_scaling,
			 (.tlv_ops.extract_metadata =
					tcp_opt_window_scaling_metadata));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_timestamp_node,
			 xdp2_parse_tcp_option_timestamp,
			 (.tlv_ops.extract_metadata =
					tcp_opt_timestamp_metadata));

XDP2_MAKE_TLV_OVERLAY_PARSE_NODE(tcp_opt_sack_node, xdp2_parse_tlv_null,
				 tcp_sack_tlv_table,
				 (.unknown_overlay_ret = XDP2_STOP_OKAY));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_1, xdp2_parse_tcp_option_sack_1,
			 (.tlv_ops.extract_metadata =
					tcp_opt_sack_metadata_1));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_2, xdp2_parse_tcp_option_sack_2,
			 (.tlv_ops.extract_metadata =
					tcp_opt_sack_metadata_2));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_3, xdp2_parse_tcp_option_sack_3,
			 (.tlv_ops.extract_metadata =
					tcp_opt_sack_metadata_3));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_4, xdp2_parse_tcp_option_sack_4,
			 (.tlv_ops.extract_metadata =
					tcp_opt_sack_metadata_4));
/* Protocol tables */

XDP2_MAKE_PROTO_TABLE(ether_table,
	( __cpu_to_be16(ETH_P_IP), ipv4_check_node )
);

XDP2_MAKE_PROTO_TABLE(ipv4_check_table,
	( 4, ipv4_node )
);

XDP2_MAKE_PROTO_TABLE(ipv4_table,
	( IPPROTO_TCP, tcp_node ),
	( IPPROTO_UDP, ports_node )
);

XDP2_MAKE_TLV_TABLE(tcp_tlv_table,
	( TCPOPT_MSS, tcp_opt_mss_node ),
	( TCPOPT_WINDOW, tcp_opt_window_scaling_node ),
	( TCPOPT_TIMESTAMP, tcp_opt_timestamp_node ),
	( TCPOPT_SACK, tcp_opt_sack_node )
);

/* Keys are possible lengths of the TCP sack option */
XDP2_MAKE_TLV_TABLE(tcp_sack_tlv_table,
	( 10, tcp_opt_sack_1 ),
	( 18, tcp_opt_sack_2 ),
	( 26, tcp_opt_sack_3 ),
	( 34, tcp_opt_sack_4 )
);

XDP2_PARSER(xdp2_parser_simple_tuple, "XDP2 parser for 5 tuple TCP/UDP",
	    ether_node,
	    (.max_frames = 1,
	     .metameta_size = 0,
	     .frame_size = sizeof(struct xdp2_metadata_all)
	    )
);
