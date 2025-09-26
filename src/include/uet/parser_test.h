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

#ifndef __UET_PARSER_TEST_H__
#define __UET_PARSER_TEST_H__

/* Defines UET parser nodes for common parser test framework */

#include "uet/pds.h"
#include "uet/ses.h"

#define XDP2_DEFINE_PARSE_NODE
#include "uet/proto_uet_pds.h"
#include "uet/proto_uet_ses.h"
#undef XDP2_DEFINE_PARSE_NODE

#include "xdp2/parser_test_helpers.h"

/* Parser definitions in parse_dump for UET */

/************************ PDS ****************************/

/* Print ROD/RUD request */
static void print_pds_rud_rod_request(
		const struct uet_pds_rud_rod_request *rhdr,
		const struct xdp2_ctrl_data *ctrl)
{
	__u8 next_hdr = uet_pds_common_get_next_hdr(rhdr);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tType: %s (%u)\n",
			     uet_pkt_type_to_text(rhdr->type), rhdr->type);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tNext header: %s (%u)\n",
			     uet_next_header_type_to_text(next_hdr), next_hdr);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRetransmission: %s\n",
			     rhdr->retrans ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tAck requested: %s\n",
			     rhdr->ackreq ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSYN: %s\n",
			     rhdr->syn ? "yes" : "no");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tClear PSN offset: %u (0x%x)\n",
			     ntohs(rhdr->clear_psn_offset),
			     ntohs(rhdr->clear_psn_offset));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPSN: %u (0x%x)\n",
			     ntohl(rhdr->psn), ntohl(rhdr->psn));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSPDcid: %u (0x%x)\n",
			     ntohs(rhdr->spdcid), ntohs(rhdr->spdcid));

	if (rhdr->syn) {
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPSN offset %u (0x%x)\n",
				uet_pds_rud_rod_request_get_psn_offset(rhdr),
				uet_pds_rud_rod_request_get_psn_offset(rhdr));
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPDC source %s\n",
			rhdr->use_rsv_pdc ? "PDC from reserved pool" :
					    "PDC from global shared ppol");
	} else {
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tDPDcid: %u (0x%x)\n",
				     ntohs(rhdr->dpdcid), ntohs(rhdr->dpdcid));
	}
}

/* Print ROD/RUD request with CC */
static void print_pds_rud_rod_request_cc(
		const struct uet_pds_rud_rod_request_cc *pkt,
		const struct xdp2_ctrl_data *ctrl)
{
	print_pds_rud_rod_request(&pkt->req, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- CC\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tCCC ID: %u (0x%x)\n",
			     pkt->req_cc_state.ccc_id,
			     pkt->req_cc_state.ccc_id);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tCredit target %u (0x%x)\n",
			     xdp2_ntohl24(pkt->req_cc_state.credit_target),
			     xdp2_ntohl24(pkt->req_cc_state.credit_target));
}

/* Handle plain RUD request */
static int handler_pds_rud_request(const void *hdr, size_t hdr_len,
				   size_t hdr_off, void *metadata, void *frame,
				   const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_rud_rod_request *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS RUD request\n");

	if (verbose < 10)
		return 0;

	print_pds_rud_rod_request(pkt, ctrl);

	return 0;
}

/* Handle RUD request with a SYN */
static int handler_pds_rud_request_syn(const void *hdr, size_t hdr_len,
				       size_t hdr_off, void *metadata,
				       void *frame,
				       const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_rud_rod_request *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS RUD request SYN\n");

	if (verbose < 10)
		return 0;

	print_pds_rud_rod_request(pkt, ctrl);

	return 0;
}

/* Handle RUD request with CC */
static int handler_pds_rud_request_cc(const void *hdr, size_t hdr_len,
				      size_t hdr_off, void *metadata,
				      void *frame,
				      const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_rud_rod_request_cc *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS RUD request CC\n");

	if (verbose < 10)
		return 0;

	print_pds_rud_rod_request_cc(pkt, ctrl);

	return 0;
}

/* Handle a RUD request with a SYN and CC */
static int handler_pds_rud_request_cc_syn(const void *hdr, size_t hdr_len,
					  size_t hdr_off, void *metadata,
					  void *frame,
					  const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_rud_rod_request_cc *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS RUD request CC SYN\n");

	if (verbose < 10)
		return 0;

	print_pds_rud_rod_request_cc(pkt, ctrl);

	return 0;
}

/* Handle plain ROD request */
static int handler_pds_rod_request(const void *hdr, size_t hdr_len,
				   size_t hdr_off, void *metadata,
				   void *frame,
				   const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_rud_rod_request *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS ROD request\n");

	if (verbose < 10)
		return 0;

	print_pds_rud_rod_request(pkt, ctrl);

	return 0;
}

/* Handle ROD request with a SYN */
static int handler_pds_rod_request_syn(const void *hdr, size_t hdr_len,
				       size_t hdr_off, void *metadata,
				       void *frame,
				       const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_rud_rod_request *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS ROD request SYN\n");

	if (verbose < 10)
		return 0;

	print_pds_rud_rod_request(pkt, ctrl);

	return 0;
}

/* Handle ROD request with CC */
static int handler_pds_rod_request_cc(const void *hdr, size_t hdr_len,
				      size_t hdr_off, void *metadata,
				      void *frame,
				      const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_rud_rod_request_cc *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS ROD request CC\n");

	if (verbose < 10)
		return 0;

	print_pds_rud_rod_request_cc(pkt, ctrl);

	return 0;
}

/* Handle a ROD request with a SYN and CC */
static int handler_pds_rod_request_cc_syn(const void *hdr, size_t hdr_len,
					  size_t hdr_off, void *metadata,
					  void *frame,
					  const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_rud_rod_request_cc *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS ROD request CC SYN\n");

	if (verbose < 10)
		return 0;

	print_pds_rud_rod_request_cc(pkt, ctrl);

	return 0;
}

/* Print common PSD ACK info */
static void print_pds_common_ack(const struct uet_pds_ack *pkt,
				 const struct xdp2_ctrl_data *ctrl)
{
	__u8 next_hdr = uet_pds_common_get_next_hdr(pkt);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tType: %s (%u)\n",
			     uet_pkt_type_to_text(pkt->type), pkt->type);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tNext header: %s (%u)\n",
			     uet_next_header_type_to_text(next_hdr), next_hdr);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tECN marked: %s\n",
			     pkt->ecn_marked ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRetransmission: %s\n",
			     pkt->retrans ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tProbe: %s\n",
			     pkt->probe ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRequest type: %s (%u)\n",
			     uet_pds_ack_request_type_to_text(pkt->request),
			     pkt->request);

	if (pkt->probe)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tProbe opaque: %u (0x%x)\n",
				     ntohs(pkt->probe_opaque),
				     ntohs(pkt->probe_opaque));
	else
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tACK PSN offset: %u (0x%x)\n",
				     ntohs(pkt->ack_psn_offset),
				     ntohs(pkt->ack_psn_offset));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tCACK PSN: %u (0x%x)\n",
			     ntohl(pkt->cack_psn), ntohl(pkt->cack_psn));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSPDcid: %u (0x%x)\n",
			     ntohs(pkt->spdcid), ntohs(pkt->spdcid));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tDPDcid: %u (0x%x)\n",
			     ntohs(pkt->dpdcid), ntohs(pkt->dpdcid));
}

/* Print common PDS ACK CC info */
static int handler_pds_ack(const void *hdr, size_t hdr_len,
			   size_t hdr_off, void *metadata,
			   void *frame,
			   const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_ack *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS ACK\n");

	if (verbose < 10)
		return 0;

	print_pds_common_ack(pkt, ctrl);

	return 0;
}

/* PDS ACK handler */
static void print_pds_common_ack_cc(const struct uet_pds_ack_cc *pkt,
				    const struct xdp2_ctrl_data *ctrl)
{
	print_pds_common_ack(&pkt->ack, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- Common ACK\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tCC type: %s (%u)\n",
			     uet_pds_cc_type_to_text(pkt->cc_type),
			     pkt->cc_type);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tCC flags: %u (0x%x)\n",
			     pkt->cc_flags, pkt->cc_flags);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMPR: %u (0x%x)\n", pkt->mpr, pkt->mpr);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSACK PSN offset: %u (0x%x)\n",
			     ntohs(pkt->sack_psn_offset),
			     ntohs(pkt->sack_psn_offset));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSACK bitmap: 0x%llx\n",
			     ntohll(pkt->sack_bitmap));
}

/* PDS ACK handler with CC NSCC */
static int handler_pds_ack_cc_nscc(const void *hdr, size_t hdr_len,
				   size_t hdr_off, void *metadata,
				   void *frame,
				   const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_ack_cc *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS ACK CC NSCC\n");

	if (verbose < 10)
		return 0;

	print_pds_common_ack_cc(pkt, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- NSCC\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tService time: %u (0x%x)\n",
			     ntohs(pkt->nscc.service_time),
			     ntohs(pkt->nscc.service_time));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRestore cwnd: %s\n",
			     pkt->nscc.restore_cwnd ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRecv cwnd pending: %u (0x%x)\n",
			     pkt->nscc.rcv_cwnd_pend, pkt->nscc.rcv_cwnd_pend);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tReceived bytes: %u (0x%x)\n",
			     xdp2_ntohl24(pkt->nscc.rcvd_bytes),
			     xdp2_ntohl24(pkt->nscc.rcvd_bytes));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tOOO count: %u (0x%x)\n",
			     ntohs(pkt->nscc.ooo_count),
			     ntohs(pkt->nscc.ooo_count));

	return 0;
}

/* PDS ACK handler with CC credit */
static int handler_pds_ack_cc_credit(const void *hdr, size_t hdr_len,
				     size_t hdr_off, void *metadata,
				     void *frame,
				     const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_ack_cc *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS ACK CC credit\n");

	if (verbose < 10)
		return 0;

	print_pds_common_ack_cc(pkt, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- Credit\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tCredit: %u (0x%x)\n",
			     xdp2_ntohl24(pkt->credit.credit),
			     xdp2_ntohl24(pkt->credit.credit));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tOOO count: %u (0x%x)\n",
			     ntohs(pkt->credit.ooo_count),
			     ntohs(pkt->credit.ooo_count));

	return 0;
}

/* PDS ACK handler with CCX */
static int handler_pds_ack_ccx(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *metadata,
			       void *frame, const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_ack_ccx *pkt = hdr;
	int i;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS ACK CCX\n");

	if (verbose < 10)
		return 0;

	print_pds_common_ack(&pkt->ack, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- ACK CCX\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tCCX type: %u\n", pkt->ccx_type);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tCCX flags: %u\n", pkt->cc_flags);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMPR: %u (0x%x)\n", pkt->mpr, pkt->mpr);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSACK PSN offset: %u (0x%x)\n",
			     ntohs(pkt->sack_psn_offset),
			     ntohs(pkt->sack_psn_offset));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSACK bitmap: 0x%llx\n",
			     ntohll(pkt->sack_bitmap));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tAck CC state: ");
	for (i = 0; i < 8; i++) {
		if (i)
			XDP2_PTH_LOC_PRINTFC(ctrl, " ");
		XDP2_PTH_LOC_PRINTFC(ctrl, "%02x", pkt->ack_cc_state[i]);
	}

	XDP2_PTH_LOC_PRINTFC(ctrl, "\n");

	return 0;
}

/* Print common PDS NACK info */
static void print_pds_common_nack(const struct uet_pds_nack *hdr,
				  const struct xdp2_ctrl_data *ctrl)
{
	__u8 next_hdr = uet_pds_common_get_next_hdr(hdr);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tType: %s (%u)\n",
			     uet_pkt_type_to_text(hdr->type), hdr->type);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tNext header: %s (%u)\n",
			     uet_next_header_type_to_text(next_hdr), next_hdr);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tECN marked: %s\n",
			     hdr->ecn_marked ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRetransmission: %s\n",
			     hdr->retrans ? "yes" : "no");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tNack type: %s (%u)\n",
			     uet_pds_nack_type_to_text(hdr->nack_type),
			     hdr->nack_type);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tNack code: %s (%u)\n",
			     uet_pds_nack_code_to_text(hdr->nack_code),
			     hdr->nack_code);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tVendor code: %u (%x)\n",
			     hdr->vendor_code, hdr->vendor_code);

	if (hdr->nack_type == UET_PDS_NACK_TYPE_RUD_ROD)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tNack PSN: %u (%x)\n",
				     ntohl(hdr->nack_psn),
				     ntohl(hdr->nack_psn));
	else
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tNack packet ID: %u (%x)\n",
				     ntohl(hdr->nack_pkt_id),
				     ntohl(hdr->nack_pkt_id));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSPDcid: %u (0x%x)\n",
			     ntohs(hdr->spdcid), ntohs(hdr->spdcid));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tDPDcid: %u (0x%x)\n",
			     ntohs(hdr->dpdcid), ntohs(hdr->dpdcid));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPayload: %u (0x%x)\n",
			     ntohl(hdr->payload), ntohl(hdr->payload));
}

/* PDS NACK handler */
static int handler_pds_nack(const void *hdr, size_t hdr_len,
			    size_t hdr_off, void *metadata,
			    void *frame, const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_nack *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS NACK\n");

	if (verbose < 10)
		return 0;

	print_pds_common_nack(pkt, ctrl);

	return 0;
}

/* PDS NACK handler with CCX */
static int handler_pds_nack_ccx(const void *hdr, size_t hdr_len,
				size_t hdr_off, void *metadata,
				void *frame, const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_nack_ccx *pkt = hdr;
	int i;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS NACK CCX\n");

	if (verbose < 10)
		return 0;

	print_pds_common_nack(&pkt->nack, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- NACK CCX\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tNack CCX type: 0x%x\n",
			     pkt->nccx_type);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tNack CCX state: 0x%x ",
			     pkt->nccx_ccx_state1);

	for (i = 0; i < 15; i++) {
		if (i)
			XDP2_PTH_LOC_PRINTFC(ctrl, " ");
		XDP2_PTH_LOC_PRINTFC(ctrl, "%02x", pkt->nack_ccx_state2[i]);
	}

	XDP2_PTH_LOC_PRINTFC(ctrl, "\n");

	return 0;
}

/* Print common Control info */
static void print_pds_common_control(const struct uet_pds_control_pkt *pkt,
				     const struct xdp2_ctrl_data *ctrl)
{
	__u8 ctl_type = uet_pds_control_pkt_get_ctl_type(pkt);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tControl type: %s (%u)\n",
			     uet_pds_control_pkt_type_to_text(ctl_type),
			     ctl_type);

	if (pkt->rsvd_isrod) {
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPDC is ROD\n");
	} else {
		if (ctl_type == UET_PDS_CTL_TYPE_NOOP ||
		    ctl_type == UET_PDS_CTL_TYPE_NEGOTIATION)
			XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPDC is RUD for NOOP "
						   "and Negotiation\n");
		else
			XDP2_PTH_LOC_PRINTFC(ctrl,
					"\t\tRsvd_isrod for delivery mode\n");
	}

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRetransmission: %s\n",
			     pkt->retrans ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tAck requested: %s\n",
			     pkt->ackreq ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSYN: %s\n",
			     pkt->syn ? "yes" : "no");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tProbe opaque: %u (0x%x)\n",
			     ntohs(pkt->probe_opaque),
			     ntohs(pkt->probe_opaque));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPSN: %u (0x%x)\n",
			     ntohl(pkt->psn), ntohl(pkt->psn));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tSPDcid: %u (0x%x)\n",
			     ntohs(pkt->spdcid), ntohs(pkt->spdcid));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPayload: %u (0x%x)\n",
			     ntohl(pkt->payload), ntohl(pkt->payload));
}

/* PDS Control handler */
static int handler_pds_control(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *metadata,
			       void *frame, const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_control_pkt *pkt = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS Control\n");

	if (verbose < 10)
		return 0;

	print_pds_common_control(pkt, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- No SYN\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tDPDcid: %u (0x%x)\n",
			     ntohs(pkt->dpdcid), ntohs(pkt->dpdcid));

	return 0;
}

/* PDS Control handler with SYN */
static int handler_pds_control_syn(const void *hdr, size_t hdr_len,
				   size_t hdr_off, void *metadata,
				   void *frame,
				   const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_control_pkt *pkt = hdr;
	__u16 psn_offset = uet_pds_control_pkt_get_psn_offset(pkt);

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS Control SYN\n");

	if (verbose < 10)
		return 0;

	print_pds_common_control(pkt, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- SYN\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tUse reserved PDC: %s\n",
			     pkt->use_rsv_pdc ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPSN offset: %u (0x%x)\n",
			     psn_offset, psn_offset);

	return 0;
}

/* PDS UUD request handler */
static int handler_pds_uud_req(const void *hdr, size_t hdr_len,
			       size_t hdr_off, void *metadata,
			       void *frame,
			       const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS UUD request\n");

	return 0;
}

/* Print common RUDI request or response info */
static void print_common_rudi_req_resp(
		const struct uet_pds_rudi_req_resp *rhdr,
		const struct xdp2_ctrl_data *ctrl)
{
	__u8 next_hdr = uet_pds_common_get_next_hdr(rhdr);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tType: %s (%u)\n",
			     uet_pkt_type_to_text(rhdr->type), rhdr->type);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tNext header: %s (%u)\n",
			     uet_next_header_type_to_text(next_hdr), next_hdr);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tECN marked: %s\n",
			     rhdr->ecn_marked ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRetransmission: %s\n",
			     rhdr->retrans ? "yes" : "no");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPacket ID: %u (0x%x)\n",
			     ntohl(rhdr->pkt_id), ntohl(rhdr->pkt_id));
}

/* PDS RUDI request handler */
static int handler_pds_rudi_req(const void *hdr, size_t hdr_len,
				size_t hdr_off, void *metadata,
				void *frame,
				const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_rudi_req_resp *rhdr = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS RUDI request\n");

	if (verbose < 10)
		return 0;

	print_common_rudi_req_resp(rhdr, ctrl);

	return 0;
}

/* PDS RUDI response handler */
static int handler_pds_rudi_resp(const void *hdr, size_t hdr_len,
				 size_t hdr_off, void *metadata,
				 void *frame,
				 const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_pds_rudi_req_resp *rhdr = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET PDS RUDI response\n");

	if (verbose < 10)
		return 0;

	print_common_rudi_req_resp(rhdr, ctrl);

	return 0;
}

/* Parse node entry point for UET */
XDP2_MAKE_PARSE_NODE(uet_base_node, uet_pds_parse_packet_type_ov,
		     pds_packet_type_table, ());

XDP2_MAKE_PARSE_NODE(pds_ack_cc_base_node, uet_pds_parse_ack_cc_ov,
		     pds_ack_cc_type_table, ());

/* PDS parse nodes */
XDP2_MAKE_PARSE_NODE(pds_ack_cc_nscc,
		     uet_pds_parse_ack_cc_nscc,
		     pds_next_hdr_response_table,
		     (.ops.handler = handler_pds_ack_cc_nscc));
XDP2_MAKE_PARSE_NODE(pds_ack_cc_credit,
		     uet_pds_parse_ack_cc_credit,
		     pds_next_hdr_response_table,
		     (.ops.handler = handler_pds_ack_cc_credit));
XDP2_MAKE_PARSE_NODE(pds_rud_request,
		     uet_pds_parse_rud_rod_request,
		     pds_next_hdr_request_table,
		     (.ops.handler = handler_pds_rud_request));
XDP2_MAKE_PARSE_NODE(pds_rod_request,
		     uet_pds_parse_rud_rod_request,
		     pds_next_hdr_request_table,
		     (.ops.handler = handler_pds_rod_request));
XDP2_MAKE_PARSE_NODE(pds_rud_request_cc,
		    uet_pds_parse_rud_rod_request_cc,
		    pds_next_hdr_request_table,
		    (.ops.handler = handler_pds_rud_request_cc));
XDP2_MAKE_PARSE_NODE(pds_rod_request_cc,
		     uet_pds_parse_rud_rod_request_cc,
		     pds_next_hdr_request_table,
		     (.ops.handler = handler_pds_rod_request_cc));
XDP2_MAKE_PARSE_NODE(pds_rud_request_syn,
		     uet_pds_parse_rud_rod_request,
		     pds_next_hdr_request_table,
		     (.ops.handler = handler_pds_rud_request_syn));
XDP2_MAKE_PARSE_NODE(pds_rod_request_syn,
		     uet_pds_parse_rud_rod_request,
		     pds_next_hdr_request_table,
		     (.ops.handler = handler_pds_rod_request_syn));
XDP2_MAKE_PARSE_NODE(pds_rud_request_cc_syn,
		     uet_pds_parse_rud_rod_request_cc,
		     pds_next_hdr_request_table,
		     (.ops.handler = handler_pds_rud_request_cc_syn));
XDP2_MAKE_PARSE_NODE(pds_rod_request_cc_syn,
		     uet_pds_parse_rud_rod_request_cc,
		     pds_next_hdr_request_table,
		     (.ops.handler = handler_pds_rod_request_cc_syn));
XDP2_MAKE_PARSE_NODE(pds_ack,
		     uet_pds_parse_ack,
		     pds_next_hdr_response_table,
		     (.ops.handler = handler_pds_ack));
XDP2_MAKE_PARSE_NODE(pds_ack_ccx,
		     uet_pds_parse_ack_ccx,
		     pds_next_hdr_response_table,
		     (.ops.handler = handler_pds_ack_ccx));
XDP2_MAKE_PARSE_NODE(pds_nack,
		     uet_pds_parse_nack,
		     pds_next_hdr_response_table,
		     (.ops.handler = handler_pds_nack));
XDP2_MAKE_PARSE_NODE(pds_nack_ccx,
		     uet_pds_parse_nack_ccx,
		     pds_next_hdr_response_table,
		     (.ops.handler = handler_pds_nack_ccx));
XDP2_MAKE_LEAF_PARSE_NODE(pds_control,
			  uet_pds_parse_control_pkt,
			  (.ops.handler = handler_pds_control));
XDP2_MAKE_LEAF_PARSE_NODE(pds_control_syn,
			  uet_pds_parse_control_pkt,
			  (.ops.handler = handler_pds_control_syn));
XDP2_MAKE_PARSE_NODE(pds_uud_req,
		     uet_pds_parse_uud_req,
		     pds_next_hdr_request_table,
		     (.ops.handler = handler_pds_uud_req));
XDP2_MAKE_PARSE_NODE(pds_rudi_req,
		     uet_pds_parse_rudi_req_resp,
		     pds_next_hdr_request_table,
		     (.ops.handler = handler_pds_rudi_req));
XDP2_MAKE_PARSE_NODE(pds_rudi_resp,
		     uet_pds_parse_rudi_req_resp,
		     pds_next_hdr_response_table,
		     (.ops.handler = handler_pds_rudi_resp));

XDP2_MAKE_PROTO_TABLE(pds_ack_cc_type_table,
	( UET_PDS_CC_TYPE_NSCC, pds_ack_cc_nscc),
	( UET_PDS_CC_TYPE_CREDIT, pds_ack_cc_credit)
);

#define MATCH_ONE(M, F)							\
	( __cpu_to_be16((M) << 11), F )

#define MATCH_PAIR_SYN(M, F, F_SYN)					\
	( __cpu_to_be16((M) << 11), F ),				\
	( __cpu_to_be16(((M) << 11) | (1 << 2)), F_SYN )

#define MATCH_PAIR(M, F)						\
	( __cpu_to_be16((M) << 11), F ),				\
	( __cpu_to_be16(((M) << 11) | (1 << 2)), F )			\

	/* PDS packet type table */
XDP2_MAKE_PROTO_TABLE(pds_packet_type_table,
	/* UET_PDS_TYPE_TSS is TBD */
	MATCH_PAIR_SYN(UET_PDS_TYPE_RUD_REQ, pds_rud_request,
		       pds_rud_request_syn),
	MATCH_PAIR_SYN(UET_PDS_TYPE_ROD_REQ, pds_rod_request,
		       pds_rod_request_syn),
	MATCH_PAIR_SYN(UET_PDS_TYPE_RUD_CC_REQ, pds_rud_request_cc,
		       pds_rud_request_cc_syn),
	MATCH_PAIR_SYN(UET_PDS_TYPE_ROD_CC_REQ, pds_rod_request_cc,
		       pds_rod_request_cc_syn),
	MATCH_PAIR_SYN(UET_PDS_TYPE_CONTROL, pds_control,
		       pds_control_syn),
	MATCH_PAIR_SYN(UET_PDS_TYPE_RUDI_REQ, pds_rudi_req,
		       pds_rudi_req),

	MATCH_PAIR(UET_PDS_TYPE_ACK, pds_ack),
	MATCH_PAIR(UET_PDS_TYPE_ACK_CC, pds_ack_cc_base_node),
	MATCH_PAIR(UET_PDS_TYPE_ACK_CCX, pds_ack_ccx),
	MATCH_PAIR(UET_PDS_TYPE_NACK, pds_nack),
	MATCH_PAIR(UET_PDS_TYPE_NACK_CCX, pds_nack_ccx),
	MATCH_PAIR(UET_PDS_TYPE_RUDI_RESP, pds_rudi_resp),
	MATCH_ONE(UET_PDS_TYPE_UUD_REQ, pds_uud_req)
);

/************************ SES ****************************/

/* SES standard headers */

/* Print common standard header */
static void print_ses_common_hdr(const struct uet_ses_common_hdr *chdr,
				 const struct xdp2_ctrl_data *ctrl)
{
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tVersion: %u\n", chdr->version);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tOpcode: %s (%u)\n",
			     uet_ses_request_msg_type_to_text(chdr->opcode),
			     chdr->opcode);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tDelivery complete: %s\n",
			     chdr->delivery_complete ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tInitiator error: %s\n",
			     chdr->initiator_error ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRelative addressing: %s\n",
			     chdr->relative_addressing ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tHeader data present: %s\n",
			     chdr->hdr_data_present ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tEnd of message: %s\n",
			     chdr->end_of_msg ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tStart of message: %s\n",
			     chdr->start_of_msg ? "yes" : "no");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMessage ID: %u (0x%x)\n",
			     ntohs(chdr->message_id), ntohs(chdr->message_id));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRI generation: %u (0x%x)\n",
			     chdr->ri_generation, chdr->ri_generation);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tJob ID: %u (0x%x)\n",
			     xdp2_ntohl24(chdr->job_id),
			     xdp2_ntohl24(chdr->job_id));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPID on FEP: %u (0x%x)\n",
			     uet_ses_common_get_pid_on_fep(chdr),
			     uet_ses_common_get_pid_on_fep(chdr));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tResource index: %u (0x%x)\n",
			     uet_ses_common_get_resource_index(chdr),
			     uet_ses_common_get_resource_index(chdr));
}

/* NO-OP standard size handler */
static int handler_uet_ses_no_op_std(const void *hdr, size_t hdr_len,
				     size_t hdr_off, void *metadata,
				     void *frame,
				     const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES NO-OP std\n");

	return 0;
}

/* Common print non start of message header */
static void print_ses_common_std_nosom_hdr(
		const struct uet_ses_request_std_hdr *shdr,
		const struct xdp2_ctrl_data *ctrl)
{
	print_ses_common_hdr(&shdr->cmn, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- No SOM\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tBuffer offset: %llu (0x%llx)\n",
			     ntohll(shdr->buffer_offset),
			     ntohll(shdr->buffer_offset));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tInitiator: %u (0x%x)\n",
			     ntohl(shdr->initiator),
			     ntohl(shdr->initiator));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMemory key/match bits: %llu (0x%llx)\n",
			     ntohll(shdr->memory_key),
			     ntohll(shdr->memory_key));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPayload length: %u (0x%x)\n",
			     uet_ses_request_std_hdr_get_payload_length(shdr),
			     uet_ses_request_std_hdr_get_payload_length(shdr));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMessage offset: %u (0x%x)\n",
			     ntohl(shdr->buffer_offset),
			     ntohl(shdr->buffer_offset));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRequest length: %u (0x%x)\n",
			     ntohl(shdr->request_length),
			     ntohl(shdr->request_length));
}

/* Common print start of standard message header */
static void print_ses_common_std_som_hdr(
		const struct uet_ses_request_std_hdr *shdr,
		const struct xdp2_ctrl_data *ctrl)
{
	__u16 payload_length = uet_ses_request_std_hdr_get_payload_length(shdr);

	print_ses_common_hdr(&shdr->cmn, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- SOM\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tBuffer offset: %llu (0x%llx)\n",
			     ntohll(shdr->buffer_offset),
			     ntohll(shdr->buffer_offset));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tInitiator: %u (0x%x)\n",
			     ntohl(shdr->initiator), ntohl(shdr->initiator));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMemory key/match bits: %llu (0x%llx)\n",
			     ntohll(shdr->memory_key),
			     ntohll(shdr->memory_key));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPayload length: %u (0x%x)\n",
			     payload_length, payload_length);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMessage offset: %u (0x%x)\n",
			     ntohl(shdr->buffer_offset),
			     ntohl(shdr->buffer_offset));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRequest length: %u (0x%x)\n",
			     ntohl(shdr->request_length),
			     ntohl(shdr->request_length));
}

/* Common handler for non start of standard message header */
static int handler_uet_ses_request_nosom_std(const void *hdr,
					     const struct xdp2_ctrl_data *ctrl,
					     const char *label)
{
	const struct uet_ses_request_std_hdr *shdr = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES %s std no SOM\n", label);

	if (verbose < 10)
		return 0;

	print_ses_common_std_nosom_hdr(shdr, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- %s\n", label);


	return 0;
}

/* Common handler for start of standard message header */
static int handler_uet_ses_request_som_std(const void *hdr,
					   const struct xdp2_ctrl_data *ctrl,
					   const char *label)
{
	const struct uet_ses_request_std_hdr *shdr = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES %s std SOM\n", label);

	if (verbose < 10)
		return 0;

	print_ses_common_std_som_hdr(shdr, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- %s\n", label);


	return 0;
}

/* Macros to generate parse nodes for standard requests. Tgwo node
 * variants are created: nosom and som
 */
#define __MAKE_REQUEST_STD1(NAME, LABEL)				\
static int handler_uet_ses_request_nosom_##NAME##_std(			\
		const void *hdr, size_t hdr_len,			\
		size_t hdr_off, void *metadata,				\
		void *frame,						\
		const struct xdp2_ctrl_data *ctrl)			\
{									\
	handler_uet_ses_request_nosom_std(hdr, ctrl, LABEL);		\
									\
	return 0;							\
}									\
									\
static int handler_uet_ses_request_som_##NAME##_std(			\
		const void *hdr, size_t hdr_len,			\
		size_t hdr_off, void *metadata,				\
		void *frame,						\
		const struct xdp2_ctrl_data *ctrl)			\
{									\
	handler_uet_ses_request_som_std(hdr, ctrl, LABEL);		\
									\
	return 0;							\
}									\
									\
XDP2_MAKE_PARSE_NODE(uet_ses_request_##NAME##_std,			\
		     uet_ses_parse_std_message_som_base_hdr_ov,		\
		     uet_ses_msg_##NAME##_std_table, ())

#define __MAKE_REQUEST_STD2(NAME)					\
XDP2_MAKE_PROTO_TABLE(uet_ses_msg_##NAME##_std_table,			\
	( 0, uet_ses_request_nosom_##NAME##_std ),			\
	( 1, uet_ses_request_som_##NAME##_std )				\
)

/* Make a leaf parse node for a standard request */
#define MAKE_REQUEST_LEAF_STD(NAME, LABEL)				\
__MAKE_REQUEST_STD1(NAME, LABEL);					\
XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_request_nosom_##NAME##_std,		\
		uet_ses_parse_request_std_hdr,				\
		(.ops.handler =						\
			handler_uet_ses_request_nosom_##NAME##_std));	\
XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_request_som_##NAME##_std,		\
		uet_ses_parse_request_std_hdr,				\
		(.ops.handler =						\
			handler_uet_ses_request_som_##NAME##_std));	\
__MAKE_REQUEST_STD2(NAME)

/* Make standard request nodes */
MAKE_REQUEST_LEAF_STD(read, "Read");
MAKE_REQUEST_LEAF_STD(write, "Write");
MAKE_REQUEST_LEAF_STD(send, "Send");
MAKE_REQUEST_LEAF_STD(datagram_send, "Datagram send");
MAKE_REQUEST_LEAF_STD(tagged_send, "Tagged send");
MAKE_REQUEST_LEAF_STD(tsend_atomic, "Tagged send atomic");
MAKE_REQUEST_LEAF_STD(tsend_fetch_atomic, "Tagged send fetch atomic");
MAKE_REQUEST_LEAF_STD(error, "Error");

/* Make an autonext parse node for a standard request */
#define MAKE_REQUEST_AUTONEXT_STD(SOM_NEXT_NODE, NOSOM_NEXT_NODE,	\
				NAME, LABEL)				\
__MAKE_REQUEST_STD1(NAME, LABEL);					\
XDP2_MAKE_AUTONEXT_PARSE_NODE(uet_ses_request_nosom_##NAME##_std,	\
		uet_ses_parse_request_std_hdr,				\
		SOM_NEXT_NODE,						\
		(.ops.handler =						\
			handler_uet_ses_request_nosom_##NAME##_std));	\
XDP2_MAKE_AUTONEXT_PARSE_NODE(uet_ses_request_som_##NAME##_std,		\
		uet_ses_parse_request_std_hdr,				\
		NOSOM_NEXT_NODE,					\
		(.ops.handler =						\
			handler_uet_ses_request_som_##NAME##_std));	\
__MAKE_REQUEST_STD2(NAME)

XDP2_MAKE_PARSE_NODE(atomic_switch_node, uet_ses_parse_atomic_op_ov,
		     uet_ses_amo_table, ());

MAKE_REQUEST_AUTONEXT_STD(atomic_switch_node, atomic_switch_node,
			  atomic, "Atomic");
MAKE_REQUEST_AUTONEXT_STD(atomic_switch_node, atomic_switch_node,
			  fetching_atomic, "Fetching atomic");

/* Common print deferrable standard message header */
static void print_ses_common_deferable_std(
		const struct uet_ses_defer_send_std_hdr *dhdr,
		const struct xdp2_ctrl_data *ctrl)
{
	print_ses_common_hdr(&dhdr->cmn, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tInitiator restart token: %u (0x%x)\n",
			     ntohl(dhdr->initiator_restart_token),
			     ntohl(dhdr->initiator_restart_token));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tTarget restart token: %u (0x%x)\n",
			     ntohl(dhdr->target_restart_token),
			     ntohl(dhdr->target_restart_token));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tInitiator: %u (0x%x)\n",
			     ntohl(dhdr->initiator), ntohl(dhdr->initiator));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMatch bits: %llu (0x%llx)\n",
			     ntohll(dhdr->match_bits),
			     ntohll(dhdr->match_bits));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tHeader data: %llu (0x%llx)\n",
			     ntohll(dhdr->header_data),
			     ntohll(dhdr->header_data));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRequest length: %u (0x%x)\n",
			     ntohl(dhdr->request_length),
			     ntohl(dhdr->request_length));
}

/* SES deferrable standard send handler */
static int handler_uet_ses_request_deferrable_send_std(
		const void *hdr, size_t hdr_len, size_t hdr_off,
		void *metadata, void *frame, const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_ses_defer_send_std_hdr *dhdr = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES deferred send\n");

	if (verbose < 10)
		return 0;

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- deferrable send\n");

	print_ses_common_deferable_std(dhdr, ctrl);

	return 0;
}

/* SES deferrable standard tagged send handler */
static int handler_uet_ses_request_deferrable_tsend_std(
		const void *hdr, size_t hdr_len, size_t hdr_off,
		void *metadata, void *frame, const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_ses_defer_send_std_hdr *dhdr = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES deferred tsend\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- deferrable tsend\n");

	print_ses_common_deferable_std(dhdr, ctrl);

	return 0;
}

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_request_deferrable_send_std,
		uet_ses_parse_defer_send_hdr,
		(.ops.handler = handler_uet_ses_request_deferrable_send_std));

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_request_deferrable_tsend_std,
		uet_ses_parse_defer_send_hdr,
		(.ops.handler = handler_uet_ses_request_deferrable_tsend_std));

/* Ready to restart handler */
static int handler_uet_ses_request_ready_restart(
		const void *hdr, size_t hdr_len, size_t hdr_off,
		void *metadata, void *frame, const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_ses_ready_to_restart_std_hdr *rhdr = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES ready to restart\n");

	if (verbose < 10)
		return 0;

	print_ses_common_hdr(&rhdr->cmn, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- Restart\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tBuffer offset: %llu (0x%llx)\n",
			     ntohll(rhdr->buffer_offset),
			     ntohll(rhdr->buffer_offset));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tInitiator: %u (0x%x)\n",
			     ntohl(rhdr->initiator),
			     ntohl(rhdr->initiator));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tInitiator restart token: %u (0x%x)\n",
			     ntohl(rhdr->initiator_restart_token),
			     ntohl(rhdr->initiator_restart_token));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tTarget restart token: %u (0x%x)\n",
			     ntohl(rhdr->target_restart_token),
			     ntohl(rhdr->target_restart_token));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tHeader data: %llu (0x%llx)\n",
			     ntohll(rhdr->header_data),
			     ntohll(rhdr->header_data));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRequest length: %u (0x%x)\n",
			     ntohl(rhdr->request_length),
			     ntohl(rhdr->request_length));

	return 0;
}

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_request_ready_restart_std,
		uet_ses_parse_ready_to_restart_req_hdr,
		(.ops.handler = handler_uet_ses_request_ready_restart));

/* Rendezvous send handler */
static int handler_uet_request_rendezvous_send_std(
		const void *hdr, size_t hdr_len, size_t hdr_off,
		void *metadata, void *frame, const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_ses_rendezvous_std_hdr *rhdr = hdr;
	const struct uet_ses_rendezvous_ext_hdr *reh = &rhdr->ext_hdr;
	unsigned int pid_on_fep =
		uet_ses_rendezvous_ext_hdr_get_pid_on_fep(reh);
	unsigned int resource_index =
		uet_ses_rendezvous_ext_hdr_get_resource_index(reh);

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES redezvous send\n");

	if (verbose < 10)
		return 0;

	print_ses_common_hdr(&rhdr->cmn, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- Rendezvous\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tBuffer offset: %llu (0x%llx)\n",
			     ntohll(rhdr->buffer_offset),
			     ntohll(rhdr->buffer_offset));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tInitiator: %u (0x%x)\n",
			     ntohl(rhdr->initiator),
			     ntohl(rhdr->initiator));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMatch bits: %llu (0x%llx)\n",
			     ntohll(rhdr->match_bits),
			     ntohll(rhdr->match_bits));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\t--- Rendezvous ext hdr\n");

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tEager length: %u (0x%x)\n",
			     ntohl(reh->eager_length),
			     ntohl(reh->eager_length));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRI generation: %u (0x%x)\n",
			     reh->ri_generation, reh->ri_generation);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPID on FEP: %u (0x%x)\n",
			     pid_on_fep, pid_on_fep);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tResource index: %u (0x%x)\n",
			     resource_index, resource_index);

	return 0;
}

/* Rendezvous tagged send handler */
static int handler_uet_request_rendezvous_tsend_std(
		const void *hdr, size_t hdr_len, size_t hdr_off,
		void *metadata, void *frame, const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_ses_ready_to_restart_std_hdr *rhdr = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES redezvous tsend\n");

	if (verbose < 10)
		return 0;

	print_ses_common_hdr(&rhdr->cmn, ctrl);

	return 0;
}

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_request_rendezvous_send_std,
		uet_ses_parse_rendezvous_hdr,
		(.ops.handler =
			handler_uet_request_rendezvous_send_std));

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_request_rendezvous_tsend_std,
		uet_ses_parse_rendezvous_hdr,
		(.ops.handler =
			handler_uet_request_rendezvous_tsend_std));

/* Print common atomic */
static void print_ses_common_atomic(
		const struct uet_ses_atomic_op_ext_hdr *ehdr,
		const struct xdp2_ctrl_data *ctrl)
{
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tAtomic code: %s (%u)\n",
			     uet_ses_amo_to_text(ehdr->atomic_code),
			     ehdr->atomic_code);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tAtomic datatype: %s (%u)\n",
			     uet_ses_amo_datatype_to_text(
							ehdr->atomic_datatype),
							ehdr->atomic_datatype);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tCacheable: %s\n",
			     ehdr->ctrl_cacheable ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tCPU coherent: %s\n",
			     ehdr->cpu_coherent ? "yes" : "no");
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tVendor defined: %u\n",
			     ehdr->vendor_defined);
}

/* Atomic extension header handler */
static int handler_uet_ses_atomic(
		const void *hdr, size_t hdr_len, size_t hdr_off,
		void *metadata, void *frame, const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_ses_atomic_op_ext_hdr *ext_hdr = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES atomic operation\n");

	if (verbose < 10)
		return 0;

	XDP2_PTH_LOC_PRINTFC(ctrl, "\tAtomic extension header\n");

	print_ses_common_atomic(ext_hdr, ctrl);

	return 0;
}


/* Compare and swap atomic extension header handler */
static int handler_uet_ses_atomic_cmp_swp(
		const void *hdr, size_t hdr_len, size_t hdr_off,
		void *metadata, void *frame, const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_ses_atomic_cmp_and_swap_ext_hdr *ext_hdr = hdr;
	int i;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl,
				     "\tUET SES atomic cmpswap operation\n");

	if (verbose < 10)
		return 0;

	print_ses_common_atomic(&ext_hdr->cmn, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tCompare value: ");

	for (i = 0; i < 16; i++) {
		if (i)
			XDP2_PTH_LOC_PRINTFC(ctrl, " ");

		XDP2_PTH_LOC_PRINTFC(ctrl, "%02x", ext_hdr->compare_value[i]);
	}

	XDP2_PTH_LOC_PRINTFC(ctrl, "\n\t\tSwap value: ");
	for (i = 0; i < 16; i++) {
		if (i)
			XDP2_PTH_LOC_PRINTFC(ctrl, " ");

		XDP2_PTH_LOC_PRINTFC(ctrl, "%02x", ext_hdr->swap_value[i]);
	}

	XDP2_PTH_LOC_PRINTFC(ctrl, "\n");

	return 0;
}

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_atomic_node, uet_ses_parse_atomic_ext_hdr,
			  (.ops.handler = handler_uet_ses_atomic));

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_atomic_cmp_swp_node,
			  uet_ses_parse_atomic_cmp_and_swap_ext_hdr,
			  (.ops.handler = handler_uet_ses_atomic_cmp_swp));

XDP2_MAKE_PROTO_TABLE(uet_ses_amo_table,
	( UET_SES_AMO_MIN, uet_ses_atomic_node),
	( UET_SES_AMO_MAX, uet_ses_atomic_node),
	( UET_SES_AMO_SUM, uet_ses_atomic_node),
	( UET_SES_AMO_DIFF, uet_ses_atomic_node),
	( UET_SES_AMO_PROD, uet_ses_atomic_node),
	( UET_SES_AMO_LOR, uet_ses_atomic_node),
	( UET_SES_AMO_LAND, uet_ses_atomic_node),
	( UET_SES_AMO_BOR, uet_ses_atomic_node),
	( UET_SES_AMO_BAND, uet_ses_atomic_node),
	( UET_SES_AMO_LXOR, uet_ses_atomic_node),
	( UET_SES_AMO_BXOR, uet_ses_atomic_node),
	( UET_SES_AMO_READ, uet_ses_atomic_node),
	( UET_SES_AMO_WRITE, uet_ses_atomic_node),
	( UET_SES_AMO_CSWAP, uet_ses_atomic_cmp_swp_node),
	( UET_SES_AMO_CSWAP_NE, uet_ses_atomic_cmp_swp_node),
	( UET_SES_AMO_CSWAP_LE, uet_ses_atomic_cmp_swp_node),
	( UET_SES_AMO_CSWAP_LT, uet_ses_atomic_cmp_swp_node),
	( UET_SES_AMO_CSWAP_GE, uet_ses_atomic_cmp_swp_node),
	( UET_SES_AMO_CSWAP_GT, uet_ses_atomic_cmp_swp_node),
	( UET_SES_AMO_MSWAP, uet_ses_atomic_node),
	( UET_SES_AMO_INVAL, uet_ses_atomic_node)
);

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_no_op_std,
			  xdp2_parse_null,
			  (.ops.handler = handler_uet_ses_no_op_std));

XDP2_MAKE_PROTO_TABLE(pds_hdr_request_std_table,
	( UET_SES_REQUEST_MSG_NO_OP, uet_ses_no_op_std ),
	( UET_SES_REQUEST_MSG_WRITE, uet_ses_request_write_std ),
	( UET_SES_REQUEST_MSG_READ, uet_ses_request_read_std ),
	( UET_SES_REQUEST_MSG_ATOMIC, uet_ses_request_atomic_std ),
	( UET_SES_REQUEST_MSG_FETCHING_ATOMIC,
	  uet_ses_request_fetching_atomic_std ),
	( UET_SES_REQUEST_MSG_SEND, uet_ses_request_send_std ),
	( UET_SES_REQUEST_MSG_RENDEZVOUS_SEND,
	  uet_ses_request_rendezvous_send_std ),
	( UET_SES_REQUEST_MSG_DATAGRAM_SEND,
	  uet_ses_request_datagram_send_std ),
	( UET_SES_REQUEST_MSG_DEFERRABLE_SEND,
	  uet_ses_request_deferrable_send_std ),
	( UET_SES_REQUEST_MSG_TAGGED_SEND, uet_ses_request_tagged_send_std ),
	( UET_SES_REQUEST_MSG_RENDEZVOUS_TSEND,
	  uet_ses_request_rendezvous_tsend_std ),
	( UET_SES_REQUEST_MSG_DEFERRABLE_TSEND,
	  uet_ses_request_deferrable_tsend_std ),
	( UET_SES_REQUEST_MSG_DEFERRABLE_RTR,
	  uet_ses_request_ready_restart_std ),
	( UET_SES_REQUEST_MSG_TSEND_ATOMIC, uet_ses_request_tsend_atomic_std ),
	( UET_SES_REQUEST_MSG_TSEND_FETCH_ATOMIC,
	  uet_ses_request_tsend_fetch_atomic_std ),
	( UET_SES_REQUEST_MSG_MSG_ERROR, uet_ses_request_error_std )
);

/* SES medium headers */

/* Common print medium request message */
static void handler_ses_request_medium(
		const struct uet_ses_request_medium_hdr *shdr,
		const struct xdp2_ctrl_data *ctrl,
		const char *label)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl,
				     "\tUET SES request medium %s\n", label);

	if (verbose < 10)
		return;

	print_ses_common_hdr(&shdr->cmn, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tHeader data: %llu (0x%llx)\n",
			     ntohll(shdr->header_data),
			     ntohll(shdr->header_data));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMatch bits: %llu (0x%llx)\n",
			     ntohll(shdr->match_bits),
			     ntohll(shdr->match_bits));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tInitiator: %u (0x%x)\n",
			     ntohl(shdr->initiator),
			     ntohl(shdr->initiator));
}

/* Medium no-op */
static int handler_uet_ses_msg_no_op_medium(
		const void *hdr, size_t hdr_len, size_t hdr_off,
		void *metadata, void *frame, const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES NO-OP medium\n");

	return 0;
}

#define MAKE_REQUEST_HANDLER_MEDIUM(NAME, LABEL)			\
static int handler_uet_ses_request_##NAME##_medium(			\
		const void *hdr, size_t hdr_len, size_t hdr_off,	\
		void *metadata, void *frame,				\
		const struct xdp2_ctrl_data *ctrl)			\
{									\
	handler_ses_request_medium(hdr, ctrl, LABEL);			\
									\
	return 0;							\
}

#define MAKE_REQUEST_MEDIUM(NAME, LABEL)				\
MAKE_REQUEST_HANDLER_MEDIUM(NAME, LABEL)				\
XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_request_##NAME##_medium,		\
			  uet_ses_parse_request_medium_hdr,		\
			  (.ops.handler =				\
				handler_uet_ses_request_##NAME##_medium))

#define MAKE_REQUEST_AUTONEXT_MEDIUM(NEXT_NODE, NAME, LABEL)		\
MAKE_REQUEST_HANDLER_MEDIUM(NAME, LABEL)				\
XDP2_MAKE_AUTONEXT_PARSE_NODE(uet_ses_request_##NAME##_medium,		\
		uet_ses_parse_request_medium_hdr,			\
		NEXT_NODE,						\
		(.ops.handler = handler_uet_ses_request_##NAME##_medium))

MAKE_REQUEST_MEDIUM(write, "Write");
MAKE_REQUEST_MEDIUM(read, "Read");
MAKE_REQUEST_MEDIUM(send, "Send");
MAKE_REQUEST_MEDIUM(tsend, "Tagged send");
MAKE_REQUEST_MEDIUM(datagram_send, "Datagram send");
MAKE_REQUEST_MEDIUM(tsend_atomic, "Tagged send atomic");
MAKE_REQUEST_MEDIUM(tsend_fetch_atomic, "Tagged send fecth atomic");

MAKE_REQUEST_AUTONEXT_MEDIUM(atomic_switch_node, atomic, "Atomic");
MAKE_REQUEST_AUTONEXT_MEDIUM(atomic_switch_node, fetching_atomic,
			     "Fetching atomic");

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_msg_no_op_medium,
			  xdp2_parse_null,
			  (.ops.handler = handler_uet_ses_msg_no_op_medium));

XDP2_MAKE_PROTO_TABLE(pds_hdr_request_medium_table,
	( UET_SES_REQUEST_MSG_NO_OP, uet_ses_msg_no_op_medium ),
	( UET_SES_REQUEST_MSG_WRITE, uet_ses_request_write_medium ),
	( UET_SES_REQUEST_MSG_READ, uet_ses_request_read_medium ),
	( UET_SES_REQUEST_MSG_SEND, uet_ses_request_send_medium ),
	( UET_SES_REQUEST_MSG_TAGGED_SEND,
	  uet_ses_request_tsend_medium ),
	( UET_SES_REQUEST_MSG_DATAGRAM_SEND,
	  uet_ses_request_datagram_send_medium ),
	( UET_SES_REQUEST_MSG_TSEND_ATOMIC,
	  uet_ses_request_tsend_atomic_medium ),
	( UET_SES_REQUEST_MSG_TSEND_FETCH_ATOMIC,
	  uet_ses_request_tsend_fetch_atomic_medium ),
	( UET_SES_REQUEST_MSG_ATOMIC, uet_ses_request_atomic_medium ),
	( UET_SES_REQUEST_MSG_FETCHING_ATOMIC,
	  uet_ses_request_fetching_atomic_medium )
);

/* SES small headers */

/* Common print small request message */
static void handler_ses_request_small(
		const struct uet_ses_request_small_hdr *shdr,
		const struct xdp2_ctrl_data *ctrl,
		const char *label)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES request small %s\n", label);

	if (verbose < 10)
		return;

	print_ses_common_hdr(&shdr->cmn, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tBuffer offset: %llu (0x%llx)\n",
			     ntohll(shdr->buffer_offset),
			     ntohll(shdr->buffer_offset));
}

#define MAKE_REQUEST_HANDLER_SMALL(NAME, LABEL)				\
static int handler_uet_ses_request_##NAME##_small(			\
		const void *hdr, size_t hdr_len, size_t hdr_off,	\
		void *metadata, void *frame,				\
		const struct xdp2_ctrl_data *ctrl)			\
{									\
	handler_ses_request_small(hdr, ctrl, LABEL);			\
									\
	return 0;							\
}

#define MAKE_REQUEST_SMALL(NAME, LABEL)					\
MAKE_REQUEST_HANDLER_SMALL(NAME, LABEL)					\
XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_request_##NAME##_small,		\
			  uet_ses_parse_request_small_hdr,		\
			  (.ops.handler =				\
				handler_uet_ses_request_##NAME##_small))

#define MAKE_REQUEST_AUTONEXT_SMALL(NEXT_NODE, NAME, LABEL)		\
MAKE_REQUEST_HANDLER_SMALL(NAME, LABEL)					\
XDP2_MAKE_AUTONEXT_PARSE_NODE(uet_ses_request_##NAME##_small,		\
	uet_ses_parse_request_small_hdr,				\
	NEXT_NODE,							\
	(.ops.handler = handler_uet_ses_request_##NAME##_small))

MAKE_REQUEST_SMALL(write, "Write");
MAKE_REQUEST_SMALL(read, "Read");

MAKE_REQUEST_AUTONEXT_SMALL(atomic_switch_node, atomic, "Atomic");
MAKE_REQUEST_AUTONEXT_SMALL(atomic_switch_node, fetching_atomic,
			    "Fetching atomic");

static int handler_uet_ses_msg_no_op_small(const void *hdr, size_t hdr_len,
					   size_t hdr_off, void *metadata,
					   void *frame,
					   const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES NO-OP small\n");

	return 0;
}

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_msg_no_op_small,
			  xdp2_parse_null,
			  (.ops.handler = handler_uet_ses_msg_no_op_small));

XDP2_MAKE_PROTO_TABLE(pds_hdr_request_small_table,
	( UET_SES_REQUEST_MSG_NO_OP, uet_ses_msg_no_op_small ),
	( UET_SES_REQUEST_MSG_WRITE, uet_ses_request_write_small ),
	( UET_SES_REQUEST_MSG_READ, uet_ses_request_read_small ),
	( UET_SES_REQUEST_MSG_ATOMIC, uet_ses_request_atomic_small ),
	( UET_SES_REQUEST_MSG_FETCHING_ATOMIC,
	  uet_ses_request_fetching_atomic_small)
);

/* SES no next header */
static int handler_uet_ses_no_next_hdr(const void *hdr, size_t hdr_len,
				       size_t hdr_off, void *metadata,
				       void *frame,
				       const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES no next header\n");

	return 0;
}

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_no_next_hdr, xdp2_parse_null,
			  (.ops.handler =  handler_uet_ses_no_next_hdr));

/* Master PDS table for mapping to std, medium, and small SES formats */

XDP2_MAKE_PARSE_NODE(uet_ses_request_small,
		     uet_ses_parse_common_hdr_ov,
		     pds_hdr_request_small_table, ());

XDP2_MAKE_PARSE_NODE(uet_ses_request_medium,
		     uet_ses_parse_common_hdr_ov,
		     pds_hdr_request_medium_table, ());

XDP2_MAKE_PARSE_NODE(uet_ses_request_std,
		     uet_ses_parse_common_hdr_ov,
		     pds_hdr_request_std_table, ());

XDP2_MAKE_PROTO_TABLE(pds_next_hdr_request_table,
	( UET_HDR_NONE, uet_ses_no_next_hdr),
	( UET_HDR_REQUEST_SMALL, uet_ses_request_small ),
	( UET_HDR_REQUEST_MEDIUM, uet_ses_request_medium ),
	( UET_HDR_REQUEST_STD, uet_ses_request_std )
);

/* Print common response */
static void print_ses_common_response(const struct uet_ses_common_response_hdr
								*resp,
				      const struct xdp2_ctrl_data *ctrl)
{
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPayload delivered to list: %s (%u)\n",
			     uet_ses_list_delivered_to_text(resp->list),
			     resp->list);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tOpcode: %s (%u)\n",
			     uet_ses_reponse_msg_type_to_text(resp->opcode),
			     resp->opcode);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tVersion: %x\n", resp->ver);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tReturn code: %s (%u)\n",
			     uet_ses_return_code_to_text(resp->return_code),
			     resp->return_code);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tMessage ID: %u (0x%x)\n",
			     ntohs(resp->message_id), ntohs(resp->message_id));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRI generation: %u (0x%x)\n",
			     resp->ri_generation, resp->ri_generation);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tJob ID: %u (0x%x)\n",
			     xdp2_ntohl24(resp->job_id),
			     xdp2_ntohl24(resp->job_id));
}

/* Handler for response with no data headers */
static void handler_uet_ses_nodata_response(const void *hdr,
					    const struct xdp2_ctrl_data *ctrl,
					    const char *label)
{
	const struct uet_ses_nodata_response_hdr *resp = hdr;

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES response %s\n", label);

	if (verbose < 10)
		return;

	print_ses_common_response(&resp->cmn, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tModified length: %u (0x%x)\n",
			     ntohl(resp->modified_length),
			     ntohl(resp->modified_length));
}

#define MAKE_RESPONSE(NAME, LABEL)					\
static int handler_uet_ses_##NAME##_nodata_response(			\
		const void *hdr, size_t hdr_len,			\
		size_t hdr_off, void *metadata,				\
		void *frame, const struct xdp2_ctrl_data *ctrl)		\
{									\
	handler_uet_ses_nodata_response(hdr, ctrl, LABEL);		\
									\
	return 0;							\
}									\
									\
XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_##NAME##_nodata_response,		\
			  uet_ses_parse_uet_ses_nodata_response_hdr,	\
			  (.ops.handler =				\
				handler_uet_ses_##NAME##_nodata_response))

MAKE_RESPONSE(normal, "Normal");
MAKE_RESPONSE(default, "Default");
MAKE_RESPONSE(none, "None");

XDP2_MAKE_PROTO_TABLE(uet_ses_response_nodata_table,
	( UET_SES_RESPONSE, uet_ses_normal_nodata_response ),
	( UET_SES_RESPONSE_DEFAULT, uet_ses_default_nodata_response ),
	( UET_SES_RESPONSE_NONE, uet_ses_none_nodata_response )
);

/* Handler for response with data */
static int handler_uet_ses_with_data_response(const void *hdr, size_t hdr_len,
					      size_t hdr_off, void *metadata,
					      void *frame,
					      const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_ses_with_data_response_hdr *resp = hdr;
	unsigned int payload_length =
		uet_ses_response_with_data_hdr_get_payload_length(resp);

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\tUET SES response with data\n");

	if (verbose < 10)
		return 0;

	print_ses_common_response(&resp->cmn, ctrl);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tRead request message ID: %u (%x)\n",
			     ntohs(resp->read_request_message_id),
			     ntohs(resp->read_request_message_id));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPayload length: %u (%x)\n",
			     payload_length, payload_length);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tModified length: %u (%x)\n",
			     ntohl(resp->modified_length),
			     ntohl(resp->modified_length));
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tModified length: %u (%x)\n",
			     ntohl(resp->message_offset),
			     ntohs(resp->message_offset));
	return 0;
}

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_response_with_data,
			  uet_ses_parse_uet_ses_with_data_response_hdr,
			  (.ops.handler =
				handler_uet_ses_with_data_response));

XDP2_MAKE_PROTO_TABLE(uet_ses_response_with_data_table,
	( UET_SES_RESPONSE_WITH_DATA, uet_ses_response_with_data )
);

/* Handler for response with small data */
static int handler_uet_ses_with_small_data_response(
		const void *hdr, size_t hdr_len, size_t hdr_off, void *metadata,
		void *frame, const struct xdp2_ctrl_data *ctrl)
{
	const struct uet_ses_with_small_data_response_hdr *resp = hdr;
	__u16 payload_length =
		uet_ses_with_small_data_response_hdr_get_payload_length(resp);

	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl,
				     "\tUET SES response with small data\n");

	if (verbose < 10)
		return 0;

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPayload delivered to list: %s (%u)\n",
			     uet_ses_list_delivered_to_text(resp->list),
			     resp->list);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tOpcode: %s (%u)\n",
			     uet_ses_reponse_msg_type_to_text(resp->opcode),
			     resp->opcode);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tVersion: %x\n", resp->ver);
	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tReturn code: %s (%u)\n",
			     uet_ses_return_code_to_text(resp->return_code),
			     resp->return_code);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tPayload length: %u (0x%x)\n",
			     payload_length, payload_length);

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tJob ID: %u (0x%x)\n",
			     xdp2_ntohl24(resp->job_id),
			     xdp2_ntohl24(resp->job_id));

	XDP2_PTH_LOC_PRINTFC(ctrl, "\t\tOriginal request PSN: %u (%x)\n",
			     ntohl(resp->original_request_psn),
			     ntohl(resp->original_request_psn));
	return 0;
}

XDP2_MAKE_LEAF_PARSE_NODE(uet_ses_response_with_small_data,
			  uet_ses_parse_uet_ses_with_small_data_response_hdr,
			  (.ops.handler =
				handler_uet_ses_with_small_data_response));

XDP2_MAKE_PROTO_TABLE(uet_ses_response_with_data_small_table,
	( UET_SES_RESPONSE_WITH_DATA, uet_ses_response_with_small_data )
);

/* Master PDS table for responses */

XDP2_MAKE_PARSE_NODE(uet_ses_response_tnode,
		     uet_ses_parse_common_hdr_ov,
		     uet_ses_response_nodata_table, ());

XDP2_MAKE_PARSE_NODE(uet_ses_response_with_data_tnode,
		     uet_ses_parse_common_hdr_ov,
		     uet_ses_response_with_data_table, ());

XDP2_MAKE_PARSE_NODE(uet_ses_response_with_data_small_tnode,
		     uet_ses_parse_common_hdr_ov,
		     uet_ses_response_with_data_small_table, ());

XDP2_MAKE_PROTO_TABLE(pds_next_hdr_response_table,
	( UET_HDR_NONE, uet_ses_no_next_hdr),
	( UET_HDR_RESPONSE, uet_ses_response_tnode ),
	( UET_HDR_RESPONSE_DATA, uet_ses_response_with_data_tnode ),
	( UET_HDR_RESPONSE_DATA_SMALL, uet_ses_response_with_data_small_tnode )
);

#endif /* __UET_PARSER_TEST_H__ */
