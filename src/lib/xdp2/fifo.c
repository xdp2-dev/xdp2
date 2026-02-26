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

#include "xdp2/cli.h"
#include "xdp2/fifo.h"


void xdp2_fifo_dump_one_fifo(const struct xdp2_fifo *fifo,
			     const char *name, int id1, int id2, void *cli)
{
	char id1txt[12] = "", id2txt[12] = "";
	struct xdp2_fifo_stats *stats = XDP2_ADDR_XLAT(fifo->addr_xlat_num,
						       fifo->stats);
	struct xdp2_fifo_prod_stats *p = &stats->producer;
	struct xdp2_fifo_cons_stats *c = &stats->consumer;

	if (id1 >= 0)
		sprintf(id1txt, "-%u", id1);

	if (id2 >= 0)
		sprintf(id2txt, "-%u", id2);

	/* Output both common counters for both software and hardware FIFOs */
	XDP2_CLI_PRINT(cli, "%s%s%s enqueued: %lu, dequeued %lu,"
			    " num_in_queue: %u , "
			    "num_enqueue_waiters: %u, num_ents: %u, "
			    "ent_size: %u, %s %s %s %s %s %s\n",
		       name, id1txt, id2txt, fifo->num_enqueued,
		       fifo->num_dequeued, xdp2_fifo_num_in_queue(fifo),
		       fifo->num_enqueue_waiters, fifo->num_ents,
		       fifo->ent_size,
		       xdp2_fifo_is_full(fifo) ? "full" : "not-full",
		       xdp2_fifo_is_empty(fifo) ? "empty" : "not-empty",
		       xdp2_fifo_is_error(fifo) ? "error" : "not-error",
		       fifo->initted ? "init" : "no-init",
		       fifo->consumer_cond ? "cons" : "no-cons",
		       fifo->producer_cond ? "prod" : "no-prod");

	if (!stats)
		return;

	/* FIFO stats are enabled */

	/* Output both common stats for both software and hardware FIFOs */
	XDP2_CLI_PRINT(cli, "\tProducer: ops %lu, fails %lu, waits %lu, "
			    "spurious poll wakeups %lu\n",
			p->requests, p->fails, p->waits,
			p->spurious_wakeups_poll);
	XDP2_CLI_PRINT(cli, "\tConsumer: ops %lu, fails %lu, waits %lu, "
			    "spurious poll wakeps %lu\n",
			c->requests, c->fails, c->waits,
			c->spurious_wakeups_poll);
}

void xdp2_fifo_dump_fifos(const struct xdp2_fifo *fifo,
			  unsigned int num_fifos,
			  unsigned int limit, const char *name,
			  int major_index, bool active_only, void *cli)
{
	int i;

	if (!num_fifos)
		return;

	for (i = 0; i < num_fifos; i++,
	     fifo = xdp2_add_len_to_ptr_const(fifo, xdp2_fifo_size(limit,
							fifo->ent_size))) {
		if (active_only && !fifo->num_enqueued)
			continue;
		xdp2_fifo_dump_one_fifo(fifo, name, major_index,
					num_fifos == 1 ? -1 : i, cli);
	}
}

void xdp2_fifo_dump_fifos_add_base(const struct xdp2_fifo *fifo,
				   unsigned int num_fifos,
				   unsigned int limit, const char *name,
				   int major_index, bool active_only,
				   void *cli)
{
	if (!fifo)
		return;

	fifo = xdp2_add_len_to_ptr_const(fifo, major_index * num_fifos *
					 xdp2_fifo_size(limit,
							fifo->ent_size));

	xdp2_fifo_dump_fifos(fifo, num_fifos, limit, name, major_index,
			active_only, cli);
}

void xdp2_fifo_dump_one_poll_group(const struct xdp2_fifo_poll_group
								*poll_group,
				   const char *name, int id1, int id2,
				   void *cli)
{
	struct xdp2_fifo_poll_group_stats *stats =
			XDP2_ADDR_XLAT(poll_group->addr_xlat_num,
				       poll_group->stats);
	char id1txt[12] = "", id2txt[12] = "";

	if (id1 >= 0)
		sprintf(id1txt, "-%u", id1);

	if (id2 >= 0)
		sprintf(id2txt, "-%u", id2);

	XDP2_CLI_PRINT(cli, "%s%s%s readable: %llx, writable: %llx "
			    "read-mask: %llx, write-mask: %llx %s\n",
			name, id1txt, id2txt, poll_group->fifos_ready[0],
			poll_group->fifos_ready[1], poll_group->fifos_mask[0],
			poll_group->fifos_mask[1],
			poll_group->cond ? "init" : "no-init");

	if (stats) {
		XDP2_CLI_PRINT(cli, "\twait-polls: %lu, "
				     "no-wait-polls: %lu, "
				     "ret-rd-fifo: %lu, "
				     "ret-wr-fifo: %lu, "
				     "ret-no-fifo: %lu, "
				     "spurious-write-polls: %lu\n",
				stats->num_wait_polls,
				stats->num_no_wait_polls,
				stats->num_ret_rd_fifo,
				stats->num_ret_wr_fifo,
				stats->num_ret_no_fifo,
				       stats->spurious_write_polls);
	}
}

void xdp2_fifo_dump_poll_groups(const struct xdp2_fifo_poll_group *poll_group,
				unsigned int num_poll_groups, const char *name,
				int major_index, bool active_only, void *cli)
{
	int i;

	for (i = 0; i < num_poll_groups; i++) {
		if (!active_only || poll_group[i].active)
			xdp2_fifo_dump_one_poll_group(&poll_group[i],
						      name, major_index,
						      num_poll_groups == 1 ?
								-1 : i, cli);
	}
}
