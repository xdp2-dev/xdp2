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

#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/types.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "xdp2/parser_test_helpers.h"
#include "xdp2/parser.h"
#include "xdp2/parser_metadata.h"
#include "xdp2/pcap.h"
#include "xdp2/utility.h"

#include <netinet/ether.h>

#include "siphash/siphash.h"

#include "parse_dump.h"

static void print_vlan(struct metadata *frame, int seqno)
{
	int i;

	for (i = 0; i < frame->vlan_cnt; i++) {
		XDP2_PTH_PRINTFC(seqno, "\tVLAN #%u\n", i);
		XDP2_PTH_PRINTFC(seqno, "\t\tVLAN ID: %u\n", frame->vlan[i].id);
		XDP2_PTH_PRINTFC(seqno, "\t\tDEI: %u\n", frame->vlan[i].dei);
		XDP2_PTH_PRINTFC(seqno, "\t\tPriority: %u\n", frame->vlan[i].priority);
		XDP2_PTH_PRINTFC(seqno, "\t\tTCI: %u\n", frame->vlan[i].tci);
		XDP2_PTH_PRINTFC(seqno, "\t\tTPID: 0x%04x\n",
			ntohs(frame->vlan[i].tpid));
		XDP2_PTH_PRINTFC(seqno, "\t\tENC PROTO: 0x%04x\n",
			frame->vlan[i].enc_proto);
	}
}

static void print_gre(struct metadata *frame, int seqno)
{
	unsigned int gre_version = frame->gre_version & 0x7f;

	XDP2_PTH_PRINTFC(seqno, "\tGRE version %u\n", gre_version);

	switch (gre_version) {
	case 0:
		if (frame->gre.key_id.val)
			XDP2_PTH_PRINTFC(seqno, "Key ID: %u\n", frame->gre.key_id.val);

		if (frame->gre.sequence)
			XDP2_PTH_PRINTFC(seqno, "Sequence: %u\n", frame->gre.sequence);

		if (frame->gre.ack)
			XDP2_PTH_PRINTFC(seqno, "Ack: %u\n", frame->gre.ack);
		break;
	case 1:
		if (frame->gre.key_id.val) {
			XDP2_PTH_PRINTFC(seqno, "\t\tPayload length: %u\n",
				frame->gre.key_id.payload_len);
			XDP2_PTH_PRINTFC(seqno, "\t\tCall ID: %u\n",
				frame->gre.key_id.callid);
		}
		if (frame->gre.sequence)
			XDP2_PTH_PRINTFC(seqno, "\t\tSequence: %u\n",
				frame->gre.sequence);

		if (frame->gre.ack)
			XDP2_PTH_PRINTFC(seqno, "\t\tAck: %u\n", frame->gre.ack);
		break;
	}
}

static void print_arp(struct metametadata *metametadata, int seqno)
{
	struct in_addr ipaddr;

	switch(metametadata->arp.op) {
	case 1:
		XDP2_PTH_PRINTFC(seqno, "\tARP Request:\n");
		XDP2_PTH_PRINTFC(seqno, "\t\tSource: %s\n",
			ether_ntoa((struct ether_addr *)metametadata->arp.sha));
		ipaddr.s_addr = metametadata->arp.sip;
		XDP2_PTH_PRINTFC(seqno, "\t\tSource IP: %s\n", inet_ntoa(ipaddr));
		XDP2_PTH_PRINTFC(seqno, "\t\tDestination: %s\n",
			ether_ntoa((struct ether_addr *)metametadata->arp.tha));
		ipaddr.s_addr = metametadata->arp.tip;
		XDP2_PTH_PRINTFC(seqno, "\t\tDestination IP: %s\n", inet_ntoa(ipaddr));
		break;
	case 2:
		XDP2_PTH_PRINTFC(seqno, "\tARP Reply:\n");
		XDP2_PTH_PRINTFC(seqno, "\t\tSource: %s\n",
			ether_ntoa((struct ether_addr *)metametadata->arp.sha));
		ipaddr.s_addr = metametadata->arp.sip;
		XDP2_PTH_PRINTFC(seqno, "\t\tSource IP: %s\n", inet_ntoa(ipaddr));
		XDP2_PTH_PRINTFC(seqno, "\t\tDestination: %s\n",
			ether_ntoa((struct ether_addr *)metametadata->arp.tha));
		ipaddr.s_addr = metametadata->arp.tip;
		XDP2_PTH_PRINTFC(seqno, "\t\tDestination IP: %s\n", inet_ntoa(ipaddr));
		break;
	default:
		XDP2_PTH_PRINTFC(seqno, "\tARP Unknown operation: %u\n",
			metametadata->arp.op);
		break;
	}
}

static void print_icmp(struct metametadata *metametadata, int seqno)
{
	__u8 type;
	struct {
		__be16 id;
		__be16 seq;
	} *echo;

	XDP2_PTH_PRINTFC(seqno, "\tICMP:\n");
	XDP2_PTH_PRINTFC(seqno, "\t\tType: %u\n", metametadata->icmp.type);
	XDP2_PTH_PRINTFC(seqno, "\t\tCode: %u\n", metametadata->icmp.code);

	type = metametadata->icmp.is_ipv6 ? 128 : 8;
	if (metametadata->icmp.type == type &&
	    metametadata->icmp.code == 0) {
		echo = (typeof(echo))&metametadata->icmp.data;

		XDP2_PTH_PRINTFC(seqno, "\t\tEcho Request ID: %u Seq: %u\n",
			ntohs(echo->id), ntohs(echo->seq));
	}

	type = metametadata->icmp.is_ipv6 ? 129 : 0;
	if (metametadata->icmp.type == type &&
	    metametadata->icmp.code == 0) {
		echo = (typeof(echo))&metametadata->icmp.data;

		XDP2_PTH_PRINTFC(seqno, "\t\tEcho Reply ID: %u Seq: %u\n",
			ntohs(echo->id), ntohs(echo->seq));
	}
}

static void print_tcp(struct metametadata *metametadata, int seqno)
{
	/* If data is present for TCP timestamp option then print */
	if (metametadata->tcp_options.timestamp.value ||
	    metametadata->tcp_options.timestamp.echo) {
		XDP2_PTH_PRINTFC(seqno, "\tTCP timestamps value: %u, echo %u\n",
		       metametadata->tcp_options.timestamp.value,
		       metametadata->tcp_options.timestamp.echo);
	}

	/* If data is present for MSS option then print */
	if (metametadata->tcp_options.mss)
		XDP2_PTH_PRINTFC(seqno, "\tTCP MSS: %u\n",
			metametadata->tcp_options.mss);

	if (metametadata->tcp_options.have_window_scaling)
		XDP2_PTH_PRINTFC(seqno, "\tTCP window scaling: %u\n",
			metametadata->tcp_options.window_scaling);

	if (metametadata->tcp_options.num_sacks) {
		int i;

		XDP2_PTH_PRINTFC(seqno, "\tSACKs %u:\n",
			metametadata->tcp_options.num_sacks);
		for (i = 0; i < metametadata->tcp_options.num_sacks; i++)
			XDP2_PTH_PRINTFC(seqno, "\t\t%u:%u\n",
				metametadata->tcp_options.sack[i].left_edge,
				metametadata->tcp_options.sack[i].right_edge);
	}
}

static void print_frame(struct metadata *frame, int seqno)
{
	bool is_ip = false, is_ipv4 = false;

	if (frame->ether_type)
		XDP2_PTH_PRINTFC(seqno, "\tEthertype: 0x%04x\n", frame->ether_type);

	/* Print IP addresses and ports in the metadata */
	switch (frame->addr_type) {
	case XDP2_ADDR_TYPE_IPV4: {
		char sbuf[INET_ADDRSTRLEN];
		char dbuf[INET_ADDRSTRLEN];

		inet_ntop(AF_INET, &frame->addrs.v4.saddr,
			  sbuf, sizeof(sbuf));
		inet_ntop(AF_INET, &frame->addrs.v4.daddr,
			  dbuf, sizeof(dbuf));

		if (frame->port_pair.sport ||
		    frame->port_pair.dport)
			XDP2_PTH_PRINTFC(seqno, "\tIPv4: %s:%u->%s:%u\n", sbuf,
			       ntohs(frame->port_pair.sport), dbuf,
			       ntohs(frame->port_pair.dport));
		else
			XDP2_PTH_PRINTFC(seqno, "\tIPv4: %s->%s\n", sbuf, dbuf);

		is_ip = true;
		is_ipv4 = true;

		lookup_tuple(frame, seqno);

		break;
	}
	case XDP2_ADDR_TYPE_IPV6: {
		char sbuf[INET6_ADDRSTRLEN];
		char dbuf[INET6_ADDRSTRLEN];

		inet_ntop(AF_INET6, &frame->addrs.v6.saddr,
			  sbuf, sizeof(sbuf));
		inet_ntop(AF_INET6, &frame->addrs.v6.daddr,
			  dbuf, sizeof(dbuf));

		if (frame->port_pair.sport ||
		    frame->port_pair.dport)
			XDP2_PTH_PRINTFC(seqno, "\tIPv6: %s:%u->%s:%u\n", sbuf,
			       ntohs(frame->port_pair.sport), dbuf,
			       ntohs(frame->port_pair.dport));
		else
			XDP2_PTH_PRINTFC(seqno, "\tIPv6: %s->%s\n", sbuf, dbuf);

		is_ip = true;

		lookup_tuple(frame, seqno);

		break;
	}
	case 0:
		XDP2_PTH_PRINTFC(seqno, "\tNo addresses\n");
		break;
	default:
		XDP2_PTH_PRINTFC(seqno, "\tUnknown addr type %u\n", frame->addr_type);
	}

	if (is_ip) {
		struct protoent *pent;

		pent = getprotobynumber(frame->ip_proto);

		XDP2_PTH_PRINTFC(seqno, "\tIP protocol %u (%s)\n", frame->ip_proto,
			pent ? pent->p_name : "unknown");
	}

	if (is_ipv4 && frame->ip_frag_off & htons(IP_MF | IP_OFFSET)) {
		if (frame->ip_frag_off & htons(IP_OFFSET))
			XDP2_PTH_PRINTFC(seqno, "\tNon-first fragment ID: %u "
				       "offset:%u\n", ntohs(frame->ip_frag_id),
				(ntohs(frame->ip_frag_off) & IP_OFFSET) >> 3);
		else
			XDP2_PTH_PRINTFC(seqno, "\tFirst fragment ID: %u\n",
				ntohs(frame->ip_frag_id));
	}

	if (frame->is_ipv6_frag) {
		if (frame->ip_frag_off & htons(IP6_OFFSET))
			XDP2_PTH_PRINTFC(seqno, "\tNon-first fragment ID: %u "
				       "offset:%u\n", ntohl(frame->ip_frag_id),
				(ntohs(frame->ip_frag_off) & IP6_OFFSET) >> 3);
		else
			XDP2_PTH_PRINTFC(seqno, "\tFirst fragment ID: %u\n",
					 ntohl(frame->ip_frag_id));
	}

	/* Compute and print hash over addresses and port numbers */
	XDP2_PTH_PRINTFC(seqno, "\tHash 0x%08x\n",
		       XDP2_COMMON_COMPUTE_HASH(frame, HASH_START_FIELD));

	if (frame->gre_version)
		print_gre(frame, seqno);

	if (frame->vlan_cnt)
		print_vlan(frame, seqno);
}

void print_metadata(void *_metadata, const struct xdp2_ctrl_data *ctrl)
{
	struct pmetadata *pmetadata = (struct pmetadata *)_metadata;
	struct metametadata *metametadata = &pmetadata->metametadata;
	unsigned int seqno = ctrl->pkt.seqno;
	int i;

	for (i = 0; i < xdp2_min(ctrl->var.encaps + 1,
				 METADATA_FRAME_COUNT); i++) {
		XDP2_PTH_PRINTFC(seqno, "Frame #%u:\n", i);
		print_frame(&pmetadata->metadata[i], seqno);
	}

	XDP2_PTH_PRINTFC(seqno, "Summary:\n");

	XDP2_PTH_PRINTFC(seqno, "\tReturn code: %s\n",
		xdp2_get_text_code(ctrl->var.ret_code));

	XDP2_PTH_PRINTFC(seqno, "\tLast node: %s\n", ctrl->var.last_node->text_name);

	if (metametadata->arp_present)
		print_arp(metametadata, seqno);

	if (metametadata->tcp_present)
		print_tcp(metametadata, seqno);

	if (metametadata->icmp_present)
		print_icmp(metametadata, seqno);

	XDP2_PTH_PRINTFC(seqno, "\tCounters:\n");

	for (i = 0; i < 3; i++)
		XDP2_PTH_PRINTFC(seqno, "\t\tCounter #%u: %u\n",
				 i, ctrl->key.counters[i]);
}
