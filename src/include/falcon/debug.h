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

#ifndef __FALCON_DEBUG_H__
#define __FALCON_DEBUG_H__

#include <stdbool.h>

extern bool use_colors, debug_colors_instances;

#define FALCON_DEBUG_F_CONN		XDP2_BIT(0)
#define FALCON_DEBUG_F_TRANS		XDP2_BIT(1)
#define FALCON_DEBUG_F_PACKET		XDP2_BIT(2)
#define FALCON_DEBUG_F_INSTANCE		XDP2_BIT(3)
#define FALCON_DEBUG_F_SHOW_PKTS	XDP2_BIT(4)

#define __FALCON_DEBUG_COLOR_CONN	XDP2_TERM_COLOR_YELLOW
#define __FALCON_DEBUG_COLOR_TRANS	XDP2_TERM_COLOR_GREEN
#define __FALCON_DEBUG_COLOR_PACKET	XDP2_TERM_COLOR_MAGENTA

#define FALCON_DEBUG_PRINT(INSTANCE, COLOR, ...) do {			\
	struct falcon_instance *__instance = INSTANCE;			\
									\
	if (use_colors) {						\
		if (debug_colors_instances)				\
			XDP2_CLI_PRINT_COLOR_SELECT(			\
					__instance->debug_cli,		\
					__instance->num,		\
					__VA_ARGS__);			\
		else							\
			XDP2_CLI_PRINT(__instance->debug_cli,		\
				       __VA_ARGS__);			\
	} else {							\
		XDP2_CLI_PRINT_COLOR(__instance->debug_cli, COLOR,	\
				     __VA_ARGS__);			\
	}								\
} while (0)

#define FALCON_DEBUG_INSTANCE(INSTANCE, ...) do {			\
	struct falcon_instance *_instance = INSTANCE;			\
	char buff[100];							\
	int n;								\
									\
	if (!(_instance->debug_mask & FALCON_DEBUG_F_INSTANCE))		\
		break;							\
									\
	n = snprintf(buff, 100, "Instance %u:: ", _instance->num);	\
	n += snprintf(&buff[n], 100 - n, __VA_ARGS__);			\
	n += snprintf(&buff[n], 100 - n, "\n");				\
	FALCON_DEBUG_PRINT(_instance, __FALCON_DEBUG_COLOR_CONN,	\
			   "%s", buff);					\
} while (0)

#define FALCON_DEBUG_CONN(CONN, ...) do {				\
	struct falcon_conn *_conn = CONN;				\
	struct falcon_instance *_instance = _conn->instance;		\
	char buff[100];							\
	int n;								\
									\
	if (!(_instance->debug_mask & FALCON_DEBUG_F_CONN))		\
		break;							\
									\
	n = snprintf(buff, 100, "Conn %u:%u:: ", _instance->num,	\
		    _conn->local_cid);					\
	n += snprintf(&buff[n], 100 - n, __VA_ARGS__);			\
	n += snprintf(&buff[n], 100 - n, "\n");				\
	FALCON_DEBUG_PRINT(_instance, __FALCON_DEBUG_COLOR_CONN,	\
			   "%s", buff);					\
} while (0)

#define __FALCON_DEBUG_TRANS(CONN, TRANS, LABEL, ...) do {		\
	struct falcon_conn *_conn = CONN;				\
	struct falcon_instance *_instance = _conn->instance;		\
	struct falcon_transaction *_trans = TRANS;			\
	char buff[100];							\
	int n;								\
									\
	if (!(_instance->debug_mask & FALCON_DEBUG_F_TRANS))		\
		break;							\
									\
	n = snprintf(buff, 100, LABEL "Trans %u:%u:%u:: ",		\
		     _instance->num, _conn->local_cid,			\
		     _trans->req_seqno);				\
	n += snprintf(&buff[n], 100 - n, __VA_ARGS__);			\
	n += snprintf(&buff[n], 100 - n, "\n");				\
	FALCON_DEBUG_PRINT(_instance, __FALCON_DEBUG_COLOR_TRANS,	\
			   "%s", buff);					\
} while (0)

#define FALCON_DEBUG_TRANS(CONN, TRANS, ...)				\
	__FALCON_DEBUG_TRANS(CONN, TRANS, "", __VA_ARGS__)

#define FALCON_DEBUG_INITIATOR_TRANS(CONN, TRANS, ...)			\
	__FALCON_DEBUG_TRANS(CONN, TRANS, "Initiator ", __VA_ARGS__)

#define FALCON_DEBUG_TARGET_TRANS(CONN, TRANS, ...)			\
	__FALCON_DEBUG_TRANS(CONN, TRANS, "Target ", __VA_ARGS__)

#define FALCON_DEBUG_PACKET(CONN, PACKET, ...) do {			\
	struct falcon_conn *_conn = CONN;				\
	struct falcon_instance *_instance = _conn->instance;		\
	struct falcon_packet *_packet = PACKET;				\
	char buff[100];							\
	int n;								\
									\
	if (!(_instance->debug_mask & FALCON_DEBUG_F_PACKET))		\
		break;							\
									\
	n = snprintf(buff, 100, "Packet %u:%u:: ", _instance->num,	\
		     _conn->local_cid);					\
	n += snprintf(&buff[n], 100 - n, __VA_ARGS__);			\
	n += snprintf(&buff[n], 100 - n, "\n");				\
	FALCON_DEBUG_PRINT(_instance, __FALCON_DEBUG_COLOR_PACKET,	\
			   "%s", buff);					\
} while (0)

#endif /* __FALCON_DEBUG_H__ */
