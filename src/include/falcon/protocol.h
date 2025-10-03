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

#ifndef __XDP2_FALCON_PROTOCOL_H__
#define __XDP2_FALCON_PROTOCOL_H__

/* Falon implementation definitions */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "falcon/debug.h"
#include "falcon/falcon.h"

#include "xdp2/config.h"
#include "xdp2/locks.h"
#include "xdp2/timer.h"
#include "xdp2/udp_comm.h"

struct falcon_instance;

enum falcon_initiator_transact_state {
	/* Pull transactions */
	FALCON_INITIATOR_TRANSACT_PULL_REQ_FROM_ULP,
	FALCON_INITIATOR_TRANSACT_PULL_REQ_TX,
	FALCON_INITIATOR_TRANSACT_PULL_REQ_ACKD,
	FALCON_INITIATOR_TRANSACT_PULL_DATA_RX,
	FALCON_INITIATOR_TRANSACT_PULL_DATA_TO_ULP,

	/* Push transactions */
	FALCON_INITIATOR_TRANSACT_PUSH_DATA_FROM_ULP,
	FALCON_INITIATOR_TRANSACT_PUSH_DATA_TX,
	FALCON_INITIATOR_TRANSACT_PUSH_DATA_ACKD,
	FALCON_INITIATOR_TRANSACT_PUSH_DATA_CPL_TO_ULP,
};

static inline const char *falcon_initiator_transact_state_to_text(
		enum falcon_initiator_transact_state state)
{
	switch (state) {
	case FALCON_INITIATOR_TRANSACT_PULL_REQ_FROM_ULP:
		return "initiator-pull-req-from-ulp";
	case FALCON_INITIATOR_TRANSACT_PULL_REQ_TX:
		return "initiator-pull-req-tx";
	case FALCON_INITIATOR_TRANSACT_PULL_REQ_ACKD:
		return "initiator-pull-req-ackd";
	case FALCON_INITIATOR_TRANSACT_PULL_DATA_RX:
		return "initiator-pull-data-rx";
	case FALCON_INITIATOR_TRANSACT_PULL_DATA_TO_ULP:
		return "initiator-pull-data-to-ulp";

	case FALCON_INITIATOR_TRANSACT_PUSH_DATA_FROM_ULP:
		return "initiator-push-data-from-ulp";
	case FALCON_INITIATOR_TRANSACT_PUSH_DATA_TX:
		return "initiator-push-data-tx";
	case FALCON_INITIATOR_TRANSACT_PUSH_DATA_ACKD:
		return "initiator-push-data-ackd";
	case FALCON_INITIATOR_TRANSACT_PUSH_DATA_CPL_TO_ULP:
		return "initiator-push-cpl-to-ulp";

	default:
		return "Unknown state";
	}
}

enum falcon_target_transact_state {
	/* Pull transactions */
	FALCON_TARGET_TRANSACT_PULL_REQ_RX,
	FALCON_TARGET_TRANSACT_PULL_REQ_TO_ULP,
	FALCON_TARGET_TRANSACT_PULL_REQ_ULP_ACKD,
	FALCON_TARGET_TRANSACT_PULL_DATA_FROM_ULP,
	FALCON_TARGET_TRANSACT_PULL_DATA_TX,
	FALCON_TARGET_TRANSACT_PULL_DATA_ACKD,

	/* Push transactions */
	FALCON_TARGET_TRANSACT_PUSH_DATA_RX,
	FALCON_TARGET_TRANSACT_PUSH_DATA_TO_ULP,
	FALCON_TARGET_TRANSACT_PUSH_DATA_ULP_ACKD,
};

static inline const char *falcon_target_transact_state_to_text(
		enum falcon_target_transact_state state)
{
	switch (state) {
	case FALCON_TARGET_TRANSACT_PULL_REQ_RX:
		return "target-pull-req-rx";
	case FALCON_TARGET_TRANSACT_PULL_REQ_TO_ULP:
		return "target-pull-req-to-ulp";
	case FALCON_TARGET_TRANSACT_PULL_REQ_ULP_ACKD:
		return "target-pull-req-ulp-ackd";
	case FALCON_TARGET_TRANSACT_PULL_DATA_FROM_ULP:
		return "target-pull-data-from-ulp";
	case FALCON_TARGET_TRANSACT_PULL_DATA_TX:
		return "target-pull-data-tx";
	case FALCON_TARGET_TRANSACT_PULL_DATA_ACKD:
		return "target-pull-data-ackd";

	/* Push transactions */
	case FALCON_TARGET_TRANSACT_PUSH_DATA_RX:
		return "target-push-data-rx";
	case FALCON_TARGET_TRANSACT_PUSH_DATA_TO_ULP:
		return "target-push-data-ulp-to-ulp";
	case FALCON_TARGET_TRANSACT_PUSH_DATA_ULP_ACKD:
		return "target-push-data-ulp-ackd";

	default:
		return "Unknown state";
	}
}

enum falcon_ulp_errors {
	FALCON_ULP_NO_ERR = 0,
	FALCON_ULP_BAD_CID = -1,
	FALCON_ULP_BAD_REQ_SEQNO = -2,
	FALCON_ULP_TRANS_ALLOC_FAILED = -3,
	FALCON_NO_AVAIL_REQUESTS = -4,
};

struct falcon_transaction {
	union {
		enum falcon_initiator_transact_state initiator_state;
		enum falcon_target_transact_state target_state;
	};

	__u32 req_seqno;
	__u16 request_size;
};

struct falcon_packet {
};

/* Falcon protocol control block */

struct falcon_conn_stats {
	unsigned long req_pkts_sent;
	unsigned long data_pkts_sent;
	unsigned long send_failed;
};

#define FALCON_CONN_STATS_BUMP(CONN, NAME) do {			\
	struct falcon_conn *_conn = CONN;			\
								\
	_conn->conn_stats.NAME++;				\
} while (0)

struct falcon_conn_initiator_stats {
	unsigned long send_deferred_prior_pending;
	unsigned long send_deferred_no_packets;

	/* Pull operations */
	unsigned long pull_req_from_ulp;
	unsigned long pull_req_tx;
	unsigned long pull_req_ackd;
	unsigned long pull_data_rx;
	unsigned long pull_data_to_ulp;

	/* Push operations */
	unsigned long push_data_from_ulp;
	unsigned long push_data_tx;
	unsigned long push_data_ackd;
	unsigned long push_data_cpl_to_ulp;
};

struct falcon_conn_target_stats {
	/* Pull operations */
	unsigned long pull_req_rx;
	unsigned long pull_req_to_ulp;
	unsigned long pull_data_from_ulp;
	unsigned long pull_data_tx;
	unsigned long pull_data_ackd;

	/* Push operations */
	unsigned long push_data_rx;
	unsigned long push_data_ulp_tx;
	unsigned long push_data_ulp_ackd;
};

#define FALCON_CONN_INITIATOR_STATS_BUMP(CONN, NAME) do {	\
	struct falcon_conn *_conn = CONN;			\
								\
	_conn->initiator_stats.NAME++;				\
} while (0)

#define FALCON_CONN_TARGET_STATS_BUMP(CONN, NAME) do {		\
	struct falcon_conn *_conn = CONN;			\
								\
	_conn->target_stats.NAME++;				\
} while (0)

#define FALCON_OO_WIND_REQ	(1 << 0)
#define FALCON_OO_WIND_DATA	(1 << 1)

#define FALCON_NUM_WIND_DATA_PACKETS	128
#define FALCON_NUM_WIND_REQ_PACKETS	64
#define FALCON_NUM_WIND_TRANSACTIONS	64

struct falcon_conn {
	/* Connection identifiers */
	__u32 local_cid;
	__u32 remote_cid;

	/* Desintation function, per Falcon spec this is <Host, PF, VF> */
	__u32 dest_function;

	/* IP address and destination port of destination. Just IPv4 for
	 * now
	 */
	struct in_addr remote;
	unsigned short dport;

	__u32 req_first_unsent;
	__u32 data_first_unsent;

	__u32 targ_reply_first_unsent;

	/* Falcon protocol type: RDMA or NVMe */
	enum falcon_protocol_type protocol_type;

	bool ordered;

	bool target_ulp_delivery_blocked;

	XDP2_LOCKS_MUTEX_T mutex;

	/* XXX Need to sort these out */
	__u32 timestamp_t1;
	__u32 timestamp_t2;

	__u16 ecn_rx_pkt_cnt;

	__u32 rue_info;

	__u8 oo_wind_notify;

	__u32 nack_psn;

	/* ACK timer */
	struct xdp2_timer ack_timer;

	struct falcon_instance *instance;

	/**** Request packet state */

	/* Send state */

	/* Base PSN of the send request window */
	__u32 send_req_base_psn;

	/* Next PSN for a send request packet */
	__u32 send_req_next_psn;

	/* Request send packets array (64). Indexed by seqno % 64 */
	struct falcon_packet *send_req_packets[FALCON_NUM_WIND_REQ_PACKETS];

	/* Send request acknowledged bitmap */
	__u64 send_req_ack_bitmap;

	/* Receive state */

	/* Base PSN of the receive request window */
	__u32 recv_req_base_psn;

	/* Receive request acknowledged bitmap */
	__u64 recv_req_ack_bitmap;

	/**** Data packet state ****/

	/* Base PSN of the send data window */
	__u32 send_data_base_psn;

	/* Next PSN for a send data packet */
	__u32 send_data_next_psn;

	/* Request send packets array (128). Indexed by seqno % 128 */
	struct falcon_packet *send_data_packets[FALCON_NUM_WIND_DATA_PACKETS];

	/* Send data acknowledged bitmap */
	__u64 send_data_ack_bitmap[2];

	/* Receive state */

	/* Base PSN of the receive data window */
	__u32 recv_data_base_psn;

	/* Receive data bitmap. This is also conveyed in EACK packet
	 * as received but not necessarily acknowledged data packets
	 */
	__u64 recv_data_rx_bitmap[2];

	/* Receive data acknowledged bitmap. This is also conveyed in EACK
	 * packet as received and acknowledged packets
	 */
	__u64 recv_data_ack_bitmap[2];

	/**** Initiator state ****/

	/* Base sequence number of initiator's transaction window */
	__u32 init_trans_base_seqno;

	/* Next sequence number to assign to an initiator request */
	__u32 init_trans_next_seqno;

	/* Bitmaps */

	/* The pull request or push data has been sent to the target
	 *
	 * Used for pull request and push data
	 */
	__u64 init_sent_to_targ_bitmap;

	/* The target has acknowledged the packet
	 *
	 * Used for pull request and push data
	 */
	__u64 init_targ_acked_bitmap;

	/* Transaction has been delivered to ULP bitmap
	 *
	 * Used for pull request and push data
	 */
	__u64 init_trans_recv_data_from_net;

	__u64 init_trans_to_ulp_bitmap;

	/* Delivered to ULP
	 *
	 * For a pull request data has been delivered to ULP, for a push
	 * request completion is delivered to ULP
	 */
	__u64 init_delivered_to_ulp;

	struct falcon_transaction *init_transactions[
				FALCON_NUM_WIND_TRANSACTIONS];
	/**** Target state ****/

	/* The base request sequence number. This refers to the first
	 * request sequence number for either a pull or push transaction
	 * in the window
	 */
	__u32 targ_req_recv_seqno;

	/* Bitmaps */

	/* The received requests sequence number map. If a bit is set
	 * then a request has been received from the network
	 *
	 * Used for pull request and push data
	 */
	__u64 targ_trans_recv_from_network;

	/* Transaction has been delivered to ULP bitmap
	 *
	 * Used for pull request and push data
	 */
	__u64 targ_trans_to_ulp_bitmap;

	/* Pull data received from ULP
	 *
	 * Used for pull request
	 */
	__u64 targ_trans_pull_data_from_ulp_bitmap;

	/* Pull data transmitted
	 *
	 * Used for pull request
	 */
	__u64 targ_trans_pull_data_sent;

	/* Acknowledged
	 *
	 * For a pull request, the pull data packet was ACKed by the
	 * initiator, for a push data the push was acknowledged by the ULP
	 */
	__u64 targ_trans_acknowledged_bitmap;

	/* Array of active transaction states. Offset is request sequence
	 * number modulo 64
	 */
	struct falcon_transaction *targ_req_transactions[
				FALCON_NUM_WIND_TRANSACTIONS];

	/**** Stats ****/

	struct falcon_conn_stats conn_stats;
	struct falcon_conn_initiator_stats initiator_stats;
	struct falcon_conn_target_stats target_stats;
};

#define FALCON_MAX_CONNS 1024

struct falcon_instance_stats {
	unsigned long pull_requests;
	unsigned long pull_datas;
	unsigned long push_datas;
	unsigned long bad_cid_requests;
};

#define FALCON_INSTANCE_STATS_BUMP(INSTANCE, NAME) do {		\
	struct falcon_instance *_instance = INSTANCE;		\
								\
	_instance->stats.NAME++;				\
} while (0)

struct falcon_instance {
	unsigned int num;

	unsigned int max_conns;

	struct xdp2_comm_handle *comm;

	__u32 debug_mask;

	void *debug_cli;

	struct xdp2_timer_wheel *timer_wheel;

	struct falcon_instance_stats stats;

	struct falcon_conn *conns[];
};

static inline void falcon_set_remote_cid(struct falcon_instance *instance,
					 __u32 local_cid, __u32 remote_cid)
{
	instance->conns[local_cid]->remote_cid = remote_cid;
}

/* Functions to create and send Falcon packets */

static inline void falcon_set_base_hdr(struct falcon_conn *conn,
		struct falcon_base_hdr *bhdr, enum falcon_pkt_type packet_type,
		__u32 psn, __u32 req_seqno, bool ack_req)
{
	bhdr->version = FALCON_PROTO_VERSION;
	bhdr->dest_cid = htonl(conn->remote_cid) >> 8;
	bhdr->dest_function = ntohl(conn->dest_function) >> 8;
	bhdr->protocol_type = conn->protocol_type;
	bhdr->packet_type = packet_type;
	bhdr->ack_req = ack_req;

	bhdr->ack_data_psn = htonl(conn->recv_data_base_psn);
	bhdr->ack_req_psn = htonl(conn->recv_req_base_psn);
	bhdr->psn = htonl(psn);
	bhdr->req_seqno = htonl(req_seqno);
}

static inline void __falcon_check_send(struct falcon_instance *instance,
				       struct falcon_conn *conn,
				       __u32 psn, ssize_t expect, ssize_t got,
				       const char *label, bool have_psn)
{
	if (expect == got)
		return;

	if (have_psn)
		FALCON_DEBUG_CONN(conn,
				  "Packet send %s failed with seqno %u: "
				  "expected %ld and got %ld",
				  label, psn, expect, got);
	else
		FALCON_DEBUG_CONN(conn,
				 "Packet send %s failed: expected %ld and "
				 "got %ld", label, expect, got);

	FALCON_CONN_STATS_BUMP(conn, send_failed);
}

static inline void falcon_send_pull_req_pkt(struct falcon_conn *conn,
					    __u32 psn, __u32 req_seqno,
					    bool ack_req, __u16 req_length)
{
	struct falcon_instance *instance = conn->instance;
	struct falcon_pull_req_pkt pkt;
	ssize_t n;

	falcon_set_base_hdr(conn, &pkt.base_hdr, FALCON_PKT_TYPE_PULL_REQUEST,
			    psn, req_seqno, ack_req);

	pkt.req_length = htons(req_length);

	n = instance->comm->send_data(instance->comm, &pkt, sizeof(pkt),
				      conn->remote, conn->dport);

	__falcon_check_send(instance, conn, psn, sizeof(pkt), n,
			    "pull request", true);
}

static inline void falcon_send_pull_data_pkt(struct falcon_conn *conn,
					     __u32 psn, __u32 req_seqno,
					     bool ack_req)
{
	struct falcon_instance *instance = conn->instance;
	struct falcon_pull_data_pkt pkt;
	ssize_t n;

	falcon_set_base_hdr(conn, &pkt.base_hdr, FALCON_PKT_TYPE_PULL_DATA,
			    psn, req_seqno, ack_req);

	n = instance->comm->send_data(instance->comm, &pkt, sizeof(pkt),
				      conn->remote, conn->dport);

	__falcon_check_send(instance, conn, psn, sizeof(pkt), n,
			    "pull data", true);
}

static inline void falcon_send_push_data_pkt(struct falcon_conn *conn,
					     __u32 psn, __u32 req_seqno,
					     bool ack_req, __u16 req_length)
{
	struct falcon_instance *instance = conn->instance;
	struct falcon_push_data_pkt pkt;
	ssize_t n;

	falcon_set_base_hdr(conn, &pkt.base_hdr, FALCON_PKT_TYPE_PUSH_DATA,
			    psn, req_seqno, ack_req);

	pkt.req_length = htons(req_length);

	n = instance->comm->send_data(instance->comm, &pkt, sizeof(pkt),
				      conn->remote, conn->dport);

	__falcon_check_send(instance, conn, psn, sizeof(pkt), n,
			    "push data", true);
}

static inline void falcon_send_resync_pkt(struct falcon_conn *conn,
					  __u32 psn, __u32 req_seqno,
					  bool ack_req,
					  enum falcon_resync_code resync_code,
					  enum falcon_pkt_type resync_pkt_type,
					  __u32 vendor_defined)
{
	struct falcon_instance *instance = conn->instance;
	struct falcon_resync_pkt pkt;
	ssize_t n;

	falcon_set_base_hdr(conn, &pkt.base_hdr, FALCON_PKT_TYPE_RESYNC,
			    psn, req_seqno, ack_req);

	pkt.resync_code = resync_code;
	pkt.resync_pkt_type = resync_pkt_type;
	pkt.vendor_defined = htonl(vendor_defined);

	n = instance->comm->send_data(instance->comm, &pkt, sizeof(pkt),
				      conn->remote, conn->dport);

	__falcon_check_send(instance, conn, psn, sizeof(pkt), n,
			    "resync", true);
}

static inline void falcon_set_base_ack(struct falcon_conn *conn,
				       struct falcon_base_ack_pkt *bhdr,
				       enum falcon_pkt_type packet_type,
				       __u8 hop_count, __u8 rx_buffer_occ)
{
	bhdr->version = FALCON_PROTO_VERSION;
	bhdr->dest_cid = ntohl(conn->remote_cid) >> 8;
	bhdr->pkt_type = packet_type;

	bhdr->ack_data_psn = htonl(conn->recv_data_base_psn);
	bhdr->ack_req_psn = htonl(conn->recv_req_base_psn);
	bhdr->timestamp_t1 = htonl(conn->timestamp_t1);
	bhdr->timestamp_t2 = htonl(conn->timestamp_t2);

	bhdr->hop_count = hop_count;

#if defined(__BIG_ENDIAN_BITFIELD)
	bhdr->ecn_rx_pkt_cnt conn->ecn_rx_pkt_cnt;
	bhdr->rx_buffer_occ = rx_buffer_occ;
	bhdr->rue_info = conn->rue_info;
#else
	bhdr->ecn_rx_pkt_cnt2 = conn->ecn_rx_pkt_cnt & 0x7f;
	bhdr->ecn_rx_pkt_cnt1 = conn->ecn_rx_pkt_cnt >> 7;
	bhdr->rx_buffer_occ1 = rx_buffer_occ >> 1;
	bhdr->rx_buffer_occ2 = rx_buffer_occ & 1;

	bhdr->rue_info1 = conn->rue_info >> 14;
	bhdr->rue_info2 = (conn->rue_info >> 6) & 0xff;
	bhdr->rue_info3 = conn->rue_info & 0x3f;
#endif

	bhdr->oo_wind_notify = conn->oo_wind_notify;
}

static inline void falcon_send_ack_pkt(struct falcon_conn *conn,
				       __u8 hop_count, __u8 rx_buffer_occ)
{
	struct falcon_instance *instance = conn->instance;
	struct falcon_base_ack_pkt pkt;
	ssize_t n;

	falcon_set_base_ack(conn, &pkt, FALCON_PKT_TYPE_BACK,
			    hop_count, rx_buffer_occ);

	n = instance->comm->send_data(instance->comm, &pkt, sizeof(pkt),
				      conn->remote, conn->dport);

	__falcon_check_send(instance, conn, 0, sizeof(pkt), n,
			    "Base ACK", false);
}

static inline void falcon_send_eack_pkt(struct falcon_conn *conn,
					__u8 hop_count, __u8 rx_buffer_occ)
{
	struct falcon_instance *instance = conn->instance;
	struct falcon_ext_ack_pkt pkt;
	ssize_t n;

	falcon_set_base_ack(conn, &pkt.base_ack, FALCON_PKT_TYPE_EACK,
			    hop_count, rx_buffer_occ);

	/* Fill in the bitmaps. Note that the bitmaps are big-endian so
	 * not only do we need to swap words we also need to reorder
	 * word indices for 128-bit bitmaps
	 */
	pkt.data_ack_bitmap[0] = htonll(conn->recv_data_ack_bitmap[1]);
	pkt.data_ack_bitmap[1] = htonll(conn->recv_data_ack_bitmap[0]);

	pkt.data_rx_bitmap[0] = htonll(conn->recv_data_rx_bitmap[1]);
	pkt.data_rx_bitmap[1] = htonll(conn->recv_data_rx_bitmap[0]);

	pkt.req_ack_bitmap = htonll(conn->recv_req_ack_bitmap);

	n = instance->comm->send_data(instance->comm, &pkt, sizeof(pkt),
				      conn->remote, conn->dport);

	__falcon_check_send(instance, conn, 0, sizeof(pkt), n,
			    "Base EACK", false);
}

static inline void falcon_send_nack_pkt(struct falcon_conn *conn,
					__u8 hop_count, __u8 rx_buffer_occ,
					enum falcon_nack_code nack_code,
					__u8 rnr_nack_timeout,
					bool window, __u8 ulp_nack_code)
{
	struct falcon_instance *instance = conn->instance;
	struct falcon_nack_pkt pkt;
	ssize_t n;

	pkt.version = FALCON_PROTO_VERSION;
	pkt.dest_cid = htonl(conn->remote_cid) >> 8;
	pkt.pkt_type = FALCON_PKT_TYPE_NACK;

	pkt.ack_data_psn = htonl(conn->recv_data_base_psn);
	pkt.ack_req_psn = htonl(conn->recv_req_base_psn);
	pkt.timestamp_t1 = htonl(conn->timestamp_t1);
	pkt.timestamp_t2 = htonl(conn->timestamp_t2);

	pkt.hop_count = hop_count;

#if defined(__BIG_ENDIAN_BITFIELD)
	pkt.ecn_rx_pkt_cnt conn->ecn_rx_pkt_cnt;
	pkt.rx_buffer_occ = rx_buffer_occ;
#else
	pkt.ecn_rx_pkt_cnt2 = conn->ecn_rx_pkt_cnt & 0x7f;
	pkt.ecn_rx_pkt_cnt1 = conn->ecn_rx_pkt_cnt >> 7;
	pkt.rx_buffer_occ2 = rx_buffer_occ & 1;
	pkt.rx_buffer_occ1 = rx_buffer_occ >> 1;
#endif

#if defined(__BIG_ENDIAN_BITFIELD)
	pkt.rue_info = conn->rue_info;
#else
	pkt.rue_info3 = conn->rue_info & 0x3f;
	pkt.rue_info2 = (conn->rue_info >> 6) & 0xff;
	pkt.rue_info1 = conn->rue_info >> 14;
#endif

	pkt.nack_psn = htonl(conn->nack_psn);

	pkt.nack_code = nack_code;
	pkt.rnr_nack_timeout = rnr_nack_timeout;
	pkt.window = window;

	pkt.ulp_nack_code = ulp_nack_code;

	n = instance->comm->send_data(instance->comm, &pkt, sizeof(pkt),
				      conn->remote, conn->dport);

	__falcon_check_send(instance, conn, 0, sizeof(pkt), n,
			    "NACK", false);
}

struct falcon_instance *falcon_create_instance(unsigned int max_conns);

struct falcon_conn *falcon_create_conn(struct falcon_instance *instance,
	__be32 remote, unsigned int dport,
	struct xdp2_comm_handle *comm,
	enum falcon_protocol_type protocol_type,
	__u32 debug_mask, __u32 *local_cid);

#endif /* __XDP2_FALCON_PROTOCOL_H__ */
