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

#ifndef __UET_PROTOCOL_H__
#define __UET_PROTOCOL_H__

/* Definitions for UET protocol implementation */

#include "xdp2/bitmap.h"
#include "xdp2/timer.h"
#include "xdp2/udp_comm.h"

#include "uet/debug.h"

void uet_init(void);

enum uet_pdc_state {
	UET_PDC_STATE_CLOSED,
	UET_PDC_STATE_OPENING,
	UET_PDC_STATE_ESTABLISHED,
};

enum uet_pdc_type {
	UET_PDC_TYPE_RUD,
	UET_PDC_TYPE_ROD,
};

struct uet_pdc_stats {
	unsigned long send_failed;
};

/* An initiator matches a a PDC by { source FA, destination FA, TC, RUD/ROD }
 * FA is fabric address, TC is traffic class, RUD/ROD is uet_pdc_type enum
 */
struct uet_pdc {
	/* Pointer to fabric endpoint structure (i.e. instance) */
	struct uet_fep *fep;

	/* Type of PDC (ROD or RUD) */
	enum uet_pdc_type type;

	/* When true this is an initiator PDCID, else it's a target PDCID */
	bool initiator;

	/* Traffic class. Used for matching and to set DSCP on the wire.
	 * Values are canonical differentiated services code points
	 * (DSCP values in <netinet/ip.h> may be used)
	 */
	__u8 traffic_class;

	/* IP address and destination port of destination. Just IPv4 for
	 * now
	 */
	/* IPv4 remote fabric address. Used in intiator mapping tuple */
	struct in_addr remote_fa;

	/* IPv4 local fabric address. Used in initiator mapping tuple */
	struct in_addr local_fa;

	/* Destination port is not used in matching */
	__be16 remote_port;

	/* Local PDCID. If initiator is true then this is the intiator
	 * PDCID for the connection, else it's the target PDCID. In either
	 * case it's set as the spdcid in generated packets (NBO). This
	 * is equal to the index of the pdc in the fep->pdcs[] array
	 */
	__be16 local_pdcid;

	/* Remote PDCID. If initiator is true then this is the target
	 * PDCID for the connection, else it's the initiator PDCID. In either
	 * case it's set as the dpdcid in generated packets (NBO)
	 */
	__be16 remote_pdcid;

	enum uet_pdc_state state;

	/* Start PSN for the current PDC instance. If the PSN goes past
	 * the start_psn + 1/2 the full window then time to kill pdc and
	 * use a new one (configurable)
	 */
	__u32 start_psn;

	/* The next PSN to send in a request packet */
	__u32 psn;

	/* The cleared PSN, for acknowledging ACKs */
	__u32 clear_psn;

	/* Cumulative acknowledgment */
	__u32 cack_psn;

	/* ACK timer */
	struct xdp2_timer ack_timer;

	/* PDC stats */
	struct uet_pdc_stats stats;
};

#define UET_PDC_STATS_BUMP(PDC, NAME) do {			\
	struct uet_pdc *_pdc = PDC;				\
								\
	_pdc->stats.NAME++;					\
} while (0)

struct uet_fep_stats {
	unsigned long read_requests;
	unsigned long nacks_sent;
};

struct uet_fep {
	/* INdex number */
	unsigned int num;

	/* Maximum number of PDCs */
	unsigned int max_pdcs;

	/* Communications handle */
	struct xdp2_comm_handle *comm;

	/* ULP callbacks */
	struct ulp_callbacks *ulp_callbacks;

	/* Debug mask */
	__u32 debug_mask;

	/* Debug CLI pointer */
	void *debug_cli;

	/* Times */
	struct xdp2_timer_wheel *timer_wheel;

	/* Statistics */
	struct uet_fep_stats stats;

	/* Bitmap of allocated PDCs */
	unsigned long *pdc_alloc_map;

	/* PDCs, number indicated by max_pdcs */
	struct uet_pdc pdcs[];
};

#define UET_FEP_STATS_BUMP(FEP, NAME) do {			\
	struct uet_fep *_fep = FEP;				\
								\
	_fep->stats.NAME++;					\
} while (0)

struct uet_fep *uet_create_fep(void);

/* Lookup a PDC based on the parameters of an initiator request */
static inline struct uet_pdc *uet_pdc_get_pdc_from_initiator_request(
		struct uet_fep *fep, struct in_addr src,
		struct in_addr dest, __be16 port,
		enum uet_pdc_type pdc_type, __u8 traffic_class)
{
	struct uet_pdc *pdc;
	int i;

	i = 0;
	xdp2_bitmap_foreach_bit(fep->pdc_alloc_map, i, fep->max_pdcs) {
		pdc = &fep->pdcs[i];

		if (pdc && pdc->initiator &&
		    pdc->remote_fa.s_addr == dest.s_addr &&
		    pdc->remote_port == port && pdc->type == pdc_type &&
		    pdc->traffic_class == traffic_class) {
			XDP2_ASSERT(pdc->state != UET_PDC_STATE_CLOSED,
				    "PDC bitmap says open but PDC state is "
				    "closed for PDC %u\n", i);
			return pdc;
		}
	}

	return NULL;
}

/* Lookup a PDC from received SYN packet */
static inline struct uet_pdc *uet_get_from_syn(struct uet_fep *fep,
					       struct in_addr address,
					       __be16 pdcid)
{
	struct uet_pdc *pdc;
	int i = 0;

	xdp2_bitmap_foreach_bit(fep->pdc_alloc_map, i, fep->max_pdcs) {
		pdc = &fep->pdcs[i];

		if (pdc->remote_fa.s_addr != address.s_addr ||
		    pdc->remote_pdcid != pdcid)
			continue;

		XDP2_ASSERT(pdc->state != UET_PDC_STATE_CLOSED,
			    "PDC bitmap says open but PDC state is "
			    "closed for PDC %u\n", i);

		/* Hurray, we have found a PDC */
		return pdc;
	}

	return NULL;
}

/* Lookup a pdcid for a non-SYN packet. This can alco be used in an
 * initiator request with a locked pdcid or a target response
 */
static inline struct uet_pdc *uet_get_pdc_from_pcid_api(
		struct uet_fep *fep, __u16 pdcid)
{
	if (pdcid >= fep->max_pdcs)
		return NULL;

	return &fep->pdcs[pdcid];
}

/* Lookup a PCD from a pcdid that was received in a packet */
static inline struct uet_pdc *uet_get_pdc_from_pdcid(
		struct uet_fep *fep, __be16 pdcid)
{
	pdcid = ntohs(pdcid);
	if (pdcid == 0)
		return NULL;

	pdcid--;

	return uet_get_pdc_from_pcid_api(fep, pdcid);
}

static inline void __uet_pdc_check_send(struct uet_pdc *pdc, ssize_t expect,
					ssize_t got, const char *label)
{
	if (expect == got)
		return;

	UET_DEBUG_PDC(pdc, "Packet send %s failed: expected %ld and "
			   "got %ld", label, expect, got);

	UET_PDC_STATS_BUMP(pdc, send_failed);
}

static inline void __uet_fep_check_send(struct uet_fep *fep, ssize_t expect,
					ssize_t got, const char *label)
{
	if (expect == got)
		return;

	UET_DEBUG_FEP(fep, "Packet send %s failed: expected %ld and "
			   "got %ld", label, expect, got);

//	UET_PDC_STATS_BUMP(pdc, send_failed);
}

static inline void __uec_check_send_psn(struct uet_pdc *pdc, __u32 psn,
					ssize_t expect, ssize_t got,
					const char *label)
{
	if (expect == got)
		return;

	UET_DEBUG_PDC(pdc, "Packet send %s failed with seqno %u: "
			   "expected %ld and got %ld",
		      label, psn, expect, got);

	UET_PDC_STATS_BUMP(pdc, send_failed);
}

void uet_pdc_check_send_ack(struct uet_pdc *pdc, bool ack_req);

#endif /* __UET_PROTOCOL_H__ */
