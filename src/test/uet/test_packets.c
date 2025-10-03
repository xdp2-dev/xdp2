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

#include "xdp2/parser.h"
#include "xdp2/parser_metadata.h"
#include "xdp2/proto_defs_define.h"
#include "xdp2/utility.h"

/* Define UET parse nodes for XDP2 parser */
#define XDP2_DEFINE_PARSE_NODE
#include "uet/proto_uet_pds.h"
#include "uet/proto_uet_ses.h"
#undef XDP2_DEFINE_PARSE_NODE

#include "uet/config.h"
#include "uet/pds.h"
#include "uet/protocol.h"
#include "uet/ses.h"

#include "test.h"

/* Test for sending and receiving UET packet types */
void test_basic_packets(void)
{

	pthread_t tx_id, rx_id;
	int result;

	/* Start receiver thread */

	result = start_test_packet_rx(UET_UDP_DEST_PORT(&uet_config), &rx_id);

	XDP2_ASSERT(result >= 0, "Start test packet receiver failed");

	/* Start sender thread */

	result = start_test_packet_tx(&tx_id);
	XDP2_ASSERT(result >= 0, "Start test packet sender failed");

	pthread_join(tx_id, NULL);
	pthread_join(rx_id, NULL);
}
