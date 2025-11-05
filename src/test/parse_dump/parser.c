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

/* Parse dump program */

#include <alloca.h>
#include <linux/seg6.h>
#include <netinet/ether.h>

#include "xdp2/packets_helpers.h"
#include "xdp2/parser.h"
#include "xdp2/parser_metadata.h"
#include "xdp2/proto_defs_define.h"
#include "xdp2/utility.h"

#include "falcon/parser_test.h"
#include "sue/parser_test.h"
#include "sunh/parser_test.h"
#include "superp/parser_test.h"
#include "uet/parser_test.h"

#include "parse_dump.h"

/* Extract EtherType from Ethernet header */
static void extract_ether(const void *hdr, size_t hdr_len, size_t hdr_off,
			  void *metadata, void *_frame,
			  const struct xdp2_ctrl_data *ctrl)
{
	struct metadata *frame = _frame;
	const struct ethhdr *eh = hdr;

	frame->ether_type = ntohs(eh->h_proto);
}

/* Extract IP protocol number and address from IPv4 header */
static void extract_ipv4(const void *hdr, size_t hdr_len, size_t hdr_off,
			 void *metadata, void *_frame,
			 const struct xdp2_ctrl_data *ctrl)
{
	struct metadata *frame = _frame;
	const struct iphdr *iph = hdr;

	ctrl->key.counters[0]++;
	ctrl->key.counters[1]++;

	frame->ip_proto = iph->protocol;
	frame->addr_type = XDP2_ADDR_TYPE_IPV4;
	frame->addrs.v4.saddr = iph->saddr;
	frame->addrs.v4.daddr = iph->daddr;
	frame->ip_ttl = iph->ttl;
	frame->ip_off = hdr_off;
	frame->ip_hlen = hdr_len;
	frame->ip_frag_off = iph->frag_off;
	frame->ip_frag_id = iph->id;
}

/* Extract protocol number and addresses for SUNH header */
static void extract_sunh(const void *hdr, size_t hdr_len, size_t hdr_off,
			 void *metadata, void *_frame,
			 const struct xdp2_ctrl_data *ctrl)
{
	struct metadata *frame = _frame;
	const struct sunh_hdr *sunh = hdr;

	frame->ip_proto = sunh->next_header;
	frame->addr_type = XDP2_ADDR_TYPE_SUNH;
	frame->addrs.sunh.saddr = sunh->saddr;
	frame->addrs.sunh.daddr = sunh->daddr;
	frame->ip_ttl = sunh->hop_limit;
}

/* Extract IP next header and address from IPv4 header */
static void extract_ipv6(const void *hdr, size_t hdr_len, size_t hdr_off,
			 void *metadata, void *_frame,
			 const struct xdp2_ctrl_data *ctrl)
{
	struct metadata *frame = _frame;
	const struct ipv6hdr *iph = hdr;

	frame->ip_proto = iph->nexthdr;
	frame->addr_type = XDP2_ADDR_TYPE_IPV6;
	frame->addrs.v6.saddr = iph->saddr;
	frame->addrs.v6.daddr = iph->daddr;
	frame->ip_off = hdr_off;
}

#if 0
/* Extract IP next header and address from IPv4 header */
static void extract_ipv6_fragment_header(const void *hdr, size_t hdr_len,
					 size_t hdr_off,
					 void *metadata, void *_frame,
					 const struct xdp2_ctrl_data *ctrl)
{
	struct metadata *frame = _frame;
	const struct ipv6_frag_hdr *fraghdr = hdr;

	frame->ip_frag_off = fraghdr->frag_off;
	frame->is_ipv6_frag = true;
	frame->ip_frag_id = fraghdr->identification;
}
#endif

/* Extract port number information from a transport header. Typically,
 * the sixteen bit source and destination port numbers are in the
 * first four bytes of a transport header that has ports (e.g. TCP, UDP,
 * etc.
 */
static void extract_ports(const void *hdr, size_t hdr_len, size_t hdr_off,
			  void *metadata, void *_frame,
			  const struct xdp2_ctrl_data *ctrl)
{
	struct metadata *frame = _frame;
	const __be16 *ports = hdr;

	frame->port_pair.sport = ports[0];
	frame->port_pair.dport = ports[1];
}

static void extract_udp(const void *hdr, size_t hdr_len, size_t hdr_off,
			void *metadata, void *_frame,
			const struct xdp2_ctrl_data *ctrl)
{
	extract_ports(hdr, hdr_len, hdr_off, metadata, _frame, ctrl);
}

static void extract_tcp(const void *hdr, size_t hdr_len, size_t hdr_off,
			void *metadata, void *_frame,
			const struct xdp2_ctrl_data *ctrl)
{
	struct pmetadata *pmeta = metadata;
	struct metametadata *metametadata = &pmeta->metametadata;

	metametadata->tcp_present = true;
	extract_ports(hdr, hdr_len, hdr_off, metadata, _frame, ctrl);
}

static void extract_icmpv4(const void *hdr, size_t hdr_len, size_t hdr_off,
			   void *metadata, void *_frame,
			   const struct xdp2_ctrl_data *ctrl)
{
	struct pmetadata *pmeta = metadata;
	struct metametadata *metametadata = &pmeta->metametadata;
	const struct icmphdr *icmp = hdr;

	metametadata->icmp.type = icmp->type;
	metametadata->icmp.code = icmp->code;
	metametadata->icmp.data = *(__u32 *)&icmp->un;
	metametadata->icmp_present = true;
}

static void extract_icmpv6(const void *hdr, size_t hdr_len, size_t hdr_off,
			   void *metadata, void *_frame,
			   const struct xdp2_ctrl_data *ctrl)
{
	struct pmetadata *pmeta = metadata;
	struct metametadata *metametadata = &pmeta->metametadata;
	const struct icmphdr *icmp = hdr;

	metametadata->icmp.type = icmp->type;
	metametadata->icmp.code = icmp->code;
	metametadata->icmp.data = *(__u32 *)&icmp->un;
	metametadata->icmp.is_ipv6 = true;
	metametadata->icmp_present = true;
}

static void extract_arp(const void *hdr, size_t hdr_len, size_t hdr_off,
			void *metadata, void *_frame,
			const struct xdp2_ctrl_data *ctrl)
{
	struct pmetadata *pmeta = metadata;
	struct metametadata *metametadata = &pmeta->metametadata;
	const struct earphdr *earp = hdr;

	metametadata->arp.op = ntohs(earp->arp.ar_op) & 0xff;

	/* Record Ethernet addresses */
	memcpy(metametadata->arp.sha, earp->ar_sha, ETH_ALEN);
	memcpy(metametadata->arp.tha, earp->ar_tha, ETH_ALEN);

	/* Record IP addresses */
	memcpy(&metametadata->arp.sip, &earp->ar_sip,
	       sizeof(metametadata->arp.sip));
	memcpy(&metametadata->arp.tip, &earp->ar_tip,
	       sizeof(metametadata->arp.tip));

	metametadata->arp_present = true;
}

/* Extract GRE version */
static void extract_gre_v0(const void *hdr, size_t hdr_len, size_t hdr_off,
			   void *metadata, void *_frame,
			   const struct xdp2_ctrl_data *ctrl)
{
	struct metadata *frame = _frame;

	frame->gre_version = 0x80;
}

/* Extract GRE version */
static void extract_gre_v1(const void *hdr, size_t hdr_len, size_t hdr_off,
			   void *metadata, void *_frame,
			   const struct xdp2_ctrl_data *ctrl)
{
	struct metadata *frame = _frame;

	frame->gre_version = 0x81;
}

/* Extract GRE checksum */
static void extract_gre_flag_csum(const void *hdr, size_t hdr_len,
				  size_t hdr_off, void *metadata, void *_frame,
				  const struct xdp2_ctrl_data *ctrl)
{
	// TODO
}

/* Extract GRE key */
static void extract_gre_flag_key(const void *hdr, size_t hdr_len,
				 size_t hdr_off, void *metadata, void *_frame,
				 const struct xdp2_ctrl_data *ctrl)
{
	struct metadata *frame = _frame;
	const __u32 *key = hdr;

	frame->gre.key_id.val = ntohl(*key);
}

/* Extract GRE sequence */
static void extract_gre_flag_seq(const void *hdr, size_t hdr_len,
				 size_t hdr_off, void *metadata, void *_frame,
				 const struct xdp2_ctrl_data *ctrl)
{
	struct metadata *frame = _frame;
	const __u32 *seq = hdr;

	frame->gre.sequence = ntohl(*seq);
}

/* Extract GRE ack */
static void extract_gre_flag_ack(const void *hdr, size_t hdr_len,
				 size_t hdr_off, void *metadata, void *_frame,
				 const struct xdp2_ctrl_data *ctrl)
{
	struct metadata *frame = _frame;
	const __u32 *ack = hdr;

	frame->gre.ack = ntohl(*ack);
}

/* Extract TCP timestamp option */
static void extract_tcp_timestamp(const void *hdr, size_t hdr_len,
				 size_t hdr_off, void *metadata, void *_frame,
				 const struct xdp2_ctrl_data *ctrl)
{
	struct pmetadata *pmeta = metadata;
	struct metametadata *metametadata = &pmeta->metametadata;
	const struct tcp_opt_union *opt = hdr;

	metametadata->tcp_options.timestamp.value =
				ntohl(opt->timestamp.value);
	metametadata->tcp_options.timestamp.echo =
				ntohl(opt->timestamp.echo);
}

/* Extract TCP timestamp option */
static void extract_tcp_mss(const void *hdr, size_t hdr_len,
			    size_t hdr_off, void *metadata, void *_frame,
			    const struct xdp2_ctrl_data *ctrl)
{
	struct pmetadata *pmeta = metadata;
	struct metametadata *metametadata = &pmeta->metametadata;
	const struct tcp_opt_union *opt = hdr;

	metametadata->tcp_options.mss = ntohs(opt->mss);
}

/* Extract TCP wscale option */
static void extract_tcp_wscale(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *metadata, void *_frame,
			       const struct xdp2_ctrl_data *ctrl)
{
	struct pmetadata *pmeta = metadata;
	struct metametadata *metametadata = &pmeta->metametadata;
	const struct tcp_opt_union *opt = hdr;

	metametadata->tcp_options.have_window_scaling = true;
	metametadata->tcp_options.window_scaling =
					ntohs(opt->window_scaling);
}

/* Extract TCP wscale option */
static void extract_tcp_sack_permitted(const void *hdr, size_t hdr_len,
				       size_t hdr_off, void *metadata,
				       void *_frame,
				       const struct xdp2_ctrl_data *ctrl)
{
}

static void extract_tcp_sack_1(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *metadata, void *_frame,
			       const struct xdp2_ctrl_data *ctrl)
{
	const struct tcp_opt_union *opt = hdr;
	struct metametadata *meta = metadata;

	meta->tcp_options.sack[0].left_edge = ntohl(opt->sack[0].left_edge);
	meta->tcp_options.sack[0].right_edge = ntohl(opt->sack[0].right_edge);

	meta->tcp_options.num_sacks = 1;
}

static void extract_tcp_sack_2(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *metadata, void *_frame,
			       const struct xdp2_ctrl_data *ctrl)
{
	const struct tcp_opt_union *opt = hdr;
	struct metametadata *meta = metadata;

	meta->tcp_options.sack[0].left_edge = ntohl(opt->sack[0].left_edge);
	meta->tcp_options.sack[0].right_edge = ntohl(opt->sack[0].right_edge);
	meta->tcp_options.sack[1].left_edge = ntohl(opt->sack[1].left_edge);
	meta->tcp_options.sack[1].right_edge = ntohl(opt->sack[1].right_edge);

	meta->tcp_options.num_sacks = 2;
}

static void extract_tcp_sack_3(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *metadata, void *_frame,
			       const struct xdp2_ctrl_data *ctrl)
{
	const struct tcp_opt_union *opt = hdr;
	struct metametadata *meta = metadata;

	meta->tcp_options.sack[0].left_edge = ntohl(opt->sack[0].left_edge);
	meta->tcp_options.sack[0].right_edge = ntohl(opt->sack[0].right_edge);
	meta->tcp_options.sack[1].left_edge = ntohl(opt->sack[1].left_edge);
	meta->tcp_options.sack[1].right_edge = ntohl(opt->sack[1].right_edge);
	meta->tcp_options.sack[2].left_edge = ntohl(opt->sack[2].left_edge);
	meta->tcp_options.sack[2].right_edge = ntohl(opt->sack[2].right_edge);

	meta->tcp_options.num_sacks = 3;
}

static void extract_tcp_sack_4(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *metadata, void *_frame,
			       const struct xdp2_ctrl_data *ctrl)
{
	const struct tcp_opt_union *opt = hdr;
	struct metametadata *meta = metadata;

	meta->tcp_options.sack[0].left_edge = ntohl(opt->sack[0].left_edge);
	meta->tcp_options.sack[0].right_edge = ntohl(opt->sack[0].right_edge);
	meta->tcp_options.sack[1].left_edge = ntohl(opt->sack[1].left_edge);
	meta->tcp_options.sack[1].right_edge = ntohl(opt->sack[1].right_edge);
	meta->tcp_options.sack[2].left_edge = ntohl(opt->sack[2].left_edge);
	meta->tcp_options.sack[2].right_edge = ntohl(opt->sack[2].right_edge);
	meta->tcp_options.sack[3].left_edge = ntohl(opt->sack[3].left_edge);
	meta->tcp_options.sack[3].right_edge = ntohl(opt->sack[3].right_edge);

	meta->tcp_options.num_sacks = 4;
}

static void extract_ipv4_check(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *metadata, void *_frame,
			       const struct xdp2_ctrl_data *ctrl)
{
	ctrl->key.keys[1] = ((struct ip_hdr_byte *)hdr)->version;
}

static void extract_ipv6_check(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *metadata, void *_frame,
			       const struct xdp2_ctrl_data *ctrl)
{
	ctrl->key.keys[1] = ((struct ip_hdr_byte *)hdr)->version;
}

static void extract_final(const void *hdr, size_t hdr_len,
			  size_t hdr_off, void *metadata, void *_frame,
			  const struct xdp2_ctrl_data *ctrl)
{
	struct metadata *frame = _frame;

	frame->vlan_cnt = ctrl->key.counters[2];
}

static void extract_vlan(const void *hdr, void *_frame,
			 const struct xdp2_ctrl_data *ctrl, __u16 tpid)
{
	struct metadata *frame = _frame;
	const struct vlan_hdr *vlan = hdr;
	int index = ctrl->key.counters[2] < XDP2_MAX_VLAN_CNT ?
		    ctrl->key.counters[2]++ : XDP2_MAX_VLAN_CNT - 1;

	frame->vlan[index].id = ntohs(vlan->h_vlan_TCI) &
				VLAN_VID_MASK;
	frame->vlan[index].priority = (ntohs(vlan->h_vlan_TCI) &
				VLAN_PRIO_MASK) >> VLAN_PRIO_SHIFT;
	frame->vlan[index].tpid = ntohs(tpid);
	frame->vlan[index].enc_proto =
				ntohs(vlan->h_vlan_encapsulated_proto);
}

static void extract_e8021AD(const void *hdr, size_t hdr_len,
			    size_t hdr_off, void *metadata, void *_frame,
			    const struct xdp2_ctrl_data *ctrl)
{
	extract_vlan(hdr, _frame, ctrl, ETH_P_8021AD);
}

static void extract_e8021Q(const void *hdr, size_t hdr_len,
			   size_t hdr_off, void *metadata, void *_frame,
			   const struct xdp2_ctrl_data *ctrl)
{
	extract_vlan(hdr, _frame, ctrl, ETH_P_8021AD);
}

/* Extract routing header */
static void extract_eh(const void *hdr, size_t hdr_len,
		       size_t hdr_off, void *metadata, void *_frame,
		       const struct xdp2_ctrl_data *ctrl)
{
	const struct ipv6_opt_hdr *opt = hdr;
	struct metadata *frame = _frame;

	frame->ip_proto = opt->nexthdr;
}

static int handler_ether_root(const void *hdr, size_t hdr_len,
			      size_t hdr_off, void *metadata, void *_frame,
			       const struct xdp2_ctrl_data *ctrl)
{
	struct one_packet *pdata = ctrl->key.arg;

	if (verbose >= 5) {
		/* Print banner for the packet instance */
		XDP2_PTH_LOC_PRINTFC(ctrl,
			"------------------------------------\n");
		if (pdata)
			XDP2_PTH_LOC_PRINTFC(ctrl, "File %s, packet #%u\n",
				    pdata->pcap_file, pdata->file_number);
		XDP2_PTH_LOC_PRINTFC(ctrl, "Nodes:\n");
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tEther node\n");
	}

	return 0;
}

static int handler_icmpv6_nd_target_addr_opt(const void *hdr, size_t hdr_len,
					     size_t hdr_off, void *metadata,
					     void *_frame,
					     const struct xdp2_ctrl_data *ctrl)
{
	const struct icmpv6_nd_opt *opt = hdr;

	if (verbose >= 5 && hdr_len == 8) {
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tTarget Link Local: %s\n",
				ether_ntoa((struct ether_addr *)opt->data));
	}

	return 0;
}

#if 0
static int handler_icmpv6_nd_source_addr_opt(const void *hdr, size_t hdr_len,
					     size_t hdr_off, void *metadata,
					     void *_frame,
					     const struct xdp2_ctrl_data *ctrl)
{
	const struct icmpv6_nd_opt *opt = hdr;

	if (ctrl.hdr_len == 8) {
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSource Link Local: %s\n",
				ether_ntoa((struct ether_addr *)opt->data));
	}

	return 0;
}
#endif

XDP2_MAKE_TLV_PARSE_NODE(icmpv6_nd_target_addr_opt_node, xdp2_parse_tlv_null,
			 (.tlv_ops.handler =
					handler_icmpv6_nd_target_addr_opt));

static int handler_ipv6_neigh_solicit(const void *hdr, size_t hdr_len,
				      size_t hdr_off, void *metadata,
				      void *_frame,
				      const struct xdp2_ctrl_data *ctrl)
{
	const struct icmpv6_nd_neigh_advert *ns = hdr;
	char sbuf[INET6_ADDRSTRLEN];

	if (verbose >= 5) {
		XDP2_PTH_LOC_PRINTFC(ctrl,
				     "\tICMPv6 neighbor solication node\n");

		inet_ntop(AF_INET6, &ns->target, sbuf, sizeof(sbuf));

		XDP2_PTH_LOC_PRINTFC(ctrl,
				     "\t\tTarget IPv6 address: %s\n", sbuf);
	}

	return 0;
}

static int handler_ipv6_neigh_advert(const void *hdr, size_t hdr_len,
				     size_t hdr_off, void *metadata,
				     void *_frame,
				     const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl,
				     "\tICMPv6 neighbor advertisement node\n");

	return 0;
}

static int handler_ospf(const void *hdr, size_t hdr_len, size_t hdr_off,
			void *metadata, void *_frame,
			const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tOSPF node\n");

	return 0;
}

XDP2_PTH_MAKE_SIMPLE_HANDLER(ether, "Ether node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(ip_overlay, "IP overlay node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(ip_overlay_by_key, "IP overlay node by key")
XDP2_PTH_MAKE_SIMPLE_HANDLER(ipv4, "IPv4 node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(ipv4_check, "IPv4 check node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(ipv6, "IPv6 node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(ipv6_check, "IPv6 check node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(ipv6ip, "IPv6inIPv4")
XDP2_PTH_MAKE_SIMPLE_HANDLER(l2tp_base, "L2TP Base")
XDP2_PTH_MAKE_SIMPLE_HANDLER(geneve_base, "Geneve Base")
XDP2_PTH_MAKE_SIMPLE_HANDLER(geneve_class_0, "Geneve Class 0 option")
XDP2_PTH_MAKE_SIMPLE_HANDLER(geneve_class_0_type_80,
			     "Geneve Class 0, type x80 option")
XDP2_PTH_MAKE_SIMPLE_HANDLER(geneve_v0, "Geneve version 0")
XDP2_PTH_MAKE_SIMPLE_HANDLER(l2tp_v0, "L2TP v0")
XDP2_PTH_MAKE_SIMPLE_HANDLER(l2tp_v1, "L2TP v1")
XDP2_PTH_MAKE_SIMPLE_HANDLER(l2tp_v2, "L2TP v2")
XDP2_PTH_MAKE_SIMPLE_HANDLER(l2tp_v3, "L2TP v3")
XDP2_PTH_MAKE_SIMPLE_HANDLER(ppp, "PPP node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(pppoe, "PPPoE node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(greov, "GRE node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(icmpv4, "ICMPv4 node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(vxlan, "VXLAN node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(icmpv6, "ICMPv6 node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(arp, "ARP node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(udp, "UDP node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(protobufs1, "Protobufs1 node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(protobufs2, "Protobufs2 node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(protobufs1_phones, "Protobufs1 phones")

XDP2_PTH_MAKE_SIMPLE_TCP_OPT_HANDLER(tcp_timestamp, "TCP Timestamp")
XDP2_PTH_MAKE_SIMPLE_TCP_OPT_HANDLER(tcp_mss, "TCP MSS")
XDP2_PTH_MAKE_SIMPLE_TCP_OPT_HANDLER(tcp_wscale, "TCP window scaling")
XDP2_PTH_MAKE_SIMPLE_TCP_OPT_HANDLER(tcp_sack_permitted, "TCP sack permitted")
XDP2_PTH_MAKE_SIMPLE_TCP_OPT_HANDLER(tcp_unknown, "Unknown TCP")

static int handler_protobufs1_name(const void *hdr, size_t hdr_len,
				   size_t hdr_off, void *metadata,
				   void *_frame,
				   const struct xdp2_ctrl_data *ctrl)
{
	const void *ptr = NULL;
	size_t len = 0;
	char *buffer;
	__u64 value;

	len = parse_varint(&value, hdr, 0);
	parse_string(hdr + len, &ptr, &len, 0);

	buffer = alloca(len + 10);
	memcpy(buffer, ptr, len);
	buffer[len] = '\0';

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "Protobuf name: %s\n", buffer);

	return 0;
}

static int handler_protobufs2_phone_number(const void *hdr, size_t hdr_len,
					   size_t hdr_off, void *metadata,
					   void *_frame,
					   const struct xdp2_ctrl_data *ctrl)
{
	const void *ptr = NULL;
	size_t len = 0;
	char *buffer;
	__u64 value;

	len = parse_varint(&value, hdr, 0);
	parse_string(hdr + len, &ptr, &len, 0);

	buffer = alloca(len + 10);
	memcpy(buffer, ptr, len);
	buffer[len] = '\0';

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl,
				     "Protobuf phone number: %s\n", buffer);

	return 0;
}

static int handler_protobufs2_phone_type(const void *hdr, size_t hdr_len,
					 size_t hdr_off, void *metadata,
					 void *_frame,
					 const struct xdp2_ctrl_data *ctrl)
{
	__u64 value;

	parse_varint(&value, hdr, 0);

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl,
				     "Protobuf phone type: %llu\n", value);

	return 0;
}

static int handler_protobufs1_email(const void *hdr, size_t hdr_len,
				    size_t hdr_off, void *metadata,
				    void *_frame,
				    const struct xdp2_ctrl_data *ctrl)
{
	const void *ptr = NULL;
	size_t len = 0;
	char *buffer;
	__u64 value;

	len = parse_varint(&value, hdr, 0);
	parse_string(hdr + len, &ptr, &len, 0);

	buffer = alloca(len + 10);
	memcpy(buffer, ptr, len);
	buffer[len] = '\0';

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "Protobuf email: %s\n", buffer);

	return 0;
}

static int handler_protobufs1_id(const void *hdr, size_t hdr_len,
				 size_t hdr_off, void *metadata,
				 void *_frame,
				 const struct xdp2_ctrl_data *ctrl)
{
	__u64 value;
	size_t len;

	len = parse_varint(&value, hdr, 0);
	parse_varint(&value, hdr + len, 0);

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "Protobuf id: %llu\n", value);

	return 0;
}

XDP2_PTH_MAKE_SACK_HANDLER(1)
XDP2_PTH_MAKE_SACK_HANDLER(2)
XDP2_PTH_MAKE_SACK_HANDLER(3)
XDP2_PTH_MAKE_SACK_HANDLER(4)

XDP2_PTH_MAKE_SIMPLE_HANDLER(tcp, "TCP node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(grev0, "GREv0 node")
XDP2_PTH_MAKE_SIMPLE_HANDLER(grev1, "GREv1 node")

static int handler_okay(const void *hdr, size_t hdr_len,
			size_t hdr_off, void *metadata, void *_frame,
			const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t** Okay node\n");

	if (verbose >= 5)
		print_metadata(metadata, ctrl);

	return 0;
}

static int handler_fail(const void *hdr, size_t hdr_len,
			size_t hdr_off, void *metadata, void *_frame,
			const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t** Fail node\n");

	if (verbose >= 5)
		print_metadata(metadata, ctrl);

	return 0;
}

static int handler_atencap(const void *hdr, size_t hdr_len,
			   size_t hdr_off, void *metadata, void *_frame,
			   const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t** At encapsulation node\n");

	ctrl->key.counters[1] = 0;
	ctrl->key.counters[2] = 0;

	return 0;
}

/* Parse nodes, See test_parser_threads_func.c for a description of the
 * dependencies used in the test
 */
XDP2_MAKE_PARSE_NODE(ether_node_root, xdp2_parse_ether, ether_table,
		     (.ops.extract_metadata = extract_ether,
		      .ops.handler = handler_ether_root));

XDP2_MAKE_PARSE_NODE(ether_node, xdp2_parse_ether, ether_table,
		     (.ops.extract_metadata = extract_ether,
		      .ops.handler = handler_ether));

XDP2_MAKE_PARSE_NODE(ip_overlay_node, xdp2_parse_ip, ip_overlay_table,
		     (.ops.handler = handler_ip_overlay));

XDP2_MAKE_PARSE_NODE(ip_overlay_by_key_node, xdp2_parse_ip_by_key,
		     ip_overlay_by_key_table,
		     (.ops.handler = handler_ip_overlay_by_key, .key_sel = 1));

XDP2_MAKE_PARSE_NODE(ipv4_node, xdp2_parse_ipv4, ip4_table,
		     (.ops.extract_metadata = extract_ipv4,
		      .ops.handler = handler_ipv4));

XDP2_MAKE_AUTONEXT_PARSE_NODE(ipv4_check_node, xdp2_parse_ipv4_check,
			      ip_overlay_by_key_node,
			      (.ops.extract_metadata = extract_ipv4_check,
			       .ops.handler = handler_ipv4_check));

XDP2_MAKE_AUTONEXT_PARSE_NODE(ipv4ip_node, xdp2_parse_ipv4ip, ipv4_node,
			      (.ops.handler = handler_ipv4_check));

XDP2_MAKE_PARSE_NODE(ipv6_node, xdp2_parse_ipv6, ip6_table,
		     (.ops.extract_metadata = extract_ipv6,
		      .ops.handler = handler_ipv6));

XDP2_MAKE_AUTONEXT_PARSE_NODE(ipv6_check_node, xdp2_parse_ipv6_check,
			      ip_overlay_by_key_node,
			      (.ops.extract_metadata = extract_ipv6_check,
			       .ops.handler = handler_ipv6_check));

XDP2_MAKE_AUTONEXT_PARSE_NODE(ipv6ip_node, xdp2_parse_ipv6ip, ipv6_node,
			      (.ops.handler = handler_ipv6ip));

XDP2_MAKE_PARSE_NODE(l2tp_base_node, xdp2_parse_l2tp_base, l2tp_base_table,
		     (.ops.handler = handler_l2tp_base));

XDP2_MAKE_PARSE_NODE(geneve_base_node, xdp2_parse_geneve_base,
		     geneve_base_table,
		     (.ops.handler = handler_geneve_base));

XDP2_MAKE_TLVS_PARSE_NODE(geneve_v0_node, xdp2_parse_geneve_v0,
			  ether_table, geneve_class_table,
			  (.ops.handler = handler_geneve_v0), ());

XDP2_MAKE_TLV_OVERLAY_PARSE_NODE(geneve_class_0_tlv_node,
				 xdp2_parse_geneve_tlv_overlay,
				 geneve_tlv_table,
				 (.tlv_ops.handler = handler_geneve_class_0));

XDP2_MAKE_TLV_PARSE_NODE(geneve_class_0_tlv_80_node,
			 xdp2_parse_tlv_null,
			 (.tlv_ops.handler = handler_geneve_class_0_type_80));

XDP2_MAKE_PARSE_NODE(ppp_node, xdp2_parse_ppp, ppp_table,
		     (.ops.handler = handler_ppp));

XDP2_MAKE_PARSE_NODE(pppoe_node, xdp2_parse_pppoe, ppp_table,
		     (.ops.handler = handler_pppoe));

XDP2_MAKE_LEAF_PARSE_NODE(icmpv4_node, xdp2_parse_icmpv4,
			  (.ops.extract_metadata = extract_icmpv4,
			   .ops.handler = handler_icmpv4,
			   .unknown_ret = XDP2_STOP_OKAY));

XDP2_MAKE_PARSE_NODE(icmpv6_node, xdp2_parse_icmpv6, icmpv6_table,
		     (.ops.extract_metadata = extract_icmpv6,
		      .ops.handler = handler_icmpv6,
		      .unknown_ret = XDP2_STOP_OKAY));

XDP2_PTH_MAKE_SIMPLE_TCP_OPT_HANDLER(icmpv6_nd_solicit_opt_unknown,
		"Unknown neighbor discovery solicitation option")

XDP2_MAKE_TLV_PARSE_NODE(icmpv6_nd_solicit_opt_unknown_node,
			 xdp2_parse_tlv_null,
			 (.tlv_ops.handler =
					handler_icmpv6_nd_solicit_opt_unknown));

XDP2_MAKE_TLV_TABLE(icmpv6_neigh_solicit_table,
	( 1, icmpv6_nd_target_addr_opt_node )
);

XDP2_MAKE_LEAF_TLVS_PARSE_NODE(icmpv6_neigh_solicit,
			  xdp2_parse_icmpv6_nd_solicit,
			  icmpv6_neigh_solicit_table,
			  (.ops.handler = handler_ipv6_neigh_solicit),
			  (.tlv_wildcard_node =
				XDP2_TLV_NODE(
					icmpv6_nd_solicit_opt_unknown_node))
			  );

XDP2_MAKE_LEAF_PARSE_NODE(icmpv6_neigh_advert, xdp2_parse_null,
			  (.ops.handler = handler_ipv6_neigh_advert));

XDP2_MAKE_AUTONEXT_PARSE_NODE(vxlan_node, xdp2_parse_vxlan, ether_node,
			      (.ops.handler = handler_vxlan));

XDP2_MAKE_LEAF_TLVS_PARSE_NODE(protobufs1_node, xdp2_parse_protobufs,
			       protobufs1_table,
			       (.ops.handler = handler_protobufs1), ());

XDP2_MAKE_TLV_PARSE_NODE(protobufs2_entry_node, xdp2_parse_nested_protobuf,
			 (.nested_node = XDP2_PARSE_NODE(protobufs1_node)));

XDP2_MAKE_LEAF_TLVS_PARSE_NODE(protobufs2_node, xdp2_parse_protobufs,
			       protobufs2_table,
			       (.ops.handler = handler_protobufs2), ());

XDP2_MAKE_LEAF_PARSE_NODE(ospf_node, xdp2_parse_null,
			  (.ops.handler = handler_ospf));

XDP2_MAKE_TLV_TABLE(protobufs2_table,
	(1, protobufs2_entry_node )
);

XDP2_MAKE_PARSE_NODE(udp_node, xdp2_parse_udp, udp_ports_table,
		     (.ops.extract_metadata = extract_udp,
		      .ops.handler = handler_udp,
		      .unknown_ret = XDP2_STOP_OKAY));

XDP2_MAKE_LEAF_PARSE_NODE(arp_node, xdp2_parse_arp,
			  (.ops.extract_metadata = extract_arp,
			   .ops.handler = handler_arp));

/* TCP TLV nodes */
XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_timestamp_node,
			 xdp2_parse_tcp_option_timestamp,
			 (.tlv_ops.extract_metadata = extract_tcp_timestamp,
			  .tlv_ops.handler = handler_tcp_timestamp));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_mss_node,
			 xdp2_parse_tcp_option_mss,
			 (.tlv_ops.extract_metadata = extract_tcp_mss,
			  .tlv_ops.handler = handler_tcp_mss));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_wscale_node,
			 xdp2_parse_tcp_option_window_scaling,
			 (.tlv_ops.extract_metadata = extract_tcp_wscale,
			  .tlv_ops.handler = handler_tcp_wscale));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_permitted_node,
			 xdp2_parse_tcp_option_sack_permitted,
			 (.tlv_ops.extract_metadata =
						extract_tcp_sack_permitted,
			 .tlv_ops.handler = handler_tcp_sack_permitted));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_unknown_node, xdp2_parse_tcp_option_unknown,
			 (.tlv_ops.handler = handler_tcp_unknown));

/* TCP TLV nodes */
XDP2_MAKE_TLV_PARSE_NODE(protobufs1_name_node,
			 xdp2_parse_tlv_null,
			 (.tlv_ops.handler = handler_protobufs1_name));

XDP2_MAKE_TLV_PARSE_NODE(protobufs1_id_node,
			 xdp2_parse_tlv_null,
			 (.tlv_ops.handler = handler_protobufs1_id));

XDP2_MAKE_TLV_PARSE_NODE(protobufs1_email_node,
			 xdp2_parse_tlv_null,
			 (.tlv_ops.handler = handler_protobufs1_email));

XDP2_MAKE_TLV_PARSE_NODE(protobufs2_phone_number_node,
			 xdp2_parse_tlv_null,
			 (.tlv_ops.handler = handler_protobufs2_phone_number));

XDP2_MAKE_TLV_PARSE_NODE(protobufs2_phone_type_node,
			 xdp2_parse_tlv_null,
			 (.tlv_ops.handler = handler_protobufs2_phone_type));

XDP2_MAKE_LEAF_TLVS_PARSE_NODE(protobufs1_phone_node, xdp2_parse_protobufs,
			       protobufs2_phone_table, (), ());

XDP2_MAKE_TLV_PARSE_NODE(protobufs1_phones_node,
			 xdp2_parse_nested_protobuf,
			 (.tlv_ops.handler = handler_protobufs1_phones,
			  .nested_node = &protobufs1_phone_node.pn));

XDP2_MAKE_TLV_OVERLAY_PARSE_NODE(tcp_opt_sack_node, xdp2_parse_tlv_null,
				 tcp_sack_tlv_table, ());

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_1, xdp2_parse_tcp_option_sack_1,
			 (.tlv_ops.extract_metadata = extract_tcp_sack_1,
			  .tlv_ops.handler = handler_tcp_sack_1));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_2, xdp2_parse_tcp_option_sack_2,
			 (.tlv_ops.extract_metadata = extract_tcp_sack_2,
			  .tlv_ops.handler = handler_tcp_sack_2));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_3, xdp2_parse_tcp_option_sack_3,
			 (.tlv_ops.extract_metadata = extract_tcp_sack_3,
			  .tlv_ops.handler = handler_tcp_sack_3));

XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_sack_4, xdp2_parse_tcp_option_sack_4,
			 (.tlv_ops.extract_metadata = extract_tcp_sack_4,
			  .tlv_ops.handler = handler_tcp_sack_4));

XDP2_MAKE_LEAF_TLVS_PARSE_NODE(tcp_node, xdp2_parse_tcp_tlvs, tcp_tlv_table,
			       (.ops.extract_metadata = extract_tcp,
				.ops.handler = handler_tcp),
			       (.tlv_wildcard_node = &tcp_opt_unknown_node));

XDP2_MAKE_LEAF_PARSE_NODE(okay_node, xdp2_parse_null,
			  (.ops.extract_metadata = extract_final,
			   .ops.handler = handler_okay));

XDP2_MAKE_LEAF_PARSE_NODE(fail_node, xdp2_parse_null,
			  (.ops.extract_metadata = extract_final,
			   .ops.handler = handler_fail));

XDP2_MAKE_LEAF_PARSE_NODE(atencap_node, xdp2_parse_null,
			  (.ops.extract_metadata = extract_final,
			   .ops.handler = handler_atencap));

XDP2_MAKE_PARSE_NODE(gre_base_node, xdp2_parse_gre_base, gre_base_table,
		     (.ops.handler = handler_greov));

XDP2_DECL_FLAG_FIELDS_TABLE(gre_v0_flag_fields_table);
XDP2_MAKE_FLAG_FIELDS_PARSE_NODE(gre_v0_node, xdp2_parse_gre_v0, gre_v0_table,
				 gre_v0_flag_fields_table,
				 (.ops.extract_metadata = extract_gre_v0,
				  .ops.handler = handler_grev0), ());

XDP2_MAKE_FLAG_FIELDS_PARSE_NODE(gre_v1_node, xdp2_parse_gre_v1,
				 gre_v1_table, gre_v1_flag_fields_table,
				 (.ops.extract_metadata = extract_gre_v1,
				  .ops.handler = handler_grev1,
				  .flags = XDP2_PARSE_NODE_F_ZERO_LEN_OK), ());

XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_flag_csum_node,
				(.ops.extract_metadata =
						extract_gre_flag_csum));
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_flag_key_node,
				(.ops.extract_metadata =
						extract_gre_flag_key));
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_flag_seq_node,
				(.ops.extract_metadata =
						extract_gre_flag_seq));

XDP2_PTH_MAKE_SIMPLE_FLAG_FIELD_HANDLER(gre_pptp_key, "GRE PPTP key")
XDP2_PTH_MAKE_SIMPLE_FLAG_FIELD_HANDLER(gre_pptp_ack, "GRE PPTP ack")
XDP2_PTH_MAKE_SIMPLE_FLAG_FIELD_HANDLER(gre_pptp_seq, "GRE PPTP seq")

XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_pptp_flag_ack_node,
				(.ops.extract_metadata =
						extract_gre_flag_ack,
				 .ops.handler = handler_gre_pptp_ack));
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_pptp_flag_key_node,
				(.ops.extract_metadata =
						extract_gre_flag_key,
				 .ops.handler = handler_gre_pptp_key));
XDP2_MAKE_FLAG_FIELD_PARSE_NODE(gre_pptp_flag_seq_node,
				(.ops.extract_metadata =
						extract_gre_flag_seq,
				 .ops.handler = handler_gre_pptp_seq));

XDP2_PTH_MAKE_SIMPLE_HANDLER(e8021AD, "VLAN e8021AD")
XDP2_PTH_MAKE_SIMPLE_HANDLER(e8021Q, "VLAN e8021Q")

XDP2_MAKE_PARSE_NODE(e8021AD_node, xdp2_parse_vlan, ether_table,
		     (.ops.extract_metadata = extract_e8021AD,
		      .ops.handler = handler_e8021AD));

XDP2_MAKE_PARSE_NODE(e8021Q_node, xdp2_parse_vlan, ether_table,
		     (.ops.extract_metadata = extract_e8021Q,
		      .ops.handler = handler_e8021Q));

XDP2_MAKE_PARSE_NODE(sunh_node_alt, sunh_parse, ip6_table,
		     (.ops.extract_metadata = extract_sunh,
		      .ops.handler = handler_sunh));

/* Protocol tables */

XDP2_MAKE_PROTO_TABLE(ether_table,
	( __cpu_to_be16(ETH_P_IP), ip_overlay_node ),
	( __cpu_to_be16(ETH_P_IPV6), ip_overlay_node ),
	( __cpu_to_be16(ETH_P_PPP_SES), pppoe_node ),
	( __cpu_to_be16(0x880b), ppp_node ),
	( __cpu_to_be16(ETH_P_8021AD), e8021AD_node ),
	( __cpu_to_be16(ETH_P_8021Q), e8021Q_node ),
	( __cpu_to_be16(ETH_P_ARP), arp_node ),
	( __cpu_to_be16(ETH_P_TEB), ether_node ),
	( __cpu_to_be16(ETH_P_SUNH), sunh_node_alt )
);

XDP2_MAKE_PROTO_TABLE(ip_overlay_table,
	( 4, ipv4_check_node ),
	( 6, ipv6_check_node )
);

XDP2_MAKE_PROTO_TABLE(ip_overlay_by_key_table,
	( 4, ipv4_node ),
	( 6, ipv6_node )
);

#define IPPROTO_OSPF 89

XDP2_MAKE_PROTO_TABLE(ip4_table,
	( IPPROTO_TCP, tcp_node ),
	( IPPROTO_UDP, udp_node ),
	( IPPROTO_GRE, gre_base_node ),
	( IPPROTO_L2TP, l2tp_base_node ),
	( IPPROTO_IPIP, ipv4ip_node ),
	( IPPROTO_IPV6, ipv6ip_node ),
	( IPPROTO_ICMP, icmpv4_node ),
	( IPPROTO_OSPF, ospf_node )
);

XDP2_PTH_MAKE_SIMPLE_EH_HANDLER(ipv6_hbh_options, "IPv6 HBH options")
XDP2_PTH_MAKE_SIMPLE_EH_HANDLER(ipv6_dest_options, "IPv6 Dest options")
XDP2_PTH_MAKE_SIMPLE_EH_HANDLER(ipv6_routing_header, "IPv6 Routing header")
XDP2_PTH_MAKE_SIMPLE_EH_HANDLER(ipv6_routing_header_check,
				"IPv6 Routing header check")
XDP2_PTH_MAKE_SIMPLE_EH_HANDLER(ipv6_fragment_header, "IPv6 Fragment header")
XDP2_PTH_MAKE_SIMPLE_EH_HANDLER(ipv6_ah_header, "IPv6 Authentication header")

static int handler_ipv6_srv6_routing_header(const void *hdr, size_t hdr_len,
					    size_t hdr_off, void *metadata,
					    void *_frame,
					    const struct xdp2_ctrl_data *ctrl)
{
	const struct ipv6_sr_hdr *srhdr = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tSRv6 header length %u\n",
				     ipv6_optlen((struct ipv6_opt_hdr *)hdr));

	if (verbose >= 5) {
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSegments left: %u\n",
				     srhdr->segments_left);
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tFirst segment: %u\n",
				     srhdr->first_segment);
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tFlags: %u\n", srhdr->flags);
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tTag: %u\n", srhdr->tag);

		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSegments:\n");
	}

	return 0;
}

static int handler_ipv6_srv6_segment(const void *hdr, size_t hdr_len,
				     size_t hdr_off, void *metadata,
				     void *_frame,
				     const struct xdp2_ctrl_data *ctrl)
{
	const struct in6_addr *addr = hdr;
	char sbuf[INET6_ADDRSTRLEN];

	inet_ntop(AF_INET6, addr, sbuf, sizeof(sbuf));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t\t%u: %s\n",
			     ctrl->key.counters[2]++, sbuf);

	return 0;
}

XDP2_MAKE_PARSE_NODE(ipv6_hbh_options_node, xdp2_parse_ipv6_eh, ip6_table,
		     (.ops.extract_metadata = extract_eh,
		      .ops.handler = handler_ipv6_hbh_options,
		      .unknown_ret = XDP2_STOP_OKAY));

XDP2_MAKE_PARSE_NODE(ipv6_dest_options_node, xdp2_parse_ipv6_eh, ip6_table,
		     (.ops.extract_metadata = extract_eh,
		      .ops.handler = handler_ipv6_dest_options,
		      .unknown_ret = XDP2_STOP_OKAY));

XDP2_MAKE_PARSE_NODE(ipv6_routing_header_node,
		     xdp2_parse_ipv6_eh, ip6_table,
		     (.ops.extract_metadata = extract_eh,
		      .ops.handler = handler_ipv6_routing_header,
		      .unknown_ret = XDP2_STOP_OKAY));

XDP2_MAKE_ARREL_PARSE_NODE(ipv6_srv6_segment,
			   (.ops.handler = handler_ipv6_srv6_segment));

XDP2_MAKE_ARRAY_PARSE_NODE_NOTAB(ipv6_srv6_routing_header_node,
				 xdp2_parse_srv6_seg_list, ip6_table,
				 (.ops.extract_metadata = extract_eh,
				  .ops.handler =
					handler_ipv6_srv6_routing_header,
				  .unknown_ret = XDP2_STOP_OKAY),
				 (.array_wildcard_node =
					XDP2_ARREL_NODE(ipv6_srv6_segment)));

XDP2_MAKE_PARSE_NODE(ipv6_routing_header_node_check,
		     xdp2_parse_ipv6_routing_hdr, rthdr_table,
		     (.ops.handler = handler_ipv6_routing_header_check,
		      .wildcard_node = XDP2_PARSE_NODE(
					ipv6_routing_header_node)));

XDP2_MAKE_PARSE_NODE(ipv6_fragment_header_node,
		     xdp2_parse_ipv6_eh, ip6_table,
		     (.ops.handler = handler_ipv6_fragment_header,
		      .unknown_ret = XDP2_STOP_OKAY));

XDP2_MAKE_PARSE_NODE(ipv6_ah_header_node,
		     xdp2_parse_ipv6_eh, ip6_table,
		     (.ops.extract_metadata = extract_eh,
		      .ops.handler = handler_ipv6_ah_header,
		      .unknown_ret = XDP2_STOP_OKAY));

XDP2_MAKE_PROTO_TABLE(ip6_table,
	( IPPROTO_TCP, tcp_node ),
	( IPPROTO_UDP, udp_node ),
	( IPPROTO_GRE, gre_base_node ),
	( IPPROTO_L2TP, l2tp_base_node ),
	( IPPROTO_IPIP, ipv4ip_node ),
	( IPPROTO_IPV6, ipv6ip_node ),
	( IPPROTO_ICMPV6, icmpv6_node ),
	( IPPROTO_HOPOPTS, ipv6_hbh_options_node ),
	( IPPROTO_DSTOPTS, ipv6_dest_options_node ),
	( IPPROTO_ROUTING, ipv6_routing_header_node_check ),
	( IPPROTO_FRAGMENT, ipv6_fragment_header_node ),
	( IPPROTO_AH, ipv6_ah_header_node ),
	( IPPROTO_SUPERP, superp_pdl_node )
);

XDP2_MAKE_PROTO_TABLE(rthdr_table,
	( 4, ipv6_srv6_routing_header_node )
);

XDP2_MAKE_PARSE_NODE(l2tp_v0_node, xdp_parse_l2tp_v0_base,
		     l2tp_v0_base_table, (.ops.handler = handler_l2tp_v0));

XDP2_MAKE_AUTONEXT_PARSE_NODE(l2tp_v0_offsz_node, xdp2_parse_l2tp_v0_offsz,
			      ppp_node, (.ops.handler = handler_l2tp_v0));

XDP2_MAKE_AUTONEXT_PARSE_NODE(l2tp_v1_node, xdp2_parse_l2tp_v0_offsz,
			      ppp_node, (.ops.handler = handler_l2tp_v1));

XDP2_MAKE_PARSE_NODE(l2tp_v2_node, xdp_parse_l2tp_v0_base,
		     l2tp_v0_base_table, (.ops.handler = handler_l2tp_v2));

XDP2_MAKE_LEAF_PARSE_NODE(l2tp_v3_node, xdp2_parse_null,
			  (.ops.handler = handler_l2tp_v3));

XDP2_MAKE_PROTO_TABLE(l2tp_v0_base_table,
	( 0, ppp_node ),
	( 1, l2tp_v0_offsz_node )
);

XDP2_MAKE_PROTO_TABLE(l2tp_base_table,
	( 0, l2tp_v0_node ),
	( 1, l2tp_v1_node ),
	( 2, l2tp_v2_node ),
	( 3, l2tp_v3_node )
);

XDP2_MAKE_PROTO_TABLE(geneve_base_table,
	( 0, geneve_v0_node )
);

XDP2_MAKE_TLV_TABLE(geneve_class_table,
	( 0, geneve_class_0_tlv_node )
);

XDP2_MAKE_TLV_TABLE(geneve_tlv_table,
	( 0x80, geneve_class_0_tlv_80_node )
);

XDP2_PTH_MAKE_SIMPLE_HANDLER(ppp_lcp, "PPP LCP")
XDP2_PTH_MAKE_SIMPLE_HANDLER(ppp_pap, "PPP PAP")
XDP2_PTH_MAKE_SIMPLE_HANDLER(ppp_chap, "PPP CHAP")
XDP2_PTH_MAKE_SIMPLE_HANDLER(ppp_icpc, "PPP ICPC")

XDP2_MAKE_LEAF_PARSE_NODE(ppp_lcp_node, xdp2_parse_null,
			  (.ops.handler = handler_ppp_lcp));

XDP2_MAKE_LEAF_PARSE_NODE(ppp_pap_node, xdp2_parse_null,
			  (.ops.handler = handler_ppp_pap));

XDP2_MAKE_LEAF_PARSE_NODE(ppp_chap_node, xdp2_parse_null,
			  (.ops.handler = handler_ppp_chap));

XDP2_MAKE_LEAF_PARSE_NODE(ppp_icpc_node, xdp2_parse_null,
			  (.ops.handler = handler_ppp_icpc));

XDP2_MAKE_PROTO_TABLE(ppp_table,
	( __cpu_to_be16(0x21), ip_overlay_node ),
	( __cpu_to_be16(0xc021), ppp_lcp_node ),
	( __cpu_to_be16(0xc023), ppp_pap_node ),
	( __cpu_to_be16(0xc223), ppp_chap_node ),
	( __cpu_to_be16(0x8021), ppp_icpc_node ),
	( __cpu_to_be16(0x0057), ipv6_node )
);

XDP2_MAKE_PROTO_TABLE(gre_base_table,
	( 0, gre_v0_node ),
	( 1, gre_v1_node )
);

XDP2_MAKE_PROTO_TABLE(gre_v0_table,
	( __cpu_to_be16(ETH_P_IP), ip_overlay_node ),
	( __cpu_to_be16(ETH_P_IPV6), ip_overlay_node ),
	( __cpu_to_be16(ETH_P_TEB), ether_node ),
	( __cpu_to_be16(ETH_P_PPP_SES), pppoe_node ),
	( __cpu_to_be16(0x880b), ppp_node )
);

XDP2_MAKE_PROTO_TABLE(gre_v1_table,
	( __cpu_to_be16(ETH_P_IP), ipv4_check_node ),
	( __cpu_to_be16(ETH_P_IPV6), ipv6_check_node ),
	( __cpu_to_be16(ETH_P_TEB), ether_node ),
	( __cpu_to_be16(ETH_P_PPP_SES), pppoe_node ),
	( __cpu_to_be16(0x880b), ppp_node )
);

XDP2_MAKE_PROTO_TABLE(udp_ports_table,
	( __cpu_to_be16(8888), protobufs1_node ),
	( __cpu_to_be16(9999), protobufs2_node ),
	( __cpu_to_be16(4789), vxlan_node ),
	( __cpu_to_be16(1701), l2tp_base_node ),
	( __cpu_to_be16(6081), geneve_base_node ),
	( __cpu_to_be16(UET_UDP_PORT_NUM), uet_base_node ),
	( __cpu_to_be16(FALCON_UDP_PORT_NUM), falcon_base_node ),
	( __cpu_to_be16(SUE_UDP_PORT_NUM), sue_base_node ),
	( __cpu_to_be16(SUPERP_UDP_PORT_NUM), superp_pdl_node )
);

XDP2_MAKE_PROTO_TABLE(icmpv6_table,
	( 135, icmpv6_neigh_solicit ),
	( 136, icmpv6_neigh_advert )
);

XDP2_MAKE_TLV_TABLE(protobufs1_table,
	( 1, protobufs1_name_node ),
	( 2, protobufs1_id_node ),
	( 3, protobufs1_email_node ),
	( 4, protobufs1_phones_node )
);

XDP2_MAKE_TLV_TABLE(protobufs2_phone_table,
	( 1, protobufs2_phone_number_node ),
	( 2, protobufs2_phone_type_node )
);

XDP2_MAKE_FLAG_FIELDS_TABLE(gre_v0_flag_fields_table,
	( GRE_FLAGS_CSUM_IDX, gre_flag_csum_node ),
	( GRE_FLAGS_KEY_IDX, gre_flag_key_node ),
	( GRE_FLAGS_SEQ_IDX, gre_flag_seq_node )
);

XDP2_MAKE_FLAG_FIELDS_TABLE(gre_v1_flag_fields_table,
	( GRE_PPTP_FLAGS_CSUM_IDX, XDP2_FLAG_NODE_NULL ),
	( GRE_PPTP_FLAGS_KEY_IDX, gre_pptp_flag_key_node ),
	( GRE_PPTP_FLAGS_SEQ_IDX, gre_pptp_flag_seq_node ),
	( GRE_PPTP_FLAGS_ACK_IDX, gre_pptp_flag_ack_node )
);

XDP2_MAKE_TLV_TABLE(tcp_tlv_table,
	( TCPOPT_TIMESTAMP, tcp_opt_timestamp_node ),
	( TCPOPT_SACK, tcp_opt_sack_node ),
	( TCPOPT_MSS, tcp_opt_mss_node ),
	( TCPOPT_WINDOW, tcp_opt_wscale_node ),
	( TCPOPT_SACK_PERM, tcp_opt_sack_permitted_node )
);

XDP2_MAKE_TLV_TABLE(tcp_sack_tlv_table,
	( 0x0a, tcp_opt_sack_1 ),
	( 0x12, tcp_opt_sack_2 ),
	( 0x1a, tcp_opt_sack_3 ),
	( 0x22, tcp_opt_sack_4 )
);

/* Make the parser */

XDP2_PARSER(parse_dump, "Parser to dump protocols", ether_node_root,
	    (.okay_node = &okay_node.pn,
	     .fail_node = &fail_node.pn,
	     .atencap_node = &atencap_node.pn,
	     .max_frames = METADATA_FRAME_COUNT,
	     .metameta_size = sizeof(struct metametadata),
	     .frame_size = sizeof(struct metadata),
	     .num_counters = 8,
	     .num_keys = 7
	    )
)
