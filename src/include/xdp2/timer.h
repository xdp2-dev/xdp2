/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2025 XDPnet
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

#ifndef __XDP2_TIMER_H__
#define __XDP2_TIMER_H__

/* XDP2 timers
 *
 * This is a fairly simple timer wheel implementation. An instance of a
 * timer is defined by the timer_wheel structure. Like most timer wheel
 * implementations this contains an array of linked list timer structures
 *
 * A timer wheel has no defined granularity. Granaularity is a property
 * applied by the user of the timer wheel. For instance, an application
 * might define several timer wheels for different granularities: like
 * nsces, usces, msecs, etc. To support this, there are two callbacks
 * set for a timer wheel:
 *	- get_current_time: Returns the current time in the units of
 *	  granularity for the timer wheel. For instance, if the implied
 *	  granularity is milliseconds then the time is returned in
 *	  milliseconds
 *	- set_next_timer_pop: Set the next time that the application should
 *	  call xdp2_run_timer
 *
 * The timers are processed for expiration when xdp2_run_timer_wheel is
 * called. This can be driven by a pop_timer maintained by the application.
 * For instance, the pop timer could be a running interval timer or could
 * be a programmed hardware time. The timer_wheel implementation invokes the
 * set_next_timer_pop callback to indicate the next time that the timer wheel
 * should be run (that is the time of the next timer that will expire)
 *
 * A timer is specified by the xdp2_timer structure. In the API, the
 * timer structure is provided by the caller. A timer structure should be
 * initialized to zeros, and the callback and argument fields should be
 * set by the caller
 *
 * The API functions are:
 *	- xdp2_timer_create_wheel: Mallocs and initializes a timer wheel.
 *	  Arguments include number of slots, the get_current_time and
 *	  set_next_timer_pop functions
 *	- xdp2_timer_add: Add a timer with some delay time to fire ((relative
 *	  to the current time, not absolute time). Arguments are the
 *	  timer wheel, the timer structure, and the expiration time. If the
 *	  timer is already running then it's removed and re-added
 *	- xdp2_timer_remove: Remove a timer from the timer wheel. Arguments
 *	  are the timer wheel and the timer structure. Note that the timer
 *	  structure must be the same one that was added
 *	- xdp2_run_timer_wheel: Run the timer wheel. Scan the timer wheel
 *	  and for any expired timers (i.e. their expiration time is less than
 *	  or equal to the current time), call the associated callback
 *	  function and remove the timer from the timer wheel
 *	- xdp2_timer_show_wheel: Show the basic info and stats for a timer
 *	  wheel
 *	- xdp2_timer_show_wheel_All: Show the basic info and stats for a timer
 *	  wheel all the active timers
 *
 * When a timer is removed there is still a possibility that the timer
 * may fire. Care must be taken to properly handle this especially if
 * the timer data structure is being freed.
 *
 * 1. If xdp2_timer_remove is called on a timer from within the timer's
 *    callback then the timer structure is free-able
 * 2. If xdp2_timer_remove returns false (not called from a callback) then
 *    the timer structure is free-able
 * 3. If xdp2_timer remove returns true when not called from the timer's
 *    callback then the timer structure is not-freeable. It is recommended
 *    that another add_timer be done with timeout of 0 delay. When the
 *    timer fires xdp2_timer_rmove should be call from the callback on
 *    the timer (just in case the callabck was for an old timeout), and
 *    then the structure is free-able per #1
 */

#include <linux/types.h>
#include <sys/queue.h>
#include <stdbool.h>

#include "xdp2/locks.h"

/* Use a circle queue for each slot. The elements are doubly linked for
 * fast removal from the list
 */
CIRCLEQ_HEAD(__xdp2_timer_wheel_list_head, xdp2_timer);

/* Timer structure */
struct xdp2_timer {
	/* User define callback and argument */
	void (*callback)(void *arg);
	void *arg;

	/* Timer is set and is in timer wheel */
	bool timer_set;

	/* Generation and index
	 *
	 * Expiration time is (gen * wheel->num_slots) + index
	 */
	unsigned long gen;
	unsigned int index;

	/* List glue */
	CIRCLEQ_ENTRY(xdp2_timer) list_ent;
};

struct xdp2_timer_wheel_stats {
	unsigned long add_timers;
	unsigned long add_already_running;
	unsigned long remove_timers;
	unsigned long change_timers;
	unsigned long timer_expires;

	unsigned long run_timer_wheel;
	unsigned long slot_retries;
	unsigned long timer_thread_signaled;
	unsigned long run_timer_wheel_retries;
	unsigned long spurious_run_timer_wheel;
	unsigned long remove_wheel_running;

	unsigned long timer_wheel_same_gen;
	unsigned long timer_wheel_next_gen;
	unsigned long timer_wheel_skip_gens;
};

#define __XDP2_TIMER_BUMP_STAT(WHEEL, NAME) do {			\
	struct xdp2_timer_wheel *_wheel = (WHEEL);			\
									\
	_wheel->stats.NAME++;						\
} while (0)

#define __XDP2_TIMER_ADD_STAT(WHEEL, NAME, NUM) do {			\
	struct xdp2_timer_wheel *_wheel = (WHEEL);			\
									\
	_wheel->stats.NAME += (NUM);					\
} while (0)

/* Timer wheel structure */
struct xdp2_timer_wheel {
	/* Callback argument set by application */
	void *cbarg;

	/* Get current time callback, Called without the timer wheel lock
	 * held
	 */
	unsigned long (*get_current_time)(void *cbarg);

	/* Set next timer pop time. When this pops xdp2_run_timer is run.
	 * Note that this is called with the timer wheel lock held so the
	 * timer wheel is not reentrant
	 */
	void (*set_next_timer_pop)(void *cbarg, unsigned long expire_time);

	/* Number of slots */
	int num_slots;

	const char *where;

	/* Bitmap of active slots */
	unsigned long *slot_bitmap;

	/* Next expiration time */
	unsigned long next_expire_time;

	/* Last time the xdp2_run_timer ran */
	unsigned long last_runtime;

	/* Count of active timers */
	int timer_count;

	/* Lock for the timer wheel */
	pthread_mutex_t mutex;

	/* Timer wheel running (timers are being scanned) */
	bool running;

	/* Timer wheel is running and got another request to run again */
	bool run_again;

	/* Timer wheel statistics */
	struct xdp2_timer_wheel_stats stats;

	/* Fields for timer thread */
	pthread_cond_t cond;
	unsigned int time_units;
	unsigned int time_div;
	unsigned long next_pop_time;

	/* Timer wheel slots */
	struct __xdp2_timer_wheel_list_head slots[];
};

/* Create timer wheel */
struct xdp2_timer_wheel *xdp2_timer_create_wheel(
		unsigned int num_slots_order, void *cbarg,
		unsigned long (*get_current_time)(void *cbarg),
		void (*set_next_timer_pop)(void *cbarg,
					   unsigned long expire_time),
		unsigned int time_units);

/* Create timer wheel with a timer thread to drive the clock */
struct xdp2_timer_wheel *xdp2_timer_create_wheel_with_timer_thread(
		unsigned int num_slots_order,
		unsigned int time_units);

void __xdp2_timer_add(struct xdp2_timer_wheel *wheel, struct xdp2_timer *timer,
		      unsigned int delay, bool not_running);


/* Add (or change) a timer */
static inline void xdp2_timer_add(struct xdp2_timer_wheel *wheel,
				  struct xdp2_timer *timer,
				  unsigned int delay)
{
	__xdp2_timer_add(wheel, timer, delay, false);
}

/*Conditionally set a timer if it's not already running */
static inline void xdp2_timer_add_not_running(struct xdp2_timer_wheel *wheel,
					      struct xdp2_timer *timer,
					      unsigned int delay)
{
	__xdp2_timer_add(wheel, timer, delay, true);
}


/* Remove a timer. Returns true if timer wheel is running allowing the
 * possiblity that the removed timer could still fire. Note that this
 * won't happen if the timer being removed is one from a callback
 */
bool xdp2_timer_remove(struct xdp2_timer_wheel *wheel,
		       struct xdp2_timer *timer);

/* Run the timer wheel (when the next timer pops */
void xdp2_run_timer_wheel(struct xdp2_timer_wheel *wheel);

/* Show timer wheel on cli */
void xdp2_timer_show_wheel(struct xdp2_timer_wheel *wheel, void *cli);

/* Show timer wheel with timers on cli */
void xdp2_timer_show_wheel_all(struct xdp2_timer_wheel *wheel, void *cli);

#endif /* __XDP2_TIMER_H__ */
