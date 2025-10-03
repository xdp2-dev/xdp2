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

/* Test of sending and receiving basic Falcon packet types */

#include <pthread.h>

#include "xdp2/utility.h"

/* Define UET parse nodes for XDP2 parser */
#define XDP2_DEFINE_PARSE_NODE
#include "uet/proto_uet_pds.h"
#include "uet/proto_uet_ses.h"
#undef XDP2_DEFINE_PARSE_NODE

#include "uet/config.h"
#include "uet/protocol.h"

#include "test.h"

static void uet_pds_fill_rud_rod_req(struct uet_pds_rud_rod_request *pds,
				     struct uet_pdc *pdc, bool syn,
				     enum uet_pds_pkt_type type,
				     enum uet_next_header_type nxt_hdr)
{
	pds->type = type;
	uet_pds_common_set_next_hdr(pds, nxt_hdr);

	pds->retrans = 1;
	pds->syn = syn;
	pds->ackreq = 1;

	pds->clear_psn_offset = 0x4321;
	pds->psn = 0x87654321;
	pds->spdcid = pdc->local_pdcid;

	if (syn) {
		pds->use_rsv_pdc = 1;
		uet_pds_rud_rod_request_set_psn_offset(pds, 0x123);
	} else {
		pds->dpdcid = pdc->remote_pdcid;
	}
}

static void uet_pds_fill_rud_rod_req_cc(struct uet_pds_rud_rod_request_cc *pds,
					struct uet_pdc *pdc, bool syn,
					enum uet_pds_pkt_type type,
					enum uet_next_header_type nxt_hdr)
{
	uet_pds_fill_rud_rod_req(&pds->req, pdc, syn, type, nxt_hdr);

	pds->req_cc_state.ccc_id = 0x78;
	pds->req_cc_state.credit_target = xdp2_htonl24(0x123456);
}

static void uet_pds_fill_ack(struct uet_pds_ack *pds, struct uet_pdc *pdc,
			     enum uet_pds_pkt_type type)

{
	pds->type = type;
	pds->ecn_marked = 1;
	pds->retrans = 1;
	pds->probe = 1;
	pds->request = UET_PDS_ACK_REQUEST_CLOSE;
	pds->ecn_marked = 1;

	pds->probe_opaque = htons(0x6543);
	pds->cack_psn = htonl(0x87654321);
	pds->spdcid = pdc->local_pdcid;
	pds->dpdcid = htons(0x8875);
}

static void uet_pds_fill_nack(struct uet_pds_nack *pds, struct uet_pdc *pdc,
			      enum uet_pds_pkt_type type,
			      enum uet_next_header_type nxt_hdr)
{
	pds->type = type;
	uet_pds_common_set_next_hdr(pds, nxt_hdr);
	pds->ecn_marked = 1;
	pds->retrans = 1;
	pds->nack_type = UET_PDS_NACK_TYPE_RUD_ROD;

	pds->nack_code = UET_PDS_NACK_CODE_EXP_NACK_ERR;
	pds->vendor_code = 0xdc;

	pds->nack_psn = htonl(0xfedcba98);

	pds->spdcid = pdc->local_pdcid;
	pds->dpdcid = htons(0x8875);

	pds->payload = htonl(0xdcba9876);
}

static void uet_pds_fill_rudi_req_resp(struct uet_pds_rudi_req_resp *pds,
				       struct uet_pdc *pdc, bool syn,
				       enum uet_pds_pkt_type type,
				       enum uet_next_header_type nxt_hdr)
{
	pds->type = type;
	uet_pds_common_set_next_hdr(pds, nxt_hdr);

	pds->ecn_marked = 1;
	pds->retrans = 1;

	pds->pkt_id = htonl(0x99887766);
}

static void uet_ses_fill_common(struct uet_ses_common_hdr *cmn,
				enum uet_ses_request_opcode opcode)
{
	cmn->version = 0;
	cmn->start_of_msg = 1;
	cmn->delivery_complete = 1;
	cmn->initiator_error = 0;
	cmn->hdr_data_present = 1;
	cmn->end_of_msg = 0;
	cmn->opcode = opcode;
	cmn->message_id = htons(1234);
	cmn->ri_generation = 234;
	cmn->job_id = xdp2_htonl24(567890);
	uet_ses_common_set_pid_on_fep(cmn, 0x432);
	uet_ses_common_set_resource_index(cmn, 0x765);
}

static void uet_ses_fill_common_resp(struct uet_ses_common_response_hdr *cmn,
				     enum uet_ses_response_type type)
{
	cmn->ver = 0;
	cmn->opcode = type;
	cmn->return_code = UET_SES_RETURN_CODE_HOST_UNSUCCESS_CMPL;

	cmn->message_id = htons(0x5566);
	cmn->ri_generation = 0x56;
	cmn->job_id = xdp2_htonl24(0x56789a);
}

static void uet_ses_fill_req_std(struct uet_ses_request_std_hdr *ses,
				 struct uet_pdc *pdc, __u8 opcode)
{
	uet_ses_fill_common(&ses->cmn, opcode);

	ses->buffer_offset = htonll(123456789012);
	ses->initiator = htonl(123456789);
	ses->match_bits = htonll(8675309ULL);
	uet_ses_request_std_hdr_set_payload_length(ses, 0x123);
	ses->message_offset = htonl(8675309);
	ses->request_length = htonl(111111111);
}

static void uet_send_one(struct uet_pdc *pdc, void *data, size_t n,
			 const char *text)
{
	struct uet_fep *fep = pdc->fep;
	size_t ret;

	ret = fep->comm->send_data(fep->comm, data, n, pdc->remote_fa,
				   pdc->remote_port);

	__uet_pdc_check_send(pdc, n, ret, text);
}

static void uet_send_req_read_std(struct uet_pdc *pdc, bool syn)
{
	struct {
		struct uet_pds_rud_rod_request_cc pds;
		struct uet_ses_request_std_hdr ses;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_fill_rud_rod_req_cc(&uet_hdr.pds, pdc, syn,
				 UET_PDS_TYPE_ROD_CC_REQ, UET_HDR_REQUEST_STD);

	uet_ses_fill_req_std(&uet_hdr.ses, pdc, UET_SES_REQUEST_OPCODE_READ);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr), "Read req std");
}

static void uet_send_req_cc_read_small(struct uet_pdc *pdc, bool syn)
{
	struct {
		struct uet_pds_rud_rod_request_cc pds;
		struct uet_ses_request_small_hdr ses;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_fill_rud_rod_req(&uet_hdr.pds.req, pdc, syn,
				 UET_PDS_TYPE_RUD_CC_REQ,
				 UET_HDR_REQUEST_SMALL);

	uet_hdr.pds.req_cc_state.ccc_id = 0xdc;
	uet_hdr.pds.req_cc_state.credit_target = xdp2_htonl24(0x89abcd);

	uet_ses_fill_common(&uet_hdr.ses.cmn,
			    UET_SES_REQUEST_OPCODE_READ);
	uet_hdr.ses.buffer_offset = htonll(0xabcdef9876543210);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr), "Send req CC read small");
}

static void uet_send_req_read_small(struct uet_pdc *pdc, bool syn)
{
	struct {
		struct uet_pds_rud_rod_request pds;
		struct uet_ses_request_small_hdr ses;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_fill_rud_rod_req(&uet_hdr.pds, pdc, syn,
				 UET_PDS_TYPE_ROD_REQ,
				 UET_HDR_REQUEST_SMALL);

	uet_ses_fill_common(&uet_hdr.ses.cmn,
			    UET_SES_REQUEST_OPCODE_READ);
	uet_hdr.ses.buffer_offset = htonll(0xabcdef9876543210);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr), "Send req read small");
}

static void uet_send_defer_send_std(struct uet_pdc *pdc, bool syn)
{
	struct {
		struct uet_pds_rud_rod_request pds;
		struct uet_ses_defer_send_std_hdr ses;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_fill_rud_rod_req(&uet_hdr.pds, pdc, syn,
				 UET_PDS_TYPE_RUD_REQ,
				 UET_HDR_REQUEST_STD);

	uet_ses_fill_common(&uet_hdr.ses.cmn,
			    UET_SES_REQUEST_OPCODE_DEFERRABLE_SEND);

	uet_hdr.ses.initiator_restart_token = htonl(0x98765432);
	uet_hdr.ses.target_restart_token = htonl(0xba987654);
	uet_hdr.ses.initiator = htonl(0x98765432);
	uet_hdr.ses.match_bits = htonll(0x123456789abcdef);
	uet_hdr.ses.header_data = htonll(0x3456789abcdef01);
	uet_hdr.ses.request_length = htonl(0xfedcba98);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr), "Send defer send std");
}

static void uet_send_req_write_medium(struct uet_pdc *pdc, bool syn)
{
	struct {
		struct uet_pds_rud_rod_request pds;
		struct uet_ses_request_medium_hdr ses;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_fill_rud_rod_req(&uet_hdr.pds, pdc, syn,
				 UET_PDS_TYPE_ROD_REQ,
				 UET_HDR_REQUEST_MEDIUM);

	uet_ses_fill_common(&uet_hdr.ses.cmn,
			    UET_SES_REQUEST_OPCODE_WRITE);

	uet_hdr.ses.buffer_offset = htonll(0xabcdef9876543210);
	uet_hdr.ses.initiator = htonl(0x98765432);
	uet_hdr.ses.match_bits = htonll(0x123456789abcdef0);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr), "Send req write medium");
}

static void uet_send_nack_response(struct uet_pdc *pdc, bool syn)
{
	struct {
		struct uet_pds_nack pds;
		struct uet_ses_nodata_response_hdr ses;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_fill_nack(&uet_hdr.pds, pdc, UET_PDS_TYPE_NACK,
			  UET_HDR_RESPONSE);

	uet_ses_fill_common_resp(&uet_hdr.ses.cmn, UET_SES_RESPONSE_TYPE_OTHER);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr), "Send NACK response");
}

static void uet_send_nack_ccx_noresponse(struct uet_pdc *pdc, bool syn)
{
	struct uet_pds_nack_ccx pds;
	int i;

	memset(&pds, 0, sizeof(pds));

	uet_pds_fill_nack(&pds.nack, pdc,  UET_PDS_TYPE_NACK_CCX,
			  UET_HDR_NONE);

	pds.nccx_type = 0xc;
	pds.nccx_ccx_state1 = 0xd;

	for (i = 0; i < 7; i++)
		pds.nack_ccx_state2[i] = 0xd0 + i;

	uet_send_one(pdc, &pds, sizeof(pds), "Send NACK CCX response");
}

static void uet_send_rudi_write_medium(struct uet_pdc *pdc, bool syn)
{
	struct {
		struct uet_pds_rudi_req_resp pds;
		struct uet_ses_request_medium_hdr ses;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_fill_rudi_req_resp(&uet_hdr.pds, pdc, syn,
				   UET_PDS_TYPE_RUDI_REQ,
				   UET_HDR_REQUEST_MEDIUM);

	uet_ses_fill_common(&uet_hdr.ses.cmn,
			    UET_SES_REQUEST_OPCODE_WRITE);

	uet_hdr.ses.buffer_offset = htonll(0xabcdef9876543210);
	uet_hdr.ses.initiator = htonl(0x98765432);
	uet_hdr.ses.match_bits = htonll(0x123456789abcdef0);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr), "Send RUDI write medium");
}

static void uet_send_write_uud_medium(struct uet_pdc *pdc, bool syn)
{
	struct {
		struct uet_pds_uud_req pds;
		struct uet_ses_request_medium_hdr ses;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_hdr.pds.type = UET_PDS_TYPE_UUD_REQ;
	uet_pds_common_set_next_hdr(&uet_hdr.pds, UET_HDR_REQUEST_MEDIUM);
	uet_hdr.pds.flags = 0x7f;

	uet_ses_fill_common(&uet_hdr.ses.cmn,
			    UET_SES_REQUEST_OPCODE_WRITE);

	uet_hdr.ses.buffer_offset = htonll(0xabcdef9876543210);
	uet_hdr.ses.initiator = htonl(0x98765432);
	uet_hdr.ses.match_bits = htonll(0x123456789abcdef0);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr), "Send UUD write medium");
}

static void uet_send_atomic_std(struct uet_pdc *pdc, bool syn)
{
	struct {
		struct uet_pds_rud_rod_request pds;
		struct uet_ses_request_std_hdr ses;
		struct uet_ses_atomic_op_ext_hdr atomic;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_fill_rud_rod_req(&uet_hdr.pds, pdc, syn, UET_PDS_TYPE_RUD_REQ,
				 UET_HDR_REQUEST_STD);
	uet_ses_fill_req_std(&uet_hdr.ses, pdc, UET_SES_REQUEST_OPCODE_ATOMIC);

	uet_hdr.atomic.atomic_code = UET_SES_AMO_PROD;
	uet_hdr.atomic.atomic_datatype = UET_SES_AMO_DATATYPE_DOUBLE_COMPLEX;
	uet_hdr.atomic.ctrl_cacheable = 1;
	uet_hdr.atomic.cpu_coherent = 1;
	uet_hdr.atomic.vendor_defined = 0x3;

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr), "Send atomic std");
}

static void uet_send_atomic_cmp_and_swap_std(struct uet_pdc *pdc, bool syn)
{
	struct {
		struct uet_pds_rud_rod_request pds;
		struct uet_ses_request_std_hdr ses;
		struct uet_ses_atomic_cmp_and_swap_ext_hdr atomic;
	} __packed uet_hdr;
	int i;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_fill_rud_rod_req(&uet_hdr.pds, pdc, syn,
				 UET_PDS_TYPE_ROD_REQ, UET_HDR_REQUEST_STD);
	uet_ses_fill_req_std(&uet_hdr.ses, pdc, UET_SES_REQUEST_OPCODE_ATOMIC);

	uet_hdr.atomic.cmn.atomic_code = UET_SES_AMO_CSWAP_GE;
	uet_hdr.atomic.cmn.atomic_datatype = UET_SES_AMO_DATATYPE_UINT128;
	uet_hdr.atomic.cmn.ctrl_cacheable = 1;
	uet_hdr.atomic.cmn.cpu_coherent = 1;
	uet_hdr.atomic.cmn.vendor_defined = 0x7;

	for (i = 0; i < 16; i++)
		uet_hdr.atomic.compare_value[i] = i;

	for (i = 0; i < 16; i++)
		uet_hdr.atomic.swap_value[i] = 15 - i;

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr),
		     "Send atomic cmp and swap std");
}

static void uet_send_rendezvous_std(struct uet_pdc *pdc, bool syn)
{
	struct {
		struct uet_pds_rud_rod_request pds;
		struct uet_ses_rendezvous_std_hdr rend;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_fill_rud_rod_req(&uet_hdr.pds, pdc, syn,
				 UET_PDS_TYPE_ROD_REQ, UET_HDR_REQUEST_STD);
	uet_ses_fill_common(&uet_hdr.rend.cmn,
			    UET_SES_REQUEST_OPCODE_RENDEZVOUS_SEND);

	uet_hdr.rend.buffer_offset = htonll(0x1234567812345678);
	uet_hdr.rend.initiator = htonl(0x12345678);
	uet_hdr.rend.match_bits = htonll(0x1234567887654321);

	uet_hdr.rend.ext_hdr.eager_length = htonl(0x99887766);
	uet_hdr.rend.ext_hdr.ri_generation = 99;
	uet_ses_rendezvous_ext_hdr_set_pid_on_fep(&uet_hdr.rend.ext_hdr,
						  0x345);
	uet_ses_rendezvous_ext_hdr_set_resource_index(&uet_hdr.rend.ext_hdr,
						      0x123);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr), "Send rendezvous std");
}

static void uet_send_ready_to_restart(struct uet_pdc *pdc, bool syn)
{
	struct {
		struct uet_pds_rud_rod_request pds;
		struct uet_ses_ready_to_restart_std_hdr ses;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_fill_rud_rod_req(&uet_hdr.pds, pdc, syn,
				 UET_PDS_TYPE_ROD_REQ, UET_HDR_REQUEST_STD);

	uet_ses_fill_common(&uet_hdr.ses.cmn,
			    UET_SES_REQUEST_OPCODE_DEFERRABLE_RTR);

	uet_hdr.ses.buffer_offset = htonll(123456789012);
	uet_hdr.ses.initiator = htonl(123456789);
	uet_hdr.ses.initiator_restart_token = htonl(123456789);
	uet_hdr.ses.target_restart_token = htonl(987654321);
	uet_hdr.ses.header_data = htonll(123456789012);
	uet_hdr.ses.request_length = htonl(111111111);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr),
		     "Send ready-to-restart std");
}

static void uet_send_ack_response(struct uet_pdc *pdc, bool arg)
{
	struct {
		struct uet_pds_ack pds;
		struct uet_ses_nodata_response_hdr ses;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_common_set_next_hdr(&uet_hdr.pds, UET_HDR_RESPONSE);
	uet_pds_fill_ack(&uet_hdr.pds, pdc, UET_PDS_TYPE_ACK);

	uet_ses_fill_common_resp(&uet_hdr.ses.cmn, UET_SES_RESPONSE_TYPE_OTHER);

	uet_hdr.ses.modified_length = htonl(0xabcdef12);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr), "Send ACK response");
}

static void uet_send_ack_ccx_response(struct uet_pdc *pdc, bool arg)
{
	struct {
		struct uet_pds_ack_ccx pds;
		struct uet_ses_nodata_response_hdr ses;
	} __packed uet_hdr;
	int i;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_common_set_next_hdr(&uet_hdr.pds, UET_HDR_RESPONSE);
	uet_pds_fill_ack(&uet_hdr.pds.ack, pdc, UET_PDS_TYPE_ACK_CCX);

	uet_hdr.pds.ccx_type = 0xc;
	uet_hdr.pds.cc_flags = 0xb;

	uet_hdr.pds.mpr = 0xfe;
	uet_hdr.pds.sack_psn_offset = htons(0x8765);
	uet_hdr.pds.sack_bitmap = htonll(0x87654321fedcba98);

	for (i = 0; i < 8; i++)
		uet_hdr.pds.ack_cc_state[i] = i + 0xc0;

	uet_ses_fill_common_resp(&uet_hdr.ses.cmn, UET_SES_RESPONSE_TYPE_OTHER);

	uet_hdr.ses.modified_length = htonl(0xabcdef12);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr), "Send ACK CCX response");
}

static void uet_send_ack_response_data_nscc(struct uet_pdc *pdc, bool arg)
{
	struct {
		struct uet_pds_ack_cc pds;
		struct uet_ses_with_data_response_hdr ses;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_common_set_next_hdr(&uet_hdr.pds, UET_HDR_RESPONSE_DATA);
	uet_pds_fill_ack(&uet_hdr.pds.ack, pdc, UET_PDS_TYPE_ACK_CC);

	uet_hdr.pds.cc_type = UET_PDS_CC_TYPE_NSCC;
	uet_hdr.pds.mpr = 67;
	uet_hdr.pds.sack_psn_offset = htons(0x1234);
	uet_hdr.pds.sack_bitmap = htonll(0x123456789abcdef0);

	uet_hdr.pds.nscc.service_time = htons(0x9abc);
	uet_hdr.pds.nscc.restore_cwnd = 1;
	uet_hdr.pds.nscc.rcv_cwnd_pend = 127;
	uet_hdr.pds.nscc.rcvd_bytes = xdp2_htonl24(0x654321);
	uet_hdr.pds.nscc.ooo_count = htons(0x9abc);

	uet_ses_fill_common_resp(&uet_hdr.ses.cmn,
				 UET_SES_RESPONSE_TYPE_WITH_DATA);

	uet_ses_with_data_response_hdr_set_payload_length(&uet_hdr.ses, 0x123);
	uet_hdr.ses.read_request_message_id = htons(9988);
	uet_hdr.ses.modified_length = htonl(0xabcdef12);
	uet_hdr.ses.message_offset = htonl(0x87654321);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr),
		     "Send ACK CCX response with data nscc");
}

static void uet_send_ack_response_data_credit(struct uet_pdc *pdc, bool arg)
{
	struct {
		struct uet_pds_ack_cc pds;
		struct uet_ses_with_small_data_response_hdr ses;
	} __packed uet_hdr;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_pds_common_set_next_hdr(&uet_hdr.pds, UET_HDR_RESPONSE_DATA_SMALL);
	uet_pds_fill_ack(&uet_hdr.pds.ack, pdc, UET_PDS_TYPE_ACK_CC);

	uet_hdr.pds.cc_type = UET_PDS_CC_TYPE_CREDIT;
	uet_hdr.pds.mpr = 67;
	uet_hdr.pds.sack_psn_offset = htons(0x1234);
	uet_hdr.pds.sack_bitmap = htonll(0x123456789abcdef0);

	uet_hdr.pds.credit.credit = xdp2_htonl24(0x345678);
	uet_hdr.pds.credit.ooo_count = htons(0x1234);

	uet_hdr.ses.list = 1;
	uet_hdr.ses.opcode = UET_SES_RESPONSE_TYPE_WITH_DATA;
	uet_hdr.ses.ver = 0;
	uet_hdr.ses.return_code = UET_SES_RETURN_CODE_NO_MATCH;

	uet_ses_with_small_data_response_hdr_set_payload_length(&uet_hdr.ses,
								0x3456);
	uet_hdr.ses.job_id = xdp2_htonl24(0x234);
	uet_hdr.ses.original_request_psn = htonl(0x2346789a);

	uet_send_one(pdc, &uet_hdr, sizeof(uet_hdr),
		     "Send ACK CCX response with data credit");
}

static void uet_send_control_pkt(struct uet_pdc *pdc, bool syn)
{
	struct uet_fep *fep = pdc->fep;
	struct {
		struct uet_pds_control_pkt pds;
	} uet_hdr;
	size_t n;

	memset(&uet_hdr, 0, sizeof(uet_hdr));

	uet_hdr.pds.type = UET_PDS_TYPE_CONTROL;
	uet_hdr.pds.rsvd_isrod = 1;
	uet_hdr.pds.retrans = 1;
	uet_hdr.pds.ackreq = 1;
	uet_hdr.pds.syn = syn;
	uet_hdr.pds.probe_opaque = htons(0x9876);
	uet_hdr.pds.psn = htonl(0x99887766);
	uet_hdr.pds.payload = htonl(0x65432109);

	uet_hdr.pds.spdcid = pdc->local_pdcid;

	if (syn)
		uet_pds_control_pkt_set_psn_offset(&uet_hdr.pds, 0x345);
	else
		uet_hdr.pds.dpdcid = htons(0x8877);

	uet_pds_control_pkt_set_ctl_type(&uet_hdr.pds,
					 UET_PDS_CTL_TYPE_CREDIT_REQ);

	n = fep->comm->send_data(fep->comm, &uet_hdr, sizeof(uet_hdr),
				      pdc->remote_fa, pdc->remote_port);

	__uet_pdc_check_send(pdc, sizeof(uet_hdr), n, "Send control packet");
}

static void test_one(struct uet_fep *fep,
		     void (*func)(struct uet_pdc *pdc, bool syn), bool syn)
{
	struct uet_pdc pdc;

	memset(&pdc, 0, sizeof(pdc));

	pdc.remote_fa.s_addr = htonl(INADDR_LOOPBACK),
	pdc.remote_port = htons(UET_UDP_DEST_PORT(&uet_config)),
	pdc.fep = fep;

	pdc.local_pdcid = htons(0x1234);

	func(&pdc, syn);
}

static void *test_packet_tx(void *arg)
{
	struct uet_fep *fep = arg;

	test_one(fep, uet_send_req_read_std, true);
	test_one(fep, uet_send_req_read_std, false);
	test_one(fep, uet_send_req_cc_read_small, true);
	test_one(fep, uet_send_control_pkt, true);
	test_one(fep, uet_send_control_pkt, false);
	test_one(fep, uet_send_ready_to_restart, false);
	test_one(fep, uet_send_atomic_std, false);
	test_one(fep, uet_send_rendezvous_std, false);
	test_one(fep, uet_send_req_read_small, true);
	test_one(fep, uet_send_req_write_medium, true);
	test_one(fep, uet_send_atomic_cmp_and_swap_std, true);
	test_one(fep, uet_send_defer_send_std, false);

	test_one(fep, uet_send_ack_response, false);
	test_one(fep, uet_send_ack_response_data_nscc, false);
	test_one(fep, uet_send_ack_response_data_credit, false);
	test_one(fep, uet_send_ack_ccx_response, true);

	test_one(fep, uet_send_rudi_write_medium, false);

	test_one(fep, uet_send_nack_response, false);
	test_one(fep, uet_send_nack_ccx_noresponse, false);

	test_one(fep, uet_send_write_uud_medium, false);

	return NULL;
}

int start_test_packet_tx(pthread_t *pthread_id)
{
	struct uet_fep *fep;
	int result;

	if (!(fep = calloc(1, sizeof(*fep)))) {
		XDP2_WARN("start_test_packet_server: calloc failed");
		return -1;
	}

	fep->comm = xdp2_udp_socket_start(-1);
	if (!fep->comm) {
		XDP2_WARN("start_test_packet_tx: udp_socket_start failed");
		return -1;
	}

	result = pthread_create(pthread_id, NULL, test_packet_tx, fep);
	if (result) {
		XDP2_WARN("start_test_packet_tx: pthread_create failed");
		return -1;
	}

	return 0;
}
