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

#include <arpa/inet.h>
#include <getopt.h>
#include <linux/types.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "xdp2/fifo.h"
#include "xdp2/locks.h"
#include "xdp2/shm.h"
#include "xdp2/utility.h"

#define MAX_THREADS 64

/* Number of threads for consumer */
int num_threads = 1;

/* Test mode */
enum mode {
	ALL_MODE,
	SERVER_MODE,
	CLIENT_MODE,
} mode;

/* Do use xdp2_fifo_poll on server, do non-blocking dequeue instead */
static bool no_poll;

static bool verbose; /* Verbose output */

/* Output  size of FIFO memory. This is useful to get the FIFO memory when
 * FIFOs are in sharded memory
 */
static bool show_size;

/* Internal for showing counts of receieved messages */
static unsigned int interval;

/* Don't wait in xdp2_fifo_poll */
static bool no_poll_wait;

/* Entry size in number of 64-bit words */
static unsigned int ent_size = 1;

/* Don't initialize FIFO memory. This could be used when the FIFO memory
 * is from shared memory and the FIFOs have already been initiaized
 */
bool no_init;

#define MAX_ENT_SIZE 255

/* Producer instance */
struct prod_struct {
	struct xdp2_fifo *fifo;
	unsigned int num;
	pthread_cond_t cond;
	pthread_t thread;
	unsigned int count;
};

struct cons_struct {
	struct xdp2_fifo **fifos;
	unsigned int num;
	pthread_cond_t cond;
	pthread_t thread;
	struct xdp2_fifo_poll_group *poll_group;
};

struct xdp2_fifo *fifos[MAX_THREADS];
struct xdp2_fifo_poll_group *poll_groups[1];

static void dump_one_fifo(const struct xdp2_fifo *fifo,
			  const char *name, int id1, int id2)
{
	char id1txt[12] = "", id2txt[12] = "";
	struct xdp2_fifo_stats *stats = XDP2_ADDR_XLAT(fifo->addr_xlat_num,
							 fifo->stats);

	if (id1 >= 0)
		sprintf(id1txt, "-%u", id1);

	if (id2 >= 0)
		sprintf(id2txt, "-%u", id2);

	printf("%s%s%s enqueued: %lu, dequeued %lu, num_in_queue: %u, "
	       "num_enqueue_waiters: %u, num_ents: %u, %s\n",
	       name, id1txt, id2txt, fifo->num_enqueued,
	       fifo->num_dequeued, xdp2_fifo_num_in_queue(fifo),
	       fifo->num_enqueue_waiters, fifo->num_ents,
	       fifo->queue_full ? "full" : "not-full");

	if (stats) {
		struct xdp2_fifo_prod_stats *p = &stats->producer;
		struct xdp2_fifo_cons_stats *c = &stats->consumer;

		printf("\tProducer: ops %lu, blocked %lu, "
		       "fails %lu , waits %lu, queue made full: "
		       "%lu, spurious wakeups %lu, "
		       "spurious poll wakeups %lu, after wait %lu, "
		       "saw empty %lu, max enqueued: %u\n",
		       p->requests, p->blocked, p->fails, p->waits,
		       p->queue_full, p->spurious_wakeups,
		       p->spurious_wakeups_poll, p->after_wait,
		       p->saw_empty, p->max_enqueued);
		printf("\tConsumer: ops %lu, fails %lu, waits %lu, "
		       "spurious wakeups %lu, spurious poll wakeps %lu, "
		       "unblocked %lu, min enqueues: %u\n",
		       c->requests, c->fails, c->waits, c->spurious_wakeups,
		       c->spurious_wakeups_poll, c->unblocked, c->min_enqueued);
	}
}

static void dump_one_poll_group(const struct xdp2_fifo_poll_group *poll_group,
				const char *name, int id1, int id2)
{
	struct xdp2_fifo_poll_group_stats *stats =
			XDP2_ADDR_XLAT(poll_group->addr_xlat_num,
					poll_group->stats);
	char id1txt[12] = "", id2txt[12] = "";

	if (id1 >= 0)
		sprintf(id1txt, "-%u", id1);

	if (id2 >= 0)
		sprintf(id2txt, "-%u", id2);

	printf("%s%s%s readable: %016llx, writable: %016llx "
	       "read-mask: %016llx, write-mask: %016llx\n",
	       name, id1txt, id2txt, poll_group->fifos_ready[0],
	       poll_group->fifos_ready[1], poll_group->fifos_mask[0],
	       poll_group->fifos_mask[1]);


	if (stats) {
		printf("\twait-polls: %lu, no-wait-polls: %lu, "
		       "ret-rd-fifo: %lu, ret-wr-fifo: %lu, "
		       "ret-no-fifo: %lu, spurious-write-polls: %lu\n",
		       stats->num_wait_polls, stats->num_no_wait_polls,
		       stats->num_ret_rd_fifo, stats->num_ret_wr_fifo,
		       stats->num_ret_no_fifo, stats->spurious_write_polls);
	}
}

static void sig_handler(int signum)
{
	int i;

	for (i = 0; i < num_threads; i++)
		dump_one_fifo(fifos[i], "fifo", i, -1);

	dump_one_poll_group(poll_groups[0], "poll-group", -1, -1);
}

static void *producer_func(void *arg)
{
	struct prod_struct *prod_arg = (struct prod_struct *)arg;
	struct xdp2_fifo *fifo = prod_arg->fifo;
	unsigned int num = prod_arg->num;
	__u64 message[MAX_ENT_SIZE], v;
	int i, j;

	for (i = 1; i <= prod_arg->count; i++) {
		v = 100 * num + i;
		if (ent_size == 1) {
			if (!xdp2_fifo_enqueue(fifo, v, true))
				break;
		} else {
			for (j = 0; j < ent_size; j++)
				message[j] = v + j * 16;
			if (!__xdp2_fifo_enqueue(fifo, ent_size,
						  message, true))
				break;
		}
		usleep(random() % 100);
	}

	return NULL;
}

static bool do_fifo_dequeue(struct xdp2_fifo *fifo, __u64 *messagep,
			    bool wait)
{
	if (ent_size == 1)
		return xdp2_fifo_dequeue(fifo, messagep, wait);

	return __xdp2_fifo_dequeue(fifo, ent_size, messagep, wait);
}

/* Consumer thread function when polling */
static void *consumer_func_poll(void *arg)
{
	struct cons_struct *cons_arg = (struct cons_struct *)arg;
	unsigned int v = 0, count = 0;
	__u64 message[MAX_ENT_SIZE];
	int i;

	while (1) {
		v = xdp2_fifo_poll(cons_arg->poll_group, !no_poll_wait);

		if (!do_fifo_dequeue(cons_arg->fifos[v], message, false)) {
			if (no_poll_wait) {
				__XDP2_FIFO_BUMP_CONS_COUNT(
					cons_arg->fifos[v],
					spurious_wakeups_poll);
				continue;
			}

			XDP2_ERR(1, "Dequeue after poll failed\n");
		}

		if (verbose) {
			if (ent_size == 1) {
				printf("Got message %llx, read fifo %u\n",
				       message[0], v);
			} else {
				printf("Got message read fifo %u: ", v);
				for (i = 0; i < ent_size; i++)
					printf("%llx ", message[i]);
				printf("\n");
			}
		}

		count++;
		if (interval && count % interval == 0)
			printf("I-poll: %u\n", count);

		usleep(random() % 1000);
	}

	return NULL;
}

/* Consumer thread function when not polling */
static void *consumer_func_nopoll(void *arg)
{
	struct cons_struct *cons_arg = (struct cons_struct *)arg;
	unsigned int v, last = 0;
	unsigned int count = 0;
	__u64 message[MAX_ENT_SIZE];
	int i;

	while (1) {
		for (i = 0; i < num_threads; i++) {
			v = (last + i) % num_threads;
			if (do_fifo_dequeue(cons_arg->fifos[v],
					    message, false))
				break;
		}

		if (i < num_threads) {
			if (verbose) {
				if (ent_size == 1) {
					printf("Got nopoll message %llx, "
					       "read fifo %u\n", message[0], v);
				} else {
					printf("Got nopoll message read "
					       "fifo %u: ", v);
					for (i = 0; i < ent_size; i++)
						printf("%llx ", message[i]);
					printf("\n");
				}
			}

			count++;
			if (interval && count % interval == 0)
				printf("I-poll: %u\n", count);

			last = (v + 1) % num_threads;
		} else {
			usleep(random() % 1000);
		}
	}

	return NULL;
}

/* Run server (consumer)  */
static void run_server(struct xdp2_fifo_stats *stats,
		       struct xdp2_fifo_poll_group_stats *poll_group_stats)
{
	struct cons_struct *cons_arg;
	void *vcond;
	int i;

	if (!no_init) {
		for (i = 0; i < num_threads; i++)
			__xdp2_fifo_init(fifos[i], 16, ent_size, 8,
					 &stats[i], 1);
	}

	cons_arg = calloc(1, sizeof(*cons_arg));
	XDP2_ASSERT(cons_arg, "Malloc cons arg failed");

	/* Create consumer thread */

	/* Create conditional veriable */
	pthread_cond_init(&cons_arg->cond, NULL);
	vcond = &cons_arg->cond;

	if (!no_init) {
		/* Initialize consumer side of FIFO (all consumer FIFOs are in
		 * one thread
		 */
		for (i = 0; i < num_threads; i++)
			xdp2_fifo_init_consumer(fifos[i],
						(XDP2_LOCKS_COND_T *)vcond);
	}

	if (!no_poll) {
		/* Create a poll group for consumer thread */

		if (!no_init) {
			xdp2_fifo_init_poll_group_mem(poll_groups[0]);
			__xdp2_fifo_init_poll_group(poll_groups[0], vcond,
						    poll_group_stats, 1,
						    no_poll_wait);

			for (i = 0; i < num_threads; i++) {
				xdp2_fifo_bind_fifo_to_read_poll(poll_groups[0],
								  fifos[i], i);
				xdp2_fifo_enable_read_poll(fifos[i]);
			}
		}

		cons_arg->poll_group = poll_groups[0];
	}

	cons_arg->fifos = fifos;

	/* Start the signal handler for reporting stats */
	signal(SIGUSR1, sig_handler);

	/* STart the threads */
	if (no_poll)
		pthread_create(&cons_arg->thread, NULL, consumer_func_nopoll,
			       cons_arg);
	else
		pthread_create(&cons_arg->thread, NULL, consumer_func_poll,
			       cons_arg);
}

/* RUn client (producer)  */
static void run_client(unsigned int count)
{
	struct prod_struct *prod_arg;
	void *vcond;
	int i;

	prod_arg = calloc(num_threads, sizeof(*prod_arg));
	XDP2_ASSERT(prod_arg, "Malloc prod arg failed");

	/* Now create the threads. Perform a deferred start */
	for (i = 0; i < num_threads; i++) {
		prod_arg[i].fifo = fifos[i];
		prod_arg[i].num = i;
		prod_arg[i].count = count;

		vcond = &prod_arg[i].cond;
		pthread_cond_init(vcond, NULL);
		if (!no_init)
			xdp2_fifo_init_producer(fifos[i],
						 (XDP2_LOCKS_COND_T *)vcond);
	}

	/* Start thread threads */
	for (i = 0; i < num_threads; i++)
		pthread_create(&prod_arg[i].thread, NULL, producer_func,
			       (void *)&prod_arg[i]);

	for (i = 0; i < num_threads; i++)
		pthread_join(prod_arg[i].thread, NULL);
}

/* Run the test */
static void run_test(unsigned int count, char *gshmfile,
		     off_t gshmfile_offset)
{
	void *fifo_mem = NULL, *stats_mem, *poll_group_stats;
	size_t all_fifos_size, one_fifo_size;
	int i;

	one_fifo_size = sizeof(*fifos[0]) + 64 * sizeof(__u64) * ent_size;
	all_fifos_size = num_threads * one_fifo_size +
			 num_threads * sizeof(struct xdp2_fifo_stats) +
			 sizeof(struct xdp2_fifo_poll_group) +
			 sizeof(struct xdp2_fifo_poll_group_stats);

	if (gshmfile) {
		fifo_mem = xdp2_shm_map_existing(gshmfile, all_fifos_size,
						 gshmfile_offset);
		XDP2_ASSERT(fifo_mem, "Global shared memory map failed\n");
	} else {
		fifo_mem = malloc(all_fifos_size);
		XDP2_ASSERT(fifo_mem, "Alloc FIFO memory failed\n");
	}

	if (show_size)
		printf("All size %lu, one FIFO %lu\n",
		       all_fifos_size, one_fifo_size);

	if (!count)
		exit(0);

	xdp2_addr_xlat_set_base(1, fifo_mem);

	if ((mode == ALL_MODE || mode == SERVER_MODE) && !no_init)
		memset(fifo_mem, 0, all_fifos_size);

	/* Create one FIFO for each thread. The producer side will be
	 * given to one of the producer threads, and the consumer side
	 * will be given to the consumer thread
	 */
	for (i = 0; i < num_threads; i++, fifo_mem += one_fifo_size)
		fifos[i] = fifo_mem;

	poll_groups[0] = fifo_mem;
	stats_mem = fifo_mem + sizeof(*poll_groups[0]);

	poll_group_stats = stats_mem + num_threads *
				sizeof(struct xdp2_fifo_poll_group_stats);

	if (mode == ALL_MODE || mode == SERVER_MODE)
		run_server(stats_mem, poll_group_stats);

	if (mode == ALL_MODE || mode == CLIENT_MODE)
		run_client(count);

	if (mode == ALL_MODE || mode == SERVER_MODE)
		pause();
}

#define ARGS "c:G:o:I:n:NSCvQDK:sk"

static void usage(char *name)
{
	fprintf(stderr, "Usage: %s [-v] [-c <count>] [-I <interval>] ", name);
	fprintf(stderr, "[-G <gshmfile>] [-o <gshfile_offset>] ");
	fprintf(stderr, "[-n <num_threads>] ");
	fprintf(stderr, "[-N] [-R] [-S] [-C] [-Q] [-D] [-s] [-v] [-k]\n");

	exit(-1);
}

int main(int argc, char *argv[])
{
	off_t gshmfile_offset = 0;
	unsigned int count = 1000;
	char *gshmfile = NULL;
	int c;

	while ((c = getopt(argc, argv, ARGS)) != -1) {
		switch (c) {
		case 'c':
			count = strtoul(optarg, NULL, 10);
			break;
		case 'G':
			gshmfile = optarg;
			break;
		case 'o':
			gshmfile_offset = strtoul(optarg, NULL, 0);
			break;
		case 'I':
			interval = strtoul(optarg, NULL, 10);
			break;
		case 'n':
			num_threads = strtoul(optarg, NULL, 10);
			if (num_threads > MAX_THREADS) {
				fprintf(stderr, "Number of threads must be "
						"less than %u\n", MAX_THREADS);
				exit(-1);
			}
			break;
		case 'N':
			no_poll = true;
			break;
		case 'Q':
			no_poll_wait = true;
			break;
		case 's':
			show_size = true;
			break;
		case 'S':
			mode = SERVER_MODE;
			break;
		case 'C':
			mode = CLIENT_MODE;
			break;
		case 'k':
			no_init = true;
			break;
		case 'K':
			ent_size = strtoul(optarg, NULL, 10);
			if (ent_size > MAX_ENT_SIZE) {
				fprintf(stderr, "Ent size too big: %u > %u\n",
					ent_size, MAX_ENT_SIZE);
				exit(1);
			}
			break;
		case 'v':
			verbose = true;
			break;
		default:
			usage(argv[0]);
		}
	}

	xdp2_locks_init_locks();

	run_test(count, gshmfile, gshmfile_offset);
}
