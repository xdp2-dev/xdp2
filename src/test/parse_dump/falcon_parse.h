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

#ifndef __XDP2_TEST_PARSE_DUMP_FALCON_H__
#define __XDP2_TEST_PARSE_DUMP_FALCON_H__

#include "falcon/falcon.h"

#define XDP2_DEFINE_PARSE_NODE
#include "falcon/proto_falcon.h"
#undef XDP2_DEFINE_PARSE_NODE

#include "xdp2/utility.h"

#include "parse_helpers.h"

MAKE_SIMPLE_HANDLER(falcon_base_node, "Falcon base node")
MAKE_SIMPLE_HANDLER(falcon_v1_node, "Falcon version 1")

static void print_falcon_base_header(const void *vhdr,
				     const struct xdp2_ctrl_data *ctrl)
{
	const struct falcon_base_hdr *bhdr = vhdr;

	LOC_PRINTFC(ctrl, "\t\tVersion: %u\n", bhdr->version);
	LOC_PRINTFC(ctrl, "\t\tDest cid: %u (0x%x)\n",
		    xdp2_ntohl24(bhdr->dest_cid), xdp2_ntohl24(bhdr->dest_cid));
	LOC_PRINTFC(ctrl, "\t\tDest function: %u (0x%x)\n",
		    xdp2_ntohl24(bhdr->dest_function),
		    xdp2_ntohl24(bhdr->dest_function));
	LOC_PRINTFC(ctrl, "\t\tProtocol type: %s (%u)\n",
		    falcon_proto_type_to_text(bhdr->protocol_type),
		    bhdr->protocol_type);
	LOC_PRINTFC(ctrl, "\t\tPacket type: %s (%u)\n",
		    falcon_pkt_type_to_text(bhdr->packet_type),
		    bhdr->packet_type);
	LOC_PRINTFC(ctrl, "\t\tAck req: %s\n", bhdr->ack_req ? "yes" : "no");
	LOC_PRINTFC(ctrl, "\t\tRecv data psn: %u (0x%x)\n",
		    htonl(bhdr->ack_data_psn), htonl(bhdr->ack_data_psn));
	LOC_PRINTFC(ctrl, "\t\tRecv req psn: %u (0x%x)\n",
		    htonl(bhdr->ack_req_psn), htonl(bhdr->ack_req_psn));
	LOC_PRINTFC(ctrl, "\t\tPsn: %u (0x%x)\n", htonl(bhdr->psn), htonl(bhdr->psn));
	LOC_PRINTFC(ctrl, "\t\tReq seqno: %u (0x%x)\n",
		    htonl(bhdr->req_seqno), htonl(bhdr->req_seqno));
}

static int handler_falcon_pull_request(const void *hdr, size_t hdr_len,
				       size_t hdr_off, void *frame,
				       void *metadata,
				       const struct xdp2_ctrl_data *ctrl)
{
	const struct falcon_pull_req_pkt *pkt = hdr;

	if (verbose >= 5)
		LOC_PRINTFC(ctrl, "\t\tFalcon pull request\n");

	if (verbose < 10)
		return 0;

	print_falcon_base_header(&pkt->base_hdr, ctrl);

	LOC_PRINTFC(ctrl, "\t\tRequest length: %u (0x%x)\n",
		    ntohs(pkt->req_length), ntohs(pkt->req_length));

	return 0;
}

static int handler_falcon_pull_data(const void *hdr, size_t hdr_len,
				    size_t hdr_off, void *frame,
				    void *metadata,
				    const struct xdp2_ctrl_data *ctrl)
{
	const struct falcon_pull_data_pkt *pkt = hdr;

	if (verbose >= 5)
		LOC_PRINTFC(ctrl, "\t\tFalcon pull data\n");

	if (verbose < 10)
		return 0;

	print_falcon_base_header(&pkt->base_hdr, ctrl);

	return 0;
}

static int handler_falcon_push_data(const void *hdr, size_t hdr_len,
				    size_t hdr_off, void *frame,
				    void *metadata,
				    const struct xdp2_ctrl_data *ctrl)
{
	const struct falcon_push_data_pkt *pkt = hdr;

	if (verbose >= 5)
		LOC_PRINTFC(ctrl, "\t\tFalcon push data\n");

	if (verbose < 10)
		return 0;

	print_falcon_base_header(&pkt->base_hdr, ctrl);

	LOC_PRINTFC(ctrl, "\t\tRequest length: %u (0x%x)\n",
		    ntohs(pkt->req_length), ntohs(pkt->req_length));

	return 0;
}

static int handler_falcon_resync(const void *hdr, size_t hdr_len,
				 size_t hdr_off, void *frame,
				 void *metadata,
				 const struct xdp2_ctrl_data *ctrl)
{
	const struct falcon_resync_pkt *pkt = hdr;

	if (verbose >= 5)
		LOC_PRINTFC(ctrl, "\t\tFalcon resync\n");

	if (verbose < 10)
		return 0;

	print_falcon_base_header(&pkt->base_hdr, ctrl);

	LOC_PRINTFC(ctrl, "\t\tResync code: %s (%u)\n",
		    falcon_resync_code_to_text(pkt->resync_code),
		    pkt->resync_code);
	LOC_PRINTFC(ctrl, "\t\tResync packet type: %s (%u)\n",
		    falcon_pkt_type_to_text(pkt->resync_pkt_type),
		    pkt->resync_pkt_type);
	LOC_PRINTFC(ctrl, "\t\tVendor defined: %u (0x%x)\n",
		    ntohl(pkt->vendor_defined), ntohl(pkt->vendor_defined));

	return 0;
}

static void print_falcon_base_ack(const void *vhdr,
				  const struct xdp2_ctrl_data *ctrl)
{
	const struct falcon_base_ack_pkt *bhdr = vhdr;

	LOC_PRINTFC(ctrl, "\t\tVersion: %u\n", bhdr->version);
	LOC_PRINTFC(ctrl, "\t\tCnx ID: %u (0x%x)\n",
		   xdp2_ntohl24(bhdr->dest_cid), xdp2_ntohl24(bhdr->dest_cid));
	LOC_PRINTFC(ctrl, "\t\tPacket type: %s (%u)\n",
		    falcon_pkt_type_to_text(bhdr->pkt_type), bhdr->pkt_type);

	LOC_PRINTFC(ctrl, "\t\tReceive data ack seqno: %u (0x%x)\n",
		    ntohl(bhdr->ack_data_psn), ntohl(bhdr->ack_data_psn));
	LOC_PRINTFC(ctrl, "\t\tReceive request ack seqno: %u (0x%x)\n",
		    ntohl(bhdr->ack_req_psn), ntohl(bhdr->ack_req_psn));
	LOC_PRINTFC(ctrl, "\t\tTimestamp t1: %u (0x%x)\n",
		    ntohl(bhdr->timestamp_t1), ntohl(bhdr->timestamp_t1));
	LOC_PRINTFC(ctrl, "\t\tTimestamp t2: %u (0x%x)\n",
		    ntohl(bhdr->timestamp_t2), ntohl(bhdr->timestamp_t2));

	LOC_PRINTFC(ctrl, "\t\tHop count: %u (0x%x)\n",
		    bhdr->hop_count, bhdr->hop_count);
	LOC_PRINTFC(ctrl, "\t\tECN packet count: %u (0x%x)\n",
		    falcon_base_ack_get_ecn_rx_pkt_cnt(bhdr),
		    falcon_base_ack_get_ecn_rx_pkt_cnt(bhdr));
	LOC_PRINTFC(ctrl, "\t\tRX buffer occupancy: %u (0x%x)\n",
		    falcon_base_ack_get_rx_buffer_occ(bhdr),
		    falcon_base_ack_get_rx_buffer_occ(bhdr));

	LOC_PRINTFC(ctrl, "\t\tRUE info: %u (0x%x)\n",
		    falcon_base_ack_get_rue_info(bhdr),
		    falcon_base_ack_get_rue_info(bhdr));

	LOC_PRINTFC(ctrl, "\t\tOut of window: %u (0x%x)\n",
		    bhdr->oo_wind_notify, bhdr->oo_wind_notify);
}

static int handler_falcon_back(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *frame,
			       void *metadata,
			       const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		LOC_PRINTFC(ctrl, "\t\tFalcon ack\n");

	if (verbose < 10)
		return 0;

	print_falcon_base_ack(hdr, ctrl);

	return 0;
}

static int handler_falcon_eack(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *frame,
			       void *metadata,
			       const struct xdp2_ctrl_data *ctrl)
{
	const struct falcon_ext_ack_pkt *eack = hdr;

	if (verbose >= 5)
		LOC_PRINTFC(ctrl, "\t\tFalcon eack\n");

	if (verbose < 10)
		return 0;

	print_falcon_base_ack(&eack->base_ack, ctrl);

	LOC_PRINTFC(ctrl, "\t\tData ack bitmap0: %016llx\n",
		    ntohll(eack->data_ack_bitmap[0]));
	LOC_PRINTFC(ctrl, "\t\tData ack bitmap1: %016llx\n",
		    ntohll(eack->data_ack_bitmap[1]));
	LOC_PRINTFC(ctrl, "\t\tData rx bitmap0: %016llx\n",
		    ntohll(eack->data_rx_bitmap[0]));
	LOC_PRINTFC(ctrl, "\t\tData rx bitmap1: %016llx\n",
		    ntohll(eack->data_rx_bitmap[1]));
	LOC_PRINTFC(ctrl, "\t\tReq ack bitmap: %016llx\n",
		    ntohll(eack->req_ack_bitmap));

	return 0;
}

static int handler_falcon_nack(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *frame,
			       void *metadata,
			       const struct xdp2_ctrl_data *ctrl)
{
	const struct falcon_nack_pkt *nack = hdr;

	if (verbose >= 5)
		LOC_PRINTFC(ctrl, "\t\tFalcon nack\n");

	if (verbose < 10)
		return 0;

	LOC_PRINTFC(ctrl, "\t\tVersion: %u\n", nack->version);
	LOC_PRINTFC(ctrl, "\t\tCnx ID: %u (0x%x)\n",
		    xdp2_ntohl24(nack->dest_cid),
		    xdp2_ntohl24(nack->dest_cid));
	LOC_PRINTFC(ctrl, "\t\tPacket type: %s (%u)\n",
		    falcon_pkt_type_to_text(nack->pkt_type),
		    nack->pkt_type);

	LOC_PRINTFC(ctrl, "\t\tReceive data ack seqno: %u (0x%x)\n",
		    ntohl(nack->ack_data_psn),
		    ntohl(nack->ack_data_psn));
	LOC_PRINTFC(ctrl, "\t\tReceive request ack seqno: %u (0x%x)\n",
		    ntohl(nack->ack_req_psn),
		    ntohl(nack->ack_req_psn));
	LOC_PRINTFC(ctrl, "\t\tTimestamp t1: %u (0x%x)\n",
		    ntohl(nack->timestamp_t1), ntohl(nack->timestamp_t1));
	LOC_PRINTFC(ctrl, "\t\tTimestamp t2: %u (0x%x)\n",
		    ntohl(nack->timestamp_t2), ntohl(nack->timestamp_t2));

	LOC_PRINTFC(ctrl, "\t\tHop count: %u (0x%x)\n",
		    nack->hop_count, nack->hop_count);
	LOC_PRINTFC(ctrl, "\t\tRX buffer occupancy: %u (0x%x)\n",
		    falcon_nack_get_rx_buffer_occ(nack),
		    falcon_nack_get_rx_buffer_occ(nack));
	LOC_PRINTFC(ctrl, "\t\tECN packet count: %u (0x%x)\n",
		    falcon_nack_get_ecn_rx_pkt_cnt(nack),
		    falcon_nack_get_ecn_rx_pkt_cnt(nack));

	LOC_PRINTFC(ctrl, "\t\tRUE info: %u (0x%x)\n",
		    falcon_nack_get_rue_info(nack),
		    falcon_nack_get_rue_info(nack));
	LOC_PRINTFC(ctrl, "\t\tNack PSN: %u (0x%x)\n",
		    ntohl(nack->nack_psn), ntohl(nack->nack_psn));

	LOC_PRINTFC(ctrl, "\t\tNack code: %s (%u)\n",
		    falcon_nack_code_to_text(nack->nack_code),
		    nack->nack_code);
	LOC_PRINTFC(ctrl, "\t\tRecv Not Ready timeout: %u "
			  "(%.2f msecs)\n", nack->rnr_nack_timeout,
			  (float)falcon_rnr_nack_field_val_to_timeout(
				nack->rnr_nack_timeout) / 100.0);
	LOC_PRINTFC(ctrl, "\t\tWindow: %s (%u)\n",
		   nack->window ? "request sliding window" :
				  "data sliding window",
		   nack->window);

	LOC_PRINTFC(ctrl, "\t\tULP Nack code : %u (0x%x)\n",
		    nack->ulp_nack_code, nack->ulp_nack_code);

	return 0;
}

XDP2_MAKE_PARSE_NODE(falcon_base_node, falcon_parse_version,
		     falcon_version_table,
		     (.ops.handler = handler_falcon_base_node));

XDP2_MAKE_PARSE_NODE(falcon_v1_node, falcon_parse_packet_type,
		     falcon_packet_type_table,
		     (.ops.handler = handler_falcon_v1_node));

XDP2_MAKE_LEAF_PARSE_NODE(falcon_pull_request, falcon_parse_pull_request,
			  (.ops.handler = handler_falcon_pull_request));
XDP2_MAKE_LEAF_PARSE_NODE(falcon_pull_data, falcon_parse_pull_data,
			  (.ops.handler = handler_falcon_pull_data));
XDP2_MAKE_LEAF_PARSE_NODE(falcon_push_data, falcon_parse_push_data,
			  (.ops.handler = handler_falcon_push_data));
XDP2_MAKE_LEAF_PARSE_NODE(falcon_resync, falcon_parse_resync,
			  (.ops.handler = handler_falcon_resync));
XDP2_MAKE_LEAF_PARSE_NODE(falcon_back, falcon_parse_back,
			  (.ops.handler = handler_falcon_back));
XDP2_MAKE_LEAF_PARSE_NODE(falcon_nack, falcon_parse_nack,
			  (.ops.handler = handler_falcon_nack));
XDP2_MAKE_LEAF_PARSE_NODE(falcon_eack, falcon_parse_eack,
			  (.ops.handler = handler_falcon_eack));

XDP2_MAKE_PROTO_TABLE(falcon_version_table,
	( 1, falcon_v1_node )
);

XDP2_MAKE_PROTO_TABLE(falcon_packet_type_table,
	( FALCON_PKT_TYPE_PULL_REQUEST, falcon_pull_request ),
	( FALCON_PKT_TYPE_PULL_DATA, falcon_pull_data ),
	( FALCON_PKT_TYPE_PUSH_DATA, falcon_push_data ),
	( FALCON_PKT_TYPE_RESYNC, falcon_resync ),
	( FALCON_PKT_TYPE_BACK, falcon_back ),
	( FALCON_PKT_TYPE_EACK, falcon_eack ),
	( FALCON_PKT_TYPE_NACK, falcon_nack )
);

#endif /* __XDP2_TEST_PARSE_DUMP_FALCON_H__ */
