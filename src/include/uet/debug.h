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

#ifndef __UET_DEBUG_H__
#define __UET_DEBUG_H__

#include <stdbool.h>

extern bool use_colors, debug_colors_feps;

#define UET_DEBUG_F_PDC			XDP2_BIT(0)
#define UET_DEBUG_F_TRANS		XDP2_BIT(1)
#define UET_DEBUG_F_PACKET		XDP2_BIT(2)
#define UET_DEBUG_F_FEP			XDP2_BIT(3)
#define UET_DEBUG_F_SHOW_PKTS		XDP2_BIT(4)

#define __UET_DEBUG_COLOR_PDC	XDP2_TERM_COLOR_YELLOW
#define __UET_DEBUG_COLOR_TRANS	XDP2_TERM_COLOR_GREEN
#define __UET_DEBUG_COLOR_PACKET	XDP2_TERM_COLOR_MAGENTA

#define UET_DEBUG_PRINT(FEP, COLOR, ...) do {				\
	struct uet_fep *__fep = FEP;					\
									\
	if (use_colors) {						\
		if (debug_colors_feps)					\
			 XDP2_CLI_PRINT_COLOR_SELECT(__fep->debug_cli,	\
						     __fep->num,	\
						     __VA_ARGS__);	\
		else							\
			XDP2_CLI_PRINT_COLOR(__fep->debug_cli, COLOR,	\
					     __VA_ARGS__);		\
	} else {							\
		XDP2_CLI_PRINT(__fep->debug_cli, __VA_ARGS__);		\
	}								\
} while (0)

#define UET_DEBUG_FEP(FEP, ...) do {					\
	struct uet_fep *_fep = FEP;					\
	char buff[100];							\
	int n;								\
									\
	if (!(_fep->debug_mask & UET_DEBUG_F_FEP))			\
		break;							\
									\
	n = snprintf(buff, 100, "FEP %u:: ", _fep->num);		\
	n += snprintf(&buff[n], 100 - n, __VA_ARGS__);			\
	n += snprintf(&buff[n], 100 - n, "\n");				\
	UET_DEBUG_PRINT(_fep, __UET_DEBUG_COLOR_PDC,			\
			   "%s", buff);					\
} while (0)

#define UET_DEBUG_PDC(PDC, ...) do {					\
	struct uet_pdc *_pdc = PDC;					\
	struct uet_fep *_fep = _pdc->fep;				\
	char buff[100];							\
	int n;								\
									\
	if (!(_fep->debug_mask & UET_DEBUG_F_PDC))			\
		break;							\
									\
	n = snprintf(buff, 100, "PDC %u:%u:: ", _fep->num,		\
		     ntohs(_pdc->local_pdcid));				\
	n += snprintf(&buff[n], 100 - n, __VA_ARGS__);			\
	n += snprintf(&buff[n], 100 - n, "\n");				\
	UET_DEBUG_PRINT(_fep, __UET_DEBUG_COLOR_PDC, "%s", buff);	\
} while (0)

#define __UET_DEBUG_TRANS(PDC, TRANS, LABEL, ...) do {			\
	struct uet_pdc *_pdc = PDC;					\
	struct uet_fep *_fep = _pdc->fep;				\
	struct uet_transaction *_trans = TRANS;				\
	char buff[100];							\
	int n;								\
									\
	if (!(_fep->debug_mask & UET_DEBUG_F_TRANS))			\
		break;							\
									\
	n = snprintf(buff, 100, LABEL "Trans %u:%u:%u:: ",		\
		     _fep->num, _pdc->local_cid,			\
		     _trans->req_seqno);				\
	n += snprintf(&buff[n], 100 - n, __VA_ARGS__);			\
	n += snprintf(&buff[n], 100 - n, "\n");				\
	UET_DEBUG_PRINT(_fep, __UET_DEBUG_COLOR_TRANS,			\
			   "%s", buff);					\
} while (0)

#define UET_DEBUG_TRANS(PDC, TRANS, ...)				\
	__UET_DEBUG_TRANS(PDC, TRANS, "", __VA_ARGS__)

#define UET_DEBUG_INITIATOR_TRANS(PDC, TRANS, ...)			\
	__UET_DEBUG_TRANS(PDC, TRANS, "Initiator ", __VA_ARGS__)

#define UET_DEBUG_TARGET_TRANS(PDC, TRANS, ...)				\
	__UET_DEBUG_TRANS(PDC, TRANS, "Target ", __VA_ARGS__)

#define UET_DEBUG_PACKET(PDC, PACKET, ...) do {				\
	struct uet_pdc *_pdc = PDC;					\
	struct uet_fep *_fep = _pdc->fep;				\
	struct uet_packet *_packet = PACKET;				\
	char buff[100];							\
	int n;								\
									\
	if (!(_fep->debug_mask & UET_DEBUG_F_PACKET))			\
		break;							\
									\
	n = snprintf(buff, 100, "Packet %u:%u:: ", _fep->num,		\
		     _pdc->local_cid);					\
	n += snprintf(&buff[n], 100 - n, __VA_ARGS__);			\
	n += snprintf(&buff[n], 100 - n, "\n");				\
	UET_DEBUG_PRINT(_fep, __UET_DEBUG_COLOR_PACKET,			\
			   "%s", buff);					\
} while (0)

#endif /* __UET_DEBUG_H__ */
