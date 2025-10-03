// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2020 Tom Herbert
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

#define XDP2_DEFINE_PARSE_NODE
#include "falcon/proto_falcon.h"
#undef XDP2_DEFINE_PARSE_NODE

#include "falcon/config.h"
#include "falcon/protocol.h"

#include "test.h"

/* Functions to send each of the packet types */

static void test_pull_req(struct falcon_instance *instance)
{
	struct falcon_conn conn;

	memset(&conn, 0, sizeof(conn));

	conn.instance = instance;

	conn.remote.s_addr = htonl(INADDR_LOOPBACK);
	conn.dport = htons(FALCON_UDP_DEST_PORT(&falcon_config));
	conn.protocol_type = FALCON_PROTO_TYPE_RDMA;

	conn.remote_cid = 0x123456;
	conn.dest_function = 0x89abcd;

	conn.recv_data_base_psn = 0x8765309a;
	conn.recv_req_base_psn = 0xfedcba98;

	falcon_send_pull_req_pkt(&conn, 0x31425364, 0xffeeddcc, true, 5566);
}

static void test_pull_data(struct falcon_instance *instance)
{
	struct falcon_conn conn;

	memset(&conn, 0, sizeof(conn));

	conn.instance = instance;

	conn.remote.s_addr = htonl(INADDR_LOOPBACK);
	conn.dport = htons(FALCON_UDP_DEST_PORT(&falcon_config));
	conn.protocol_type = FALCON_PROTO_TYPE_RDMA;

	conn.remote_cid = 0x123456;
	conn.dest_function = 0x89abcd;

	conn.recv_data_base_psn = 0x8765309a;
	conn.recv_req_base_psn = 0xfedcba98;

	falcon_send_pull_data_pkt(&conn, 0x31425364, 0xffeeddcc, false);
}

static void test_push_data(struct falcon_instance *instance)
{
	struct falcon_conn conn;

	memset(&conn, 0, sizeof(conn));

	conn.instance = instance;

	conn.remote.s_addr = htonl(INADDR_LOOPBACK);
	conn.dport = htons(FALCON_UDP_DEST_PORT(&falcon_config));
	conn.protocol_type = FALCON_PROTO_TYPE_RDMA;

	conn.remote_cid = 0x123456;
	conn.dest_function = 0x89abcd;

	conn.recv_data_base_psn = 0x8765309a;
	conn.recv_req_base_psn = 0xfedcba98;

	conn.init_trans_base_seqno = 0x31425364;
	conn.init_trans_next_seqno = 0xffeeddcc;

	falcon_send_push_data_pkt(&conn, 0x31425364, 0xffeeddcc, true, 8192);
}

static void test_resync(struct falcon_instance *instance)
{
	struct falcon_conn conn;

	memset(&conn, 0, sizeof(conn));

	conn.instance = instance;

	conn.remote.s_addr = htonl(INADDR_LOOPBACK);
	conn.dport = htons(FALCON_UDP_DEST_PORT(&falcon_config));
	conn.protocol_type = FALCON_PROTO_TYPE_NVME;

	conn.remote_cid = 0x123456;
	conn.dest_function = 0x89abcd;

	conn.recv_data_base_psn = 0x8765309a;
	conn.recv_req_base_psn = 0xfedcba98;

	falcon_send_resync_pkt(&conn, 0x31425364, 0xffeeddcc, true,
			       FALCON_RESYNC_CODE_REMOTE_XLR_FLOW,
			       FALCON_PKT_TYPE_NACK, 0x77778888);
}

static void test_ack(struct falcon_instance *instance)
{
	struct falcon_conn conn;

	memset(&conn, 0, sizeof(conn));

	conn.instance = instance;

	conn.remote.s_addr = htonl(INADDR_LOOPBACK);
	conn.dport = htons(FALCON_UDP_DEST_PORT(&falcon_config));
	conn.protocol_type = FALCON_PROTO_TYPE_NVME;

	conn.remote_cid = 0x654321;

	conn.recv_data_base_psn = 0xccddeeff;
	conn.recv_req_base_psn = 0x12345678;

	conn.timestamp_t1 = 0x99887766;
	conn.timestamp_t2 = 0x55667788;

	conn.ecn_rx_pkt_cnt = 0x2342;
	conn.rue_info = 0x76655;
	conn.oo_wind_notify = 3;

	falcon_send_ack_pkt(&conn, 7, 25);
}

static void test_eack(struct falcon_instance *instance)
{
	struct falcon_conn conn;

	memset(&conn, 0, sizeof(conn));

	conn.instance = instance;

	conn.remote.s_addr = htonl(INADDR_LOOPBACK);
	conn.dport = htons(FALCON_UDP_DEST_PORT(&falcon_config));
	conn.protocol_type = FALCON_PROTO_TYPE_NVME;

	conn.remote_cid = 0x654323;

	conn.recv_data_base_psn = 0xccddeeff;
	conn.recv_req_base_psn = 0x12345678;

	conn.timestamp_t1 = 0x99887766;
	conn.timestamp_t2 = 0x55667788;
	conn.ecn_rx_pkt_cnt = 0x2342;
	conn.rue_info = 2;
	conn.oo_wind_notify = 3;

	conn.recv_data_ack_bitmap[1] = htonll(0x1a2a3a4a12345678);
	conn.recv_data_ack_bitmap[0] = htonll(0x1b2b3b4b87654321);
	conn.recv_data_rx_bitmap[1] = htonll(0xabacada789abcdef);
	conn.recv_data_rx_bitmap[0] = htonll(0xebecedecba987654);
	conn.recv_req_ack_bitmap = htonll(0x707172738675309d);

	falcon_send_eack_pkt(&conn, 7, 25);
}

static void test_nack(struct falcon_instance *instance)
{
	struct falcon_conn conn;

	memset(&conn, 0, sizeof(conn));

	conn.instance = instance;

	conn.remote.s_addr = htonl(INADDR_LOOPBACK);
	conn.dport = htons(FALCON_UDP_DEST_PORT(&falcon_config));
	conn.protocol_type = FALCON_PROTO_TYPE_NVME;

	conn.remote_cid = 0xbec001;

	conn.recv_data_base_psn = 0xccddeeff;
	conn.recv_req_base_psn = 0x12345678;

	conn.timestamp_t1 = 0x99887766;
	conn.timestamp_t2 = 0x55667788;
	conn.ecn_rx_pkt_cnt = 0x1342;
	conn.rue_info = 2;
	conn.nack_psn = 0x55443322;

	falcon_send_nack_pkt(&conn, 2, 25, FALCON_NACK_CODE_ULP_NONRECOV_ERROR,
			     29, 1, 55);
}

/* Thread function to run the sender. Send a sample packet for each of the
 * seven Falcon packet types
 */
static void *test_packet_tx(void *arg)
{
	struct falcon_instance *instance = arg;

	test_pull_req(instance);
	test_pull_data(instance);
	test_push_data(instance);
	test_resync(instance);
	test_ack(instance);
	test_eack(instance);
	test_nack(instance);

	return NULL;
}

int start_test_packet_tx(pthread_t *pthread_id)
{
	struct falcon_instance *instance;
	int result;

	if (!(instance = calloc(1, sizeof(*instance)))) {
		XDP2_WARN("start_test_packet_server: calloc failed");
		return -1;
	}

	instance->comm = xdp2_udp_socket_start(-1);
	if (!instance->comm) {
		XDP2_WARN("start_test_packet_tx: udp_socket_start failed");
		return -1;
	}

	result = pthread_create(pthread_id, NULL, test_packet_tx, instance);
	if (result) {
		XDP2_WARN("start_test_packet_tx: pthread_create failed");
		return -1;
	}

	return 0;
}
