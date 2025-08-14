// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2025, XDPnet
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

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "xdp2/bitmap.h"
#include "xdp2/timer.h"
#include "xdp2/utility.h"

/* Implementation of XDP2 timers. See timer.h for more details */

/* Convert timespec to time units (units of some # of nsecs) */
static unsigned long ts_to_time_units(struct xdp2_timer_wheel *wheel,
				      struct timespec *ts)
{
	return (ts->tv_sec * wheel->time_div) + ts->tv_nsec / wheel->time_units;
}

/* Convert time units (units of some # of nsecs) to timespec */
static void time_units_to_ts(struct xdp2_timer_wheel *wheel,
			     struct timespec *ts, unsigned long tim)
{
	ts->tv_nsec = (tim % wheel->time_div) * wheel->time_units;
	ts->tv_sec = tim / wheel->time_div;
}

/* Tracking mutex macros */
#define TIMER_MUTEX_AT(WHEEL, STR) ((WHEEL)->where = (STR))
#define TIMER_MUTEX_AT_FUNC(WHEEL) ((WHEEL)->where = __func__)

/* Get current time in timer wheel units */
static unsigned long __get_current_time(struct xdp2_timer_wheel *wheel)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts_to_time_units(wheel, &ts);
}

/* Get current time. If a function was provided when the wheel was created
 * then call that, else call the internal function __get_current_time
 */
static unsigned long get_current_time(struct xdp2_timer_wheel *wheel)
{
	return wheel->get_current_time ?
			wheel->get_current_time(wheel->cbarg) :
			__get_current_time(wheel);
}

/* Get the timespec for some delay in timer wheel units */
static void future_ts(struct xdp2_timer_wheel *wheel, struct timespec *ts,
		      unsigned long delay)
{
	unsigned long nsecs;

	clock_gettime(CLOCK_MONOTONIC, ts);

	nsecs = (delay % wheel->time_div) * wheel->time_units + ts->tv_nsec;
	ts->tv_nsec = nsecs % 1000000000;
	ts->tv_sec += (delay / wheel->time_div) + nsecs / 1000000000;
}

/* Set the pop time. Either call an external function or signal the timer
 * thread
 */
static void set_next_timer_pop(struct xdp2_timer_wheel *wheel)
{
	if (wheel->set_next_timer_pop) {
		/* External function was provided when the wheel was
		 * created
		 */
		wheel->set_next_timer_pop(wheel->cbarg,
					  wheel->next_expire_time);
	} else {
		/* Timer thread is running, signal it */
		pthread_cond_signal(&wheel->cond);
	}
}

/* Remove a timer with the timer wheel already locked */
static void timer_remove_locked(struct xdp2_timer_wheel *wheel,
				struct xdp2_timer *timer)
{
	unsigned int index = timer->index;

	XDP2_ASSERT(timer->timer_set, "Timer being removed is not set");

	CIRCLEQ_REMOVE(&wheel->slots[index], timer, list_ent);

	if (CIRCLEQ_EMPTY(&wheel->slots[index]))
		xdp2_bitmap_unset(wheel->slot_bitmap, index);

	timer->timer_set = false;
	wheel->timer_count--;
}

/* Remove timer. If the timer structure is also being freed then
 * xdp2_check_wheel_running should be called before freeing the data structure
 *
 * Takes wheel timer mutex
 */
bool xdp2_timer_remove(struct xdp2_timer_wheel *wheel,
		       struct xdp2_timer *timer)
{
	bool ret;

	pthread_mutex_lock(&wheel->mutex);

	TIMER_MUTEX_AT(wheel, "In remove timer");

	ret = wheel->running;

	if (ret)
		__XDP2_TIMER_BUMP_STAT(wheel, remove_wheel_running);

	if (!timer->timer_set) {
		TIMER_MUTEX_AT(wheel, "Out remove timer");
		pthread_mutex_unlock(&wheel->mutex);
		return ret;
	}

	timer_remove_locked(wheel, timer);

	__XDP2_TIMER_BUMP_STAT(wheel, remove_timers);

	TIMER_MUTEX_AT(wheel, "Out remove timer");

	pthread_mutex_unlock(&wheel->mutex);

	return ret;
}

/* Add a timer
 *
 * Takes timer wheel mutex
 */
void __xdp2_timer_add(struct xdp2_timer_wheel *wheel, struct xdp2_timer *timer,
		      unsigned int delay, bool not_running)
{
	unsigned long current_time;
	unsigned long expire_time;

	/* We don't allow a delay of zero, so just make it 1 */
	delay = delay ? : 1;

	pthread_mutex_lock(&wheel->mutex);

	TIMER_MUTEX_AT(wheel, "In add timer");

	current_time = get_current_time(wheel);

	expire_time = current_time + delay;

	XDP2_ASSERT(xdp2_seqno_ul_gt(expire_time, current_time),
		    "Expire time %lu not greater than current time %lu\n",
		    expire_time, current_time);

	/* If the timer is already set then the timer is being changed
	 * so remove the timer and then re-add it
	 */
	if (timer->timer_set) {
		if (not_running) {
			__XDP2_TIMER_BUMP_STAT(wheel, add_already_running);
			pthread_mutex_unlock(&wheel->mutex);
			return;
		}
		__XDP2_TIMER_BUMP_STAT(wheel, change_timers);
		timer_remove_locked(wheel, timer);
	}

	/* Setup timer */
	timer->gen = expire_time / wheel->num_slots;
	timer->index = expire_time % wheel->num_slots;

	CIRCLEQ_INSERT_HEAD(&wheel->slots[timer->index], timer, list_ent);
	xdp2_bitmap_set(wheel->slot_bitmap, timer->index);
	timer->timer_set = true;
	wheel->timer_count++;

	if (xdp2_seqno_ul_lt(expire_time, wheel->next_expire_time) ||
	    wheel->next_expire_time == wheel->last_runtime) {
		/* The new timer is set to expire before the current pop
		 * timer for the wheel
		 */

		if (wheel->next_expire_time == wheel->last_runtime) {
			/* Pop timer wasn't running */
			wheel->last_runtime = current_time;
		}

		wheel->next_expire_time = expire_time;

		XDP2_ASSERT(xdp2_seqno_ul_gt(wheel->next_expire_time,
					     wheel->last_runtime),
			    "Next expire time on add %lu <= last expire time "
			    "%lu\n",
			    wheel->next_expire_time, wheel->last_runtime);

		/* Note we're calling set_next_timer_pop with the timer
		 * wheel mutex held
		 */
		set_next_timer_pop(wheel);
	}

	__XDP2_TIMER_BUMP_STAT(wheel, add_timers);

	TIMER_MUTEX_AT(wheel, "Out add timer");

	pthread_mutex_unlock(&wheel->mutex);
}

/* Safe version of CIRCLEQ foreach. This allows the entry to be removed in
 * the foreach loop
 */
#define CIRCLEQ_FOREACH_SAFE(var, next, head, field)			\
	for ((var) = (head)->cqh_first,					\
				(next) = (var)->field.cqe_next;		\
		(var) != (const void *)(head);				\
		(var) = (next), next = ((var)->field.cqe_next))

/* Run through the entries of one timer wheel slot
 *
 * Wheel timer mutex is held on entry, released for callbacks, and held
 * again at function return
 */
static unsigned int run_slot(struct xdp2_timer_wheel *wheel, unsigned int index,
			     unsigned long gen, unsigned long old_gen)
{
	unsigned long old_gen_mx = old_gen * wheel->num_slots;
	unsigned long gen_mx = gen * wheel->num_slots;
	struct {
		void (*callback)(void *arg);
		void *arg;
	} cbs[8];
	struct xdp2_timer *timer, *next;
	int count, cb_count = 0, i;

	TIMER_MUTEX_AT(wheel, "In run slot");
retry:
	count = 0;
	CIRCLEQ_FOREACH_SAFE(timer, next, &wheel->slots[index], list_ent) {
		unsigned long t_gen_mx = timer->gen * wheel->num_slots;

		XDP2_ASSERT(xdp2_seqno_ul_gte(t_gen_mx, old_gen_mx),
			    "Found timer in wheel with old generation "
			    "%lu < %lu", t_gen_mx, old_gen_mx);

		if (xdp2_seqno_ul_gt(t_gen_mx, gen_mx)) {
			/* Still in the future */
			continue;
		}

		/* Good expiration, remove from timer wheel */
		CIRCLEQ_REMOVE(&wheel->slots[index], timer, list_ent);
		timer->timer_set = false;
		wheel->timer_count--;

		count++;

		/* We want to make the callback with the mutex held which means
		 * that timers could be added or emoved before returning. To
		 * keep everything sane we batch up eight callbacks. If we
		 * find all eight then unlock, invoke, and then reprocess the
		 * slot from the beginning
		 */
		cbs[cb_count].callback =  timer->callback;
		cbs[cb_count].arg =  timer->arg;

		if (++cb_count >= 8) {
			/* We got a full batch */

			/* Unlock mutex for calling callbacks */
			pthread_mutex_unlock(&wheel->mutex);
			for (i = 0; i < cb_count; i++)
				cbs[i].callback(cbs[i].arg);
			pthread_mutex_lock(&wheel->mutex);

			__XDP2_TIMER_BUMP_STAT(wheel, slot_retries);
			cb_count = 0;
			TIMER_MUTEX_AT(wheel, "Retry run slot");
			goto retry;
		}
	}

	if (CIRCLEQ_EMPTY(&wheel->slots[index]))
		xdp2_bitmap_unset(wheel->slot_bitmap, index);

	/* Unlock mutex for calling callbacks */
	if (cb_count) {
		TIMER_MUTEX_AT(wheel, "Start time callbacks");
		pthread_mutex_unlock(&wheel->mutex);
		for (i = 0; i < cb_count; i++)
			cbs[i].callback(cbs[i].arg);
		pthread_mutex_lock(&wheel->mutex);
		TIMER_MUTEX_AT(wheel, "End timer callbacks");
	}

	TIMER_MUTEX_AT(wheel, "Out run slot");

	return count;
}

/* Process the timer wheel. This is called form the pop timer
 *
 * The wheel timer mutex is acquired. It's released when calling timer
 * callback functions
 */
static void __xdp2_run_timer_wheel_locked(struct xdp2_timer_wheel *wheel)
{
	unsigned int index, last_index, count = 0;
	unsigned long current_time, old_gen, new_gen;

	TIMER_MUTEX_AT(wheel, "Run timer wheel locked enter");
retry:
	current_time = get_current_time(wheel);

	if (wheel->running) {
		/* Another thread is already running the wheel (this should
		 * be very rare). Mark that we want the wheel to be run again
		 * and return
		 */
		wheel->run_again = true;
		TIMER_MUTEX_AT(wheel, "Run timer wheel already running");
		return;
	}

	wheel->running = true;

	__XDP2_TIMER_BUMP_STAT(wheel, run_timer_wheel);

	/* Compute current generation and new generation */
	old_gen = wheel->last_runtime / wheel->num_slots;
	new_gen = (current_time + 1) / wheel->num_slots;

	XDP2_ASSERT(xdp2_seqno_ul_lte(wheel->last_runtime, current_time),
		    "Last runtime %lu is > current time %lu\n",
		    wheel->last_runtime, current_time);

	index = wheel->last_runtime % wheel->num_slots;

	if (old_gen == new_gen) {
		/* Still on the old generation. Start from the previous
		 * last run slot
		 */
		__XDP2_TIMER_BUMP_STAT(wheel, timer_wheel_same_gen);
	} else if (old_gen + 1 == new_gen) {
		/* Rolled over one generation. Process the remainder of the
		 * previous generation and then reset index for the current
		 * generation starting from zero
		 */
		TIMER_MUTEX_AT(wheel, "Run next gen slot start");
		xdp2_bitmap_foreach_bit(wheel->slot_bitmap, index,
					wheel->num_slots)
			count += run_slot(wheel, index, new_gen - 1, old_gen);
		index = 0;
		__XDP2_TIMER_BUMP_STAT(wheel, timer_wheel_next_gen);
		TIMER_MUTEX_AT(wheel, "Run next gen slot end");
	} else {
		/* We skipped more than one  to a new generation, do a full
		 * sweep of all timers through the new generation minus one.
		 * Then handle the current generation starting from a
		 * zero index
		 */
		TIMER_MUTEX_AT(wheel, "Run next skips gen slot start");
		index = 0;
		xdp2_bitmap_foreach_bit(wheel->slot_bitmap, index,
					wheel->num_slots)
			count += run_slot(wheel, index, new_gen - 1, old_gen);
		index = 0;
		__XDP2_TIMER_BUMP_STAT(wheel, timer_wheel_skip_gens);
		TIMER_MUTEX_AT(wheel, "Run next skip gen slots end");
	}

	TIMER_MUTEX_AT(wheel, "Run current gen slot start");
	last_index = (current_time + 1) % wheel->num_slots;
	xdp2_bitmap_foreach_bit(wheel->slot_bitmap, index,
				last_index)
		count += run_slot(wheel, index, new_gen, old_gen);

	wheel->last_runtime = current_time;

	__XDP2_TIMER_ADD_STAT(wheel, timer_expires, count);
	TIMER_MUTEX_AT(wheel, "Run current gen slot end");

	/* Try to find next timeout */
	index = xdp2_bitmap_find(wheel->slot_bitmap, last_index,
				 wheel->num_slots);
	if (index < wheel->num_slots) {
		/* Next timer found in current generation. Note that the
		 * timers for the slot might be for a later generation in
		 * which case a spurious timeout will happen
		 */
		wheel->next_expire_time = new_gen * wheel->num_slots + index;
		set_next_timer_pop(wheel);
	} else {
		index = xdp2_bitmap_find(wheel->slot_bitmap, 0, last_index);
		if (index < last_index) {
			/* Next timer found in next generation. Note that the
			 * timers for the slot might be for a later generation
			 * in which case a spurious timeout will happen
			 */
			wheel->next_expire_time = (new_gen + 1) *
						wheel->num_slots + index;
			set_next_timer_pop(wheel);
		} else {
			/* No timers set in the wheel. Don't set
			 * set the pop timer
			 */
		}
	}

	if (!count)
		__XDP2_TIMER_BUMP_STAT(wheel, spurious_run_timer_wheel);

	if (wheel->run_again) {
		/* Another thread tried to run the wheel, retry the whole
		 * thing now. This is expected to be rare
		 */
		wheel->run_again = false;
		wheel->running = false;
		count = 0;
		__XDP2_TIMER_BUMP_STAT(wheel, run_timer_wheel_retries);

		TIMER_MUTEX_AT(wheel, "Run timer wheel retry");

		goto retry;
	}

	wheel->running = false;

	TIMER_MUTEX_AT(wheel, "Run timer wheel locked end");
}

/* Frontend function to run the timer wheel
 *
 * The timer wheel mutex is acquired
 */
void xdp2_run_timer_wheel(struct xdp2_timer_wheel *wheel)
{
	TIMER_MUTEX_AT_FUNC(wheel);

	pthread_mutex_lock(&wheel->mutex);
	TIMER_MUTEX_AT(wheel, "Before run timer wheel with lock");
	__xdp2_run_timer_wheel_locked(wheel);
	TIMER_MUTEX_AT(wheel, "After run timer wheel with lock");
	pthread_mutex_unlock(&wheel->mutex);
}

/* Timer threads */

/* Timer thread. This runs a single timer in a loop and calls
 * __xdp2_run_timer_wheel_locked when the timer expires
 */
static void *timer_thread(void *arg)
{
	struct xdp2_timer_wheel *wheel = arg;
	struct timespec ts;
	int ret;

	TIMER_MUTEX_AT_FUNC(wheel);

	pthread_mutex_lock(&wheel->mutex);

	/* Initialize to run timer immediately (doesn't matter to much,
	 * just want to make sure timer is always running
	 */
	wheel->last_runtime = get_current_time(wheel);
	wheel->next_expire_time =  wheel->last_runtime + 1;

	while (1) {
		time_units_to_ts(wheel, &ts, wheel->next_expire_time);

		TIMER_MUTEX_AT(wheel, "Time thread before timedwait");

		XDP2_ASSERT(xdp2_seqno_ul_gt(wheel->next_expire_time,
					     wheel->last_runtime),
			    "Next expire time %lu <= last expire time %lu\n",
			    wheel->next_expire_time, wheel->last_runtime);

		if (!(ret = pthread_cond_timedwait(&wheel->cond,
					   &wheel->mutex, &ts))) {
			/* The thread was signaled indicating that the
			 * next pop time was changed, Just restart the wait
			 * with the latest pop time
			 */
			__XDP2_TIMER_BUMP_STAT(wheel, timer_thread_signaled);
			continue;
		}

		if (ret != ETIMEDOUT) {
			/* Got an unexpected error */

			XDP2_ERR(1, "Timer thread failed on %s",
				 strerror(ret));
		}

		TIMER_MUTEX_AT(wheel, "Timer thread after timedwait");

		/* Pop timer expired, run the timer wheel */

		/* Preemptively set next timeout for ten seconds just to keep
		 * things running. It's very likely this will be overridden
		 * by and add_timer
		 */
		future_ts(wheel, &ts, 10UL * 1000000000 / wheel->time_units);
		wheel->next_expire_time = ts_to_time_units(wheel, &ts);

		TIMER_MUTEX_AT(wheel, "Before run timer wheel");

		/* Run the timer wheel without holding the mutex */
		__xdp2_run_timer_wheel_locked(wheel);

		TIMER_MUTEX_AT(wheel, "After run timer wheel");
	}

	return NULL;
}

/* Malloc and initialize a timer wheel */
static struct xdp2_timer_wheel *__xdp2_timer_create_wheel(
		unsigned int num_slots_order, void *cbarg,
		unsigned long (*get_current_time)(void *arg),
		void (*set_next_timer_pop)(void *arg,
					   unsigned long expire_time),
		unsigned int time_units)
{
	unsigned int num_slots = (1 << num_slots_order);
	struct xdp2_timer_wheel *wheel;
	size_t base_size = sizeof(*wheel) + (1 << num_slots_order) *
			sizeof(struct __xdp2_timer_wheel_list_head);
	size_t all_size = base_size + sizeof(unsigned long) *
			XDP2_BITMAP_NUM_BITS_TO_WORDS(num_slots);
	int i;

	if (!time_units) {
		XDP2_WARN("Time units must not be zero in timer wheel create");
		return NULL;
	}

	wheel = malloc(all_size);
	if (!wheel)
		return NULL;

	/* Initialize timer wheel */
	wheel->num_slots = num_slots;
	wheel->slot_bitmap = (unsigned long *)((void *)wheel + base_size);
	wheel->cbarg = cbarg;
	wheel->get_current_time = get_current_time;
	wheel->set_next_timer_pop = set_next_timer_pop;
	wheel->time_units = time_units;
	wheel->time_div = 1000000000 / time_units;
	wheel->where = "No info";

	pthread_mutex_init(&wheel->mutex, NULL);

	/* Initialize the slots */
	for (i = 0; i < num_slots; i++)
		CIRCLEQ_INIT(&wheel->slots[i]);

	return wheel;
}

/* Create timer wheel with external pop timer (no using timer thread) */
struct xdp2_timer_wheel *xdp2_timer_create_wheel(
		unsigned int num_slots_order, void *cbarg,
		unsigned long (*get_current_time)(void *arg),
		void (*set_next_timer_pop)(void *arg,
					   unsigned long expire_time),
		unsigned int time_units)
{
	if (!set_next_timer_pop) {
		XDP2_WARN("set_next_timer_pop must be non-null in "
			  "xdp2_timer_create_wheel");
		return NULL;
	}

	return __xdp2_timer_create_wheel(num_slots_order, cbarg,
					 get_current_time, set_next_timer_pop,
					 time_units);
}

/* Create timer wheel with external and start timer thread) */
struct xdp2_timer_wheel *xdp2_timer_create_wheel_with_timer_thread(
		unsigned int num_slots_order, unsigned int time_units)
{
	struct xdp2_timer_wheel *wheel;
	pthread_condattr_t condattr;
	pthread_t thread_id;
	struct timespec ts;

	wheel = __xdp2_timer_create_wheel(num_slots_order, NULL, NULL, NULL,
					  time_units);
	if (!wheel)
		goto fail_wheel_create;

	wheel->cbarg = wheel;
	wheel->time_units = time_units;

	/* Initialize timer to ten second from now */
	future_ts(wheel, &ts, 10UL * 1000000000 / wheel->time_units);
	wheel->next_expire_time = ts_to_time_units(wheel, &ts);

	if (pthread_condattr_init(&condattr))
		goto fail_condattr;

	if (pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC))
		goto fail_setclock;

	if (pthread_cond_init(&wheel->cond, &condattr))
		goto fail_cond_init;

	if (pthread_create(&thread_id, NULL, timer_thread, wheel))
		goto fail_thread_create;

	return wheel;

fail_thread_create:
	pthread_cond_destroy(&wheel->cond);
fail_cond_init:
fail_setclock:
	pthread_condattr_destroy(&condattr);
fail_condattr:
	free(wheel);
fail_wheel_create:
	return NULL;
}

/* Show timers on cli
 *
 * Takes the timer wheel mutex for each slot processed
 */
static void show_timers(struct xdp2_timer_wheel *wheel, void *cli)
{
	struct xdp2_timer *timer;
	unsigned int index = 0;

	/* We don't want to hold the wheel timer mutex for the whole
	 * function, just when processing each slot. This does mean that
	 * some of the output might be a little inconsistent due to race
	 * conditions (it's just debug info afterall)
	 */
	xdp2_bitmap_foreach_bit(wheel->slot_bitmap, index,
				wheel->num_slots) {
		XDP2_CLI_PRINT(cli, "Slot %u:\n", index);

		/* We take the wheel timer mutex for processing each slot
		 * to ensure pointer consistency
		 */
		pthread_mutex_lock(&wheel->mutex);

		CIRCLEQ_FOREACH(timer, &wheel->slots[index], list_ent)
			XDP2_CLI_PRINT(cli, "\tTimer: gen: %lu index: %u, "
				       "expire-time: %lu\n", timer->gen,
				       timer->index,
				       (timer->gen * wheel->num_slots) +
								timer->index);

		pthread_mutex_unlock(&wheel->mutex);
	}
}

/* Show timers on cli
 *
 * Takes the timer wheel mutex for each slot processed
 */
static void show_timer_wheel(struct xdp2_timer_wheel *wheel, bool show_all,
			     void *cli)
{
	struct xdp2_timer_wheel_stats *stats = &wheel->stats;

	XDP2_CLI_PRINT(cli, "Timer wheel: %s, timer_count %u\n",
		       wheel->last_runtime == wheel->next_expire_time ?
						"not running" : "running",
		       wheel->timer_count);
	XDP2_CLI_PRINT(cli, "    Current time: %lu\n",
		       get_current_time(wheel));
	XDP2_CLI_PRINT(cli, "    Last runtime %lu, Next expire time: %lu\n",
		       wheel->last_runtime, wheel->next_expire_time);
	XDP2_CLI_PRINT(cli, "    Add timers: %lu, Remove timers: %lu\n",
		       stats->add_timers, stats->remove_timers);
	XDP2_CLI_PRINT(cli, "    Change timers: %lu, Add already running %lu\n",
		       stats->change_timers, stats->add_already_running);
	XDP2_CLI_PRINT(cli, "    Timer expires: %lu, timer wheel runs: %lu\n",
		       stats->timer_expires, stats->run_timer_wheel);
	XDP2_CLI_PRINT(cli, "    Slot retries: %lu, timer mutex where: %s\n",
		       stats->slot_retries, wheel->where);
	XDP2_CLI_PRINT(cli, "    Timer wheel runs: %lu, retries: %lu\n",
		       stats->run_timer_wheel, stats->run_timer_wheel_retries);
	XDP2_CLI_PRINT(cli, "    Spurious runs: %lu, remove with running %lu\n",
		       stats->spurious_run_timer_wheel,
		       stats->remove_wheel_running);
	XDP2_CLI_PRINT(cli, "    Timer wheel same gen: %lu, "
			    "next gen: %lu, skip gens: %lu\n",
		       stats->timer_wheel_same_gen,
		       stats->timer_wheel_next_gen,
		       stats->timer_wheel_skip_gens);

	if (show_all)
		show_timers(wheel, cli);
}

void xdp2_timer_show_wheel(struct xdp2_timer_wheel *wheel, void *cli)
{
	show_timer_wheel(wheel, false, cli);
}

void xdp2_timer_show_wheel_all(struct xdp2_timer_wheel *wheel, void *cli)
{
	show_timer_wheel(wheel, true, cli);
}
