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

#include "uet/pds.h"
#include "uet/protocol.h"
#include "uet/ses.h"
#include "uet/parser_test.h"

#include "xdp2/utility.h"

#include "test.h"

static int handler_okay(const void *hdr, size_t hdr_len, size_t hdr_off,
			void *metadata, void *frame,
			const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t** Okay node\n\n");

	return 0;
}

static int handler_fail(const void *hdr, size_t hdr_len, size_t hdr_off,
			void *metadata, void *frame,
			const struct xdp2_ctrl_data *ctrl)
{
	if (verbose >= 5)
		XDP2_PTH_LOC_PRINTFC(ctrl, "\t** Fail node\n\n");

	return 0;
}

XDP2_MAKE_LEAF_PARSE_NODE(okay_node, xdp2_parse_null,
			  (.ops.handler = handler_okay));

XDP2_MAKE_LEAF_PARSE_NODE(fail_node, xdp2_parse_null,
			  (.ops.handler = handler_fail));

XDP2_PARSER(uet_test_parser_at_udp, "Test parser for UET from UDP",
	    uet_base_node,
	    (.okay_node = &okay_node.pn, .fail_node = &fail_node.pn,));

/* Thread function to run the receiver. Receive a packet on the UDP socket
 * and call the Falcon parser to parse and dump packet fields
 */
static void *test_packet_rx(void *arg)
{
	struct uet_fep *fep = arg;
	struct xdp2_comm_handle *receive_handle = fep->comm;
	__u32 flags = (parser_debug ? XDP2_F_DEBUG : 0);
	struct xdp2_ctrl_data ctrl;
	unsigned short from_port;
	unsigned int seqno = 0;
	struct in_addr from;
	__u8 buff[1000];
	ssize_t n;

	XDP2_CTRL_INIT_KEY_DATA(&ctrl, uet_test_parser_at_udp, NULL);

	while (1) {
		n = receive_handle->recv_data(receive_handle, buff,
					      1000, &from, &from_port);
		if (n < 0) {
			perror("Receive data failed");
			exit(1);
		}

		XDP2_CTRL_RESET_VAR_DATA(&ctrl);
		XDP2_CTRL_SET_BASIC_PKT_DATA(&ctrl, buff, n, seqno++);

		xdp2_parse(uet_test_parser_at_udp, buff, n, NULL,
			   &ctrl, flags);
	}

	return NULL;
}

/* Start receiver thread */
int start_test_packet_rx(__u16 port, pthread_t *pthread_id)
{
	struct uet_fep *fep;
	int result;

	if (!(fep = calloc(1, sizeof(*fep)))) {
		XDP2_WARN("start_test_packet_rx: calloc failed");
		return -1;
	}

	fep->comm = xdp2_udp_socket_start(port);
	if (!fep->comm) {
		XDP2_WARN("start_test_packet_rx: udp_socket_start failed");
		return -1;
	}

	result = pthread_create(pthread_id, NULL, test_packet_rx, fep);
	if (result) {
		XDP2_WARN("start_test_packet_rx: pthread_create failed");
		return -1;
	}

	return 0;
}
