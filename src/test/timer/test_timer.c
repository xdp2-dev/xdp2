// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/* * Copyright (c) 2025 XDPnet
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

/* Test for timers
 *
 * Run: ./test_timers [ -c <test-count> ] [ -v <verbose> ]
 *		      [ -I <report-interval> ][ -C <cli_port_num> ]
 *		      [-R] [ -s <sleep-time> ] [-P <prompt-color> ]
 *		      [ -n <num-timers> ] [ -t <num-threads>] [-u]
 *		      [ -T <time-units> ] [ -x <num-sub> ]
 */

#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xdp2/cli.h"
#include "xdp2/timer.h"
#include "xdp2/utility.h"

#define MAX_THREADS 10

/* Force the start of time to be just south max unsigned long to test
 * rollover
 */
#define START_TIME (-1UL - 10000000)

unsigned long my_time = START_TIME;
unsigned int clock_sleep_mod = 5;

unsigned int verbose;

/* Timer wheel for test */
struct xdp2_timer_wheel *main_wheel;

/* Pop timer variables */
unsigned long pop_time;
bool pop_running;

/* Get current time */
static unsigned long get_current_time(void *cbarg)
{
	if (verbose >= 15)
		printf("Get current time %lu\n", my_time);

	return my_time;
}

/* Set the next pop timer */
static void set_next_timer_pop(void *cbarg, unsigned long next_pop)
{
	if (verbose >= 15)
		printf("Set pop time %lu\n", next_pop);

	pop_time = next_pop;
	pop_running = true;
}

struct my_timer {
	struct xdp2_timer timer;
	unsigned int thread_num;
	unsigned int timer_num;
};

/* Timer callback */
static void callback(void *arg)
{
	struct my_timer *mtim = arg;

	if (verbose >= 10)
		printf("Got timer %u:%u\n", mtim->thread_num, mtim->timer_num);
}

/* Manually advance time */
static void advance_time(struct xdp2_timer_wheel *wheel,
			 unsigned long adv)
{
	my_time += adv;

	if (pop_running && xdp2_seqno_ul_gte(my_time, pop_time)) {
		pop_running = false;
		xdp2_run_timer_wheel(wheel);
	}
}

struct test_add {
	unsigned long count;
	int num_timers;
	int thread_num;
	unsigned int interval;
};

/* Run one thread that adds and removes timers */
static void *run_one_test(void *arg)
{
	struct test_add *test = arg;
	struct my_timer *timers;
	unsigned int delay;
	unsigned long i;
	int t;

	timers = calloc(test->num_timers, sizeof(*timers));

	for (i = 0; i < test->num_timers; i++) {
		timers[i].thread_num = test->thread_num;
		timers[i].timer_num = i;
		timers[i].timer.callback = callback;
		timers[i].timer.arg = &timers[i];
	}

	for (i = 0; i < test->count; i++) {
		if (test->interval && (i % test->interval) == 0)
			printf("T%u: %lu\n", test->thread_num, i);

		t = rand() % test->num_timers;

		delay = rand() % (3 * main_wheel->num_slots) + 1;

		/* Add (change) timer 1/2 time, remove timer the other half */
		switch (rand() % 3) {
		case 0:
			xdp2_timer_add(main_wheel, &timers[t].timer, delay);
			break;
		case 1:
			xdp2_timer_add_not_running(main_wheel,
						   &timers[t].timer, delay);
			break;
		case 2:
		default:
			xdp2_timer_remove(main_wheel, &timers[t].timer);
			break;
		}
	}

	return NULL;
}

/* Run the clock thread */
static void *run_clock(void *arg)
{
	unsigned int adv;

	/* Run a loop where every iteration we emulate time advancing by
	 * up to two times the number of slots
	 */
	while (1) {
		if (rand() % 100 == 0)
			main_wheel->run_again = true;
		adv = rand() % (2 * main_wheel->num_slots) + 1;
		advance_time(main_wheel, adv);

		/* Sleep a bit */
		usleep(rand() % clock_sleep_mod);
	}

	return NULL;
}

/* Run the test */
static void run_test(unsigned long count, int num_timers,
		     int num_threads, unsigned int interval,
		     bool use_timer_thread, unsigned int time_units)
{
	pthread_t test_id[10], clock_id;
	struct test_add test[10];
	int i;

	if (!use_timer_thread)
		main_wheel = xdp2_timer_create_wheel(7, NULL,
						     get_current_time,
						     set_next_timer_pop,
						     time_units);
	else
		main_wheel = xdp2_timer_create_wheel_with_timer_thread(
				7, time_units);

	for (i = 0; i < num_threads; i++) {
		test[i].count = count;
		test[i].num_timers = num_timers;
		test[i].thread_num = i;
		test[i].interval = interval;

		if (pthread_create(&test_id[i], NULL, run_one_test, &test[i])) {
			perror("pthread_create failed");
			exit(1);
		}
	}

	if (!use_timer_thread) {
		if (pthread_create(&clock_id, NULL, run_clock, NULL)) {
			perror("pthread_create failed");
			exit(1);
		}
	}

	for (i = 0; i < num_threads; i++)
		pthread_join(test_id[i], NULL);
}

/* CLI commands */

XDP2_CLI_MAKE_NUMBER_SET(verbose, verbose);

static void show_timer_wheel(void *cli,
			     struct xdp2_cli_thread_info *info, const char *arg)
{
	xdp2_timer_show_wheel(main_wheel, cli);
}

XDP2_CLI_ADD_SHOW_CONFIG("wheel", show_timer_wheel, 0xffff);

static void show_timers_all(void *cli,
			    struct xdp2_cli_thread_info *info, const char *arg)
{
	xdp2_timer_show_wheel_all(main_wheel, cli);
}

XDP2_CLI_ADD_SHOW_CONFIG("timers", show_timers_all, 0xffff);

#define ARGS "c:v:I:C:Rs:P:t:T:n:x:u"

static void *usage(char *prog)
{

	fprintf(stderr, "Usage: %s [ -c <test-count> ] [ -v <verbose> ]\n",
		prog);
	fprintf(stderr, "\t[ -I <report-interval> ][ -C <cli_port_num> ]\n");
	fprintf(stderr, "\t[-R] [ -s <sleep-time> ]\n");
	fprintf(stderr, "\t[ -P <prompt-color> ] [ -n <num-timers> ]\n");
	fprintf(stderr, "\t[ -t <num-threads> ] [-u] [ -T <time-units> ]\n");
	fprintf(stderr, "\t[ -x <num-sub> ]\n");

	exit(-1);
}

int main(int argc, char *argv[])
{
	unsigned int cli_port_num = 0, interval = 0, num_timers = 100;
	static struct xdp2_cli_thread_info cli_thread_info;
	int c, sleep_time = 0, num_threads = 1;
	unsigned long count = 1000000000;
	const char *prompt_color = "";
	bool use_timer_thread = false;
	unsigned int time_units = 1;
	bool random_seed = false;
	unsigned long time_sub;

	while ((c = getopt(argc, argv, ARGS)) != -1) {
		switch (c) {
		case 'c':
			count = strtoll(optarg, NULL, 10);
			break;
		case 'v':
			verbose = strtol(optarg, NULL, 10);
			break;
		case 'I':
			interval = strtol(optarg, NULL, 10);
			break;
		case 'C':
			cli_port_num = strtoul(optarg, NULL, 10);
			break;
		case 'R':
			random_seed = true;
			break;
		case 's':
			sleep_time = strtol(optarg, NULL, 10);
			break;
		case 'P':
			prompt_color = xdp2_print_color_select_text(optarg);
			break;
		case 'n':
			num_timers = strtol(optarg, NULL, 10);
			break;
		case 't':
			num_threads = strtol(optarg, NULL, 10);
			break;
		case 'u':
			use_timer_thread = true;
			break;
		case 'T':
			time_units = strtol(optarg, NULL, 10);
			break;
		case 'x':
			time_sub = strtoll(optarg, NULL, 10);
			my_time = -1UL - time_sub;
			break;
		default:
			usage(argv[0]);
		}
	}

	if (num_threads > MAX_THREADS) {
		fprintf(stderr, "Too many threads: %u > %u\n",
			num_threads, MAX_THREADS);
		usage(argv[0]);
	}

	if (random_seed)
		srand(time(NULL));

	if (cli_port_num) {
		XDP2_CLI_SET_THREAD_INFO(cli_thread_info, cli_port_num,
			(.label = "xdp2_parse dump",
			 .prompt_color = prompt_color,
			)
		);

		xdp2_cli_start(&cli_thread_info);
	}

	run_test(count, num_timers, num_threads, interval, use_timer_thread,
		 time_units);

	sleep(sleep_time);
}
