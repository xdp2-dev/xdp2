/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2026 XDPnet Inc.
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

#ifndef __XDP2_FIFO_H__
#define __XDP2_FIFO_H__

#include <pthread.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include "xdp2/addr_xlat.h"
#include "xdp2/bitmap.h"
#include "xdp2/locks.h"
#include "xdp2/utility.h"

#ifdef XDP2_FIFO_LOCK_DEBUG
#define XDP2_FIFO_MUTEX_LOCK(LOCK, TEXT)			\
	XDP2_LOCKS_MUTEX_LOCK_DEBUG(LOCK, xdp2_locks_debug_count, TEXT)
#define XDP2_FIFO_MUTEX_UNLOCK(LOCK)				\
	XDP2_LOCKS_MUTEX_UNLOCK_DEBUG(LOCK)
#else
#define XDP2_FIFO_MUTEX_LOCK(LOCK, TEXT) XDP2_LOCKS_MUTEX_LOCK(LOCK)
#define XDP2_FIFO_MUTEX_UNLOCK(LOCK) XDP2_LOCKS_MUTEX_UNLOCK(LOCK)
#endif

/* General fifo with producer and consumer pointers, maximum size and
 * low water mark. Each FIFO has a producer, represented by a write FIFO,
 * and a consumer represented by a read FIFO
 *
 * The queue is an array of __u64's that is managed is a ring based on the
 * producer and consumer pointers
 */

/* Producer stats structure. This structure contains counters for various
 * events related to the producer side of a FIFO
 */
struct xdp2_fifo_prod_stats {
	/* Counters that are incremented for all FIFO operations including
	 * hardware and software FIFOs
	 */

	/* Number of enqueue operations attempted on the FIFO */
	atomic_ulong requests;

	/* The enqueue failed. The FIFO is full and the wait argument is not
	 * set
	 */
	atomic_ulong fails;

	/* Wait on FIFO. The FIFO is full and the wait argument is set */
	atomic_ulong waits;

	/* Spurious wake from poll. Poll returned a writable FIFO, but a
	 * subsequent enqueue operation encountered a full queue. Note that
	 * this counter is incremented from the caller of enqueue function
	 * (the caller did xdp2_fifo_poll, xdp2_fifo_enqueue with no wait on
	 * the FIFO returned by the poll, and enqueue failed because the FIFO
	 * was again full)
	 */
	atomic_ulong spurious_wakeups_poll;

	/* Counters that are incremented for software FIFOs or emulated
	 * hardware FIFOs
	 */

	/* Fifo is full at enqueue. The enqueue fails if wait argument is not
	 * set, else it will wait when the wait argument is set
	 */
	unsigned long blocked;

	/* FIFO became full as a result of the enqueue operation */
	unsigned long queue_full;

	/* Spurious wakeup in enqueue. Conditional wait returned, but FIFO
	 * was still full
	 */
	unsigned long spurious_wakeups;

	/* After conditional wait in enqueue function */
	unsigned long after_wait;

	/* Enqueue operation on empty queue. The consumer will be signaled */
	unsigned long saw_empty;

	/* Maximum number of elements ever enqueued */
	unsigned int max_enqueued;
};

/* Consumer stats structure. This structure contains counters for various
 * events related to the consumer side of a FIFO
 */
struct xdp2_fifo_cons_stats {
	/* Counters that are incremented for all FIFO operations including
	 * hardware and software FIFOs
	 */

	/* Number of dequeue operations attempted on the FIFO */
	atomic_ulong requests;

	/* The dequeue failed. The FIFO is empty and the wait argument is not
	 * set
	 */
	atomic_ulong fails;

	/* Wait on FIFO. The FIFO is empty and the wait argument is set */
	atomic_long waits;

	/* Spurious wake from poll. Poll returned a readable FIFO, but a
	 * subsequent dequeue operation encountered an empty queue. Note that
	 * this counter is incremented from the caller of dequeue function
	 * (the caller did xdp2_fifo_poll, xdp2_fifo_enqueue with no wait on
	 * the FIFO returned by the poll, and dequeue failed because the FIFO
	 * was again empty)
	 */
	atomic_ulong spurious_wakeups_poll;

	/* Counters that are incremented for software FIFOs or emulated
	 * hardware FIFOs
	 */

	/* Fifo is empty at dequeue. The dequeue fails if wait argument is not
	 * set, else it will wait when the wait argument is set
	 */
	unsigned long blocked;

	/* Spurious wakeup in dequeue. Conditional wait returned, but FIFO
	 * was still empty
	 */
	unsigned long spurious_wakeups;

	/* After conditional wait in dequeue function */
	unsigned long after_wait;

	/* The queue occupancy dropped below the low water mark, the producer
	 * is signalled
	 */
	unsigned long unblocked;

	/* Minimum number of elements ever enqueued */
	unsigned int min_enqueued;
};

/* Fifo stats structure. If FIFO stats are enabled, via config variable
 * enable_fifo_stats, a FIFO has a pointer to a stats structure
 */
struct xdp2_fifo_stats {
	struct xdp2_fifo_cons_stats consumer;
	struct xdp2_fifo_prod_stats producer;
};

/* Poll group stats structure. If FIFO stats are enabled, via config variable
 * enable_fifo_stats, a poll group has a pointer to a stats structure
 */
struct xdp2_fifo_poll_group_stats {
	unsigned long num_wait_polls;
	unsigned long num_no_wait_polls;
	unsigned long num_ret_rd_fifo;
	unsigned long num_ret_wr_fifo;
	unsigned long num_ret_no_fifo;
	unsigned long spurious_write_polls;
};

struct xdp2_fifo_poll_group;

#define XDP2_FIFO_MAGIG_NUM 0xbd35f9a1c9434dd8

#define __XDP2_FIFO_CHECK_MAGIC(FIFO, FILE, LINE)			\
	XDP2_ASSERT((FIFO)->magic_num == XDP2_FIFO_MAGIG_NUM,		\
		    "Fifo does not match magic number %llx: at "	\
		    "%s :%u", (FIFO)->magic_num, FILE, LINE)

#define XDP2_FIFO_CHECK_MAGIC(FIFO)					\
	__XDP2_FIFO_CHECK_MAGIC(FIFO, __FILE__, __LINE__)

#define XDP2_POLL_GROUP_MAGIC_NUM 0x3b8c93de8fa0b237

#define __XDP2_POLL_GROUP_CHECK_MAGIC(POLL_GROUP, FILE, LINE) do {	\
	XDP2_ASSERT(POLL_GROUP, "Poll group is NULL at "		\
		     "%s:%u", FILE, LINE);				\
	XDP2_ASSERT((POLL_GROUP)->magic_num ==				\
				XDP2_POLL_GROUP_MAGIC_NUM,		\
		    "Poll group does not match magic number %llx: "	\
		    "at %s:%u", (POLL_GROUP)->magic_num, FILE, LINE);	\
} while (0)

#define XDP2_POLL_GROUP_CHECK_MAGIC(POLL_GROUP)				\
	__XDP2_POLL_GROUP_CHECK_MAGIC(POLL_GROUP, __FILE__, __LINE__)

/* Number of FIFOs in a poll group and number of words for that */
#define XDP2_POLL_GROUP_MAX_FIFOS 128
#define XDP2_POLL_GROUP_NUM_FIFOS_WORDS					\
		XDP2_BITS_TO_WORDS64(XDP2_POLL_GROUP_MAX_FIFOS)

/* Structure for a FIFO */
struct xdp2_fifo {
	/* Config, mostly read only */
	unsigned int num_ents; /* Number of entries in the queue */
	unsigned int low_water_mark; /* Must be > 0 */
	__u8 ent_size; /* Size of one entry in __u64 units */
	bool initted;

	/* Number of address translators to apply to stats, read_poll, and
	 * write_poll. Default is XDP2_ADDR_XLAT_NO_XLAT (-1) which means no
	 * address translation
	 */
	int addr_xlat_num;

	__u64 magic_num;

	/* Read/write fields */

	XDP2_LOCKS_MUTEX_T mutex; /* Lock for fifo structure */

	volatile unsigned int producer; /* Producer index */
	volatile unsigned int consumer; /* Consumer index */

	/* Indication that queue is full and enqueue requests will block.
	 * The queue full condition is cleared when the number of queued
	 * items falls below the configured low water mark
	 */
	volatile bool queue_full;

	unsigned int unblock_start_pos;

	/* Address translation is not applicable for conditional variables since
	 * they are not currently used when FIFOs are in shared memory (we'll do
	 * spin polling in such scenarios)
	 */
	XDP2_LOCKS_COND_T *consumer_cond; /* Conditional var. for consumer */
	XDP2_LOCKS_COND_T *producer_cond; /* Conditional var. for producer */

	struct xdp2_fifo_stats *stats; /* Address xlat applies */

	/* Poll info */
	int read_poll_addr_xlat_num;
	struct xdp2_fifo_poll_group *read_poll; /* Address xlat applies */
	unsigned int read_poll_num;

	int write_poll_addr_xlat_num;
	struct xdp2_fifo_poll_group *write_poll; /* Address xlat applies */
	unsigned int write_poll_num;

	/* Basic stats. These are applicable for both software and hardware
	 * FIFOs
	 */
	atomic_ulong num_enqueued;
	atomic_ulong num_dequeued;
	atomic_uint num_enqueue_waiters;

	/* Array of pointers to messages for ring fifo */
	__u64 queue[];
};

/* A poll group contains the parameters for poll function. Note that this data
 * structure is NOT thread safe. The assumption is that the caller is either
 * single threaded or provides sufficient locking. This also extends to the use
 * case where a poll operation is done and then a non-blocking read or a write
 * is performed on the returned FIFO. It is expected that the read or write
 * will always succeed in that case since this is done in the context of the
 * single threaded polling loop (a failure is considered an exception
 */
struct xdp2_fifo_poll_group {
	/* Number of address translator to apply to stats, read_poll, and
	 * write_poll. Default is XDP2_ADDR_XLAT_NO_XLAT (-1) which means no
	 * address translation
	 */
	int addr_xlat_num;

	__u64 magic_num;

	bool active;
	bool fifo_no_poll;

	/* Address translation is applicable to read_fifos and write_fifos.
	 * Each FIFO can be associated with its own address translator
	 * with the numbers kept in a table
	 */
	int fifos_addr_xlat[XDP2_POLL_GROUP_MAX_FIFOS];
	struct xdp2_fifo *fifos[XDP2_POLL_GROUP_MAX_FIFOS];

	__u64 fifos_ready[XDP2_POLL_GROUP_NUM_FIFOS_WORDS];
	__u64 fifos_mask[XDP2_POLL_GROUP_NUM_FIFOS_WORDS];

	unsigned int start_pos;

	XDP2_LOCKS_COND_T *cond; /* Conditional var. for poll group */
	XDP2_LOCKS_MUTEX_T mutex; /* Lock for fifo structure */

	struct xdp2_fifo_poll_group_stats *stats; /* Address xlat applies */
};

/* Helper macro's to do simple FIFO address translations */

#define FIFO_FIELD_XLAT(S, FIELD)					\
	XDP2_ADDR_XLAT((S)->addr_xlat_num, (S)->FIELD)

#define __FIFO_FIELD_XLAT(S, FIELD, TYPE)				\
	__XDP2_ADDR_XLAT((S)->addr_xlat_num, (S)->FIELD, TYPE)

/* Helper macros for stats */

#define __XDP2_FIFO_BUMP_PROD_COUNT(FIFO, FIELD) do {			\
	struct xdp2_fifo *_fifo = FIFO;					\
									\
	if (_fifo->stats)						\
		FIFO_FIELD_XLAT(_fifo, stats)->producer.FIELD++;	\
} while (0)

#define __XDP2_FIFO_BUMP_CONS_COUNT(FIFO, FIELD) do {			\
	struct xdp2_fifo *_fifo = FIFO;					\
									\
	if (_fifo->stats)						\
		FIFO_FIELD_XLAT(_fifo, stats)->consumer.FIELD++;	\
} while (0)

#define __XDP2_FIFO_BUMP_NUM_ENQUEUED(FIFO) do {			\
	struct xdp2_fifo *_fifo = FIFO;					\
									\
	_fifo->num_enqueued++;						\
} while (0)

#define __XDP2_FIFO_BUMP_NUM_DEQUEUED(FIFO) do {			\
	struct xdp2_fifo *_fifo = FIFO;					\
									\
	_fifo->num_dequeued++;						\
} while (0)

#define __XDP2_FIFO_BUMP_INC_NUM_WAITERS(FIFO) do {			\
	struct xdp2_fifo *_fifo = FIFO;					\
									\
	_fifo->num_enqueue_waiters++;					\
} while (0)

#define __XDP2_FIFO_BUMP_DEC_NUM_WAITERS(FIFO) do {			\
	struct xdp2_fifo *_fifo = FIFO;					\
									\
	_fifo->num_enqueue_waiters--;					\
} while (0)

/* Helper macro's to do simple poll group address translations */

#define POLL_GROUP_FIELD_XLAT(S, FIELD)					\
	XDP2_ADDR_XLAT((S)->addr_xlat_num, (S)->FIELD)

#define __POLL_GROUP_FIELD_XLAT(S, FIELD, TYPE)				\
	__XDP2_ADDR_XLAT((S)->addr_xlat_num, (S)->FIELD, TYPE)

#define __XDP2_FIFO_POLL_GROUP_BUMP_COUNT(POLL_GROUP, FIELD) do {	\
	struct xdp2_fifo_poll_group *_poll_group = POLL_GROUP;		\
									\
	if (_poll_group->stats)						\
		POLL_GROUP_FIELD_XLAT(					\
				_poll_group, stats)->FIELD++;		\
} while (0)

/* FIFO status functions
 *
 *  - xdp2_fifo_num_in_queue returns the number of elements enqueued on a FIFO
 *  - xdp2_fifo_avail_in_queue returns the number of elementes that may be
 *    enqueued on a FIFO before it's full. **xdp2_fifo_is_empty** returns true
 *    if there are non elements enqueued on a FIFO
 *  - xdp2_fifo_is_full returns true if the FIFO is full
 *  - xdp2_fifo_is_error** returns true if the FIFO is in error (will never
 *    be true for a software file)
 *
 * These functions don't have any locking requirements
 */
static inline unsigned int xdp2_fifo_sw_num_in_queue(
						const struct xdp2_fifo *fifo)
{
	return (fifo->producer == fifo->consumer) &&
		fifo->queue_full ? fifo->num_ents :
			(fifo->producer - fifo->consumer + fifo->num_ents) %
								fifo->num_ents;
}

static inline unsigned int xdp2_fifo_num_in_queue(const struct xdp2_fifo *fifo)
{
	return xdp2_fifo_sw_num_in_queue(fifo);
}

static inline unsigned int xdp2_fifo_sw_avail_in_queue(
						const struct xdp2_fifo *fifo)
{
	return fifo->num_ents - xdp2_fifo_sw_num_in_queue(fifo);
}

static inline unsigned int xdp2_fifo_avail_in_queue(
						const struct xdp2_fifo *fifo)
{
	return xdp2_fifo_sw_avail_in_queue(fifo);
}

static inline bool xdp2_fifo_sw_is_empty(const struct xdp2_fifo *fifo)
{

	return (fifo->consumer == fifo->producer) && !fifo->queue_full;
}

static inline bool xdp2_fifo_is_empty(const struct xdp2_fifo *fifo)
{
	return xdp2_fifo_sw_is_empty(fifo);
}

static inline bool xdp2_fifo_sw_is_full(const struct xdp2_fifo *fifo)
{
	return (fifo->consumer == fifo->producer) && fifo->queue_full;
}

static inline bool xdp2_fifo_is_full(const struct xdp2_fifo *fifo)
{
	return xdp2_fifo_sw_is_full(fifo);
}

static inline bool xdp2_fifo_is_error(const struct xdp2_fifo *fifo)
{
	/* No error state for software FIFOs */

	return false;
}

/* Poll group initialization
 *
 * Initializes the poll group mutex
 */

static inline void xdp2_fifo_init_poll_group_mem(
		struct xdp2_fifo_poll_group *poll_group)
{
	memset(poll_group, 0, sizeof(*poll_group));

	poll_group->magic_num = XDP2_POLL_GROUP_MAGIC_NUM;
	XDP2_LOCKS_MUTEX_INIT(&poll_group->mutex);
}

static inline void xdp2_init_poll_groups_mem(
		struct xdp2_fifo_poll_group *poll_group, unsigned int num)
{
	int i;

	for (i = 0; i < num; i++)
		xdp2_fifo_init_poll_group_mem(&poll_group[i]);
}

static inline bool ___xdp2_fifo_init_poll_group(
		struct xdp2_fifo_poll_group *poll_group, void *cond,
		struct xdp2_fifo_poll_group_stats *stats,
		int addr_xlat_num, bool fifo_no_poll,
		bool report_check)
{
	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			      "___xdp2_fifo_init_poll_group");

	if (poll_group->cond) {
		XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
		if (report_check)
			XDP2_WARN("Poll group already initialized");
		return false;
	}

	poll_group->cond = cond;
	poll_group->addr_xlat_num = addr_xlat_num;
	poll_group->stats = XDP2_ADDR_REV_XLAT(addr_xlat_num, stats);
	poll_group->fifo_no_poll = fifo_no_poll;

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);

	return true;
}

static inline bool __xdp2_fifo_init_poll_group(
		struct xdp2_fifo_poll_group *poll_group, void *cond,
		struct xdp2_fifo_poll_group_stats *stats,
		int addr_xlat_num, bool fifo_no_poll)
{
	return ___xdp2_fifo_init_poll_group(poll_group, cond,
				stats, addr_xlat_num,
				fifo_no_poll, true);
}

static inline bool __xdp2_fifo_init_poll_nowarn(
		struct xdp2_fifo_poll_group *poll_group, void *cond,
		struct xdp2_fifo_poll_group_stats *stats,
		int addr_xlat_num, bool fifo_no_poll)
{
	return ___xdp2_fifo_init_poll_group(poll_group, cond,
				stats, addr_xlat_num, fifo_no_poll, true);
}

static inline void xdp2_fifo_init_poll_group(
		struct xdp2_fifo_poll_group *poll_group, void *cond,
		struct xdp2_fifo_poll_group_stats *stats)
{
	__xdp2_fifo_init_poll_group(poll_group, cond, stats,
				    XDP2_ADDR_XLAT_NO_XLAT, false);
}

static inline void xdp2_fifo_uninit_poll_group(
		struct xdp2_fifo_poll_group *poll_group, void *cond)
{
	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			      "xdp2_fifo_uinit_poll_group");

	if (cond == poll_group->cond) {
		poll_group->cond = NULL;
		poll_group->addr_xlat_num = 0;
		poll_group->stats = NULL;
		poll_group->fifo_no_poll = 0;
	}

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

#define LOCK_COUNT 1000000

/* Order of locking mutexes: both the poll group mutex and the fifo mutex
 * may be locked. The pool group mutex is taken before the fifo mutex
 */

/* Backend function to finish binding a FIFO to a poll group for reading or
 * writing
 *
 * Releases fifo and poll group lock
 *
 * Called with both the FIFO and poll gtoup mutexes held. Releases them
 * (FIFO mutex first)
 */
static inline void __xdp2_fifo_finish_bind_fifo_to_poll(
		struct xdp2_fifo_poll_group *poll_group,
		struct xdp2_fifo *fifo, unsigned int num)
{
	/* Store the read fifo in the table by its relative address */
	poll_group->fifos_addr_xlat[num] = fifo->addr_xlat_num;
	poll_group->fifos[num] = XDP2_ADDR_REV_XLAT(fifo->addr_xlat_num, fifo);

	xdp2_bitmap64_unset(poll_group->fifos_mask, num);
	xdp2_bitmap64_unset(poll_group->fifos_ready, num);

	XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

/* Bind a FIFO to a read poll group
 *
 * Takes both the FIFO and poll group mutexes
 */
static inline void xdp2_fifo_bind_fifo_to_read_poll(
		struct xdp2_fifo_poll_group *poll_group,
		struct xdp2_fifo *fifo, unsigned int num)
{
	XDP2_FIFO_CHECK_MAGIC(fifo);
	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			      "xdp2_fifo_bind_fifo_to_read_poll");
	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			      "xdp2_fifo_bind_fifo_to_read_poll");

	/* Store the address of the poll group as a relative address */
	fifo->read_poll_num = num;
	fifo->read_poll = XDP2_ADDR_REV_XLAT(poll_group->addr_xlat_num,
					     poll_group);
	fifo->read_poll_addr_xlat_num = poll_group->addr_xlat_num;

	__xdp2_fifo_finish_bind_fifo_to_poll(poll_group, fifo, num);
}

static inline void xdp2_fifo_make_readable(struct xdp2_fifo *fifo);

/* Enable a FIFO for polling in a read poll group
 *
 * Locks and unlocks the poll group mutex and locks and unlocks the FIFO mutex
 */
static inline void xdp2_fifo_enable_read_poll(struct xdp2_fifo *fifo)
{
	struct xdp2_fifo_poll_group *poll_group;
	bool make_readable = false;

	XDP2_FIFO_CHECK_MAGIC(fifo);

	if (!fifo->read_poll)
		return;

	poll_group = XDP2_ADDR_XLAT(fifo->read_poll_addr_xlat_num,
				    fifo->read_poll);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_enable_read_poll");

	xdp2_bitmap64_set(poll_group->fifos_mask, fifo->read_poll_num);

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);

	/* Call xdp2_fifo_sw_num_in_queue with FIFO lock held when it is
	 * used in an operation al context to synchronized with other
	 * callers
	 */
	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "FIFO mutex in "
			     "xdp2_fifo_enable_read_poll");
	make_readable = xdp2_fifo_sw_num_in_queue(fifo);
	XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);

	if (make_readable)
		xdp2_fifo_make_readable(fifo);
}

/* Enable a non-FIFO poll index for polling in a read poll group
 *
 * Takes the poll group mutex
 */
static inline void xdp2_fifo_enable_nonfifo_read_poll(
		struct xdp2_fifo_poll_group *poll_group,
		unsigned int num)
{
	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_enable_nonfifo_read_poll");

	xdp2_bitmap64_set(poll_group->fifos_mask, num);

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

/* Disable a non-FIFO poll index for polling in a read poll group
 *
 * Takes the poll group mutex
 */
static inline void xdp2_fifo_disable_nonfifo_read_poll(
		struct xdp2_fifo_poll_group *poll_group, unsigned int num)
{
	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_disable_nonfifo_read_poll");

	xdp2_bitmap64_unset(poll_group->fifos_mask, num);

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

/* Backend function to finish unbinding a FIFO from a poll group for reading
 * or writing
 *
 * Called with FIFO and poll group mutexes held, and releases them
 */
static inline void __xdp2_fifo_finish_unbind_fifo_from_poll(
		struct xdp2_fifo_poll_group *poll_group,
		struct xdp2_fifo *fifo, unsigned int num)
{
	xdp2_bitmap64_unset(poll_group->fifos_ready, num);
	xdp2_bitmap64_unset(poll_group->fifos_mask, num);

	poll_group->fifos_addr_xlat[num] = 0;
	poll_group->fifos[num] = 0;

	XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

/* Unbind a FIFO from a read poll group
 *
 * Takes FIFO and poll group mutexes
 */
static inline void xdp2_fifo_unbind_fifo_from_read_poll(
						struct xdp2_fifo *fifo)
{
	struct xdp2_fifo_poll_group *poll_group;
	unsigned int num;

	XDP2_FIFO_CHECK_MAGIC(fifo);

	if (!fifo->read_poll)
		return;

	poll_group = XDP2_ADDR_XLAT(fifo->read_poll_addr_xlat_num,
				    fifo->read_poll);

	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_unbind_fifo_from_read_poll");
	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_unbind_fifo_from_read_poll");

	num = fifo->read_poll_num;

	fifo->read_poll_num = -1U;
	fifo->read_poll = NULL;

	__xdp2_fifo_finish_unbind_fifo_from_poll(poll_group, fifo, num);
}

/* Disable a FIFO for polling in a read poll group
 *
 * Takes FIFO and poll group mutexes
 */
static inline void xdp2_fifo_disable_read_poll(struct xdp2_fifo *fifo)
{
	struct xdp2_fifo_poll_group *poll_group;

	XDP2_FIFO_CHECK_MAGIC(fifo);

	if (!fifo->read_poll)
		return;

	poll_group = XDP2_ADDR_XLAT(fifo->read_poll_addr_xlat_num,
				    fifo->read_poll);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_disable_read_poll");

	xdp2_bitmap64_unset(poll_group->fifos_mask, fifo->read_poll_num);

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

/* Bind a FIFO to a write poll group
 *
 * Locks and releases FIFO and poll group lock
 */
static inline void xdp2_fifo_bind_fifo_to_write_poll(
		struct xdp2_fifo_poll_group *poll_group,
		struct xdp2_fifo *fifo, unsigned int num)
{
	XDP2_FIFO_CHECK_MAGIC(fifo);
	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_bind_write_fifo");
	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_bind_write_fifo");

	/* Store the address of the poll group as a relative address */
	fifo->write_poll_num = num;
	fifo->write_poll = XDP2_ADDR_REV_XLAT(poll_group->addr_xlat_num,
					      poll_group);
	fifo->write_poll_addr_xlat_num = poll_group->addr_xlat_num;

	__xdp2_fifo_finish_bind_fifo_to_poll(poll_group, fifo, num);
}

static inline void xdp2_fifo_make_writable(struct xdp2_fifo *fifo);

/* Enable a FIFO for polling in a write poll group
 *
 * Locks and releases poll group mutex, and then Locks and releases FIFO mutex
 */
static inline void xdp2_fifo_enable_write_poll(struct xdp2_fifo *fifo)
{
	struct xdp2_fifo_poll_group *poll_group;
	bool make_writable;

	XDP2_FIFO_CHECK_MAGIC(fifo);

	if (!fifo->write_poll)
		return;

	poll_group = XDP2_ADDR_XLAT(fifo->write_poll_addr_xlat_num,
				    fifo->write_poll);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_set_write_fifo");

	xdp2_bitmap64_set(poll_group->fifos_mask, fifo->write_poll_num);

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);

	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "FIFO mutex in "
			     "xdp2_fifo_set_write_fifo");
	make_writable = !!xdp2_fifo_sw_avail_in_queue(fifo);
	XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);

	if (make_writable)
		xdp2_fifo_make_writable(fifo);
}

/* Enable a non-FIFO poll index for polling in a write poll group
 *
 * Takes FIFO and poll group mutexes
 */
static inline void xdp2_fifo_enable_nonfifo_write_poll(
		struct xdp2_fifo_poll_group *poll_group, unsigned int num)
{
	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_enable_nonfifo_write_poll");

	xdp2_bitmap64_set(poll_group->fifos_mask, num);

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

/* Disable a non-FIFO poll index for polling in a write poll group
 *
 * Takes poll group mutex
 */
static inline void xdp2_fifo_disable_nonfifo_write_poll(
		struct xdp2_fifo_poll_group *poll_group, unsigned int num)
{
	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_disable_nonfifo_write_poll");

	xdp2_bitmap64_unset(poll_group->fifos_mask, num);

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

/* Unbind a FIFO from a write poll group
 *
 * Takes FIFO and poll group mutexes
 */
static inline void xdp2_fifo_unbind_fifo_from_write_poll(
						struct xdp2_fifo *fifo)
{
	struct xdp2_fifo_poll_group *poll_group;
	unsigned int num;

	XDP2_FIFO_CHECK_MAGIC(fifo);

	if (!fifo->write_poll)
		return;

	poll_group = XDP2_ADDR_XLAT(fifo->write_poll_addr_xlat_num,
				    fifo->write_poll);

	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_unbind_write_fifo");
	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_unbind_write_fifo");

	num = fifo->write_poll_num;

	fifo->write_poll_num = -1U;
	fifo->write_poll = NULL;

	__xdp2_fifo_finish_unbind_fifo_from_poll(poll_group, fifo, num);
}

/* Unset a FIFO for write polling indicated by it's poll group number
 *
 * Takes poll group mutex
 */
static inline void xdp2_fifo_disable_write_poll(struct xdp2_fifo *fifo)
{
	struct xdp2_fifo_poll_group *poll_group;

	XDP2_FIFO_CHECK_MAGIC(fifo);

	if (!fifo->write_poll)
		return;

	poll_group = XDP2_ADDR_XLAT(fifo->write_poll_addr_xlat_num,
				    fifo->write_poll);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_set_write_fifo");

	xdp2_bitmap64_unset(poll_group->fifos_mask, fifo->write_poll_num);

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

/* Make a FIFO readable, This is called from an enqueue operation.
 *
 * Takes FIFO mutex. May take poll group mutex.
 */
static inline void xdp2_fifo_make_readable(struct xdp2_fifo *fifo)
{
	struct xdp2_fifo_poll_group *poll_group;
	bool do_signal = false;
	unsigned int poll_num;

	XDP2_FIFO_CHECK_MAGIC(fifo);

	if (fifo->consumer_cond)
		XDP2_LOCKS_COND_SIGNAL(fifo->consumer_cond);

	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_make_readable");

	if (!fifo->read_poll) {
		XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
		return;
	}

	/* Get the absolute address of the poll group for reading from its
	 * relative address
	 */
	poll_group = XDP2_ADDR_XLAT(fifo->read_poll_addr_xlat_num,
				    fifo->read_poll);

	poll_num = fifo->read_poll_num;

	XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);

	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_make_readable");

	if (xdp2_bitmap64_isset(poll_group->fifos_mask, poll_num)) {
		xdp2_bitmap64_set(poll_group->fifos_ready, poll_num);
		do_signal = true;
	}

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);

	if (do_signal)
		XDP2_LOCKS_COND_SIGNAL(poll_group->cond);
}

/* Mark FIFO as not readable, This is called from an dequeue operation when
 * FIFO becomes empty.
 *
 * Takes FIFO mutex. May take poll group mutex.
 */
static inline void xdp2_fifo_mark_non_readable(struct xdp2_fifo *fifo)
{
	struct xdp2_fifo_poll_group *poll_group =
			XDP2_ADDR_XLAT(fifo->read_poll_addr_xlat_num,
				       fifo->read_poll);
	unsigned int poll_num;

	XDP2_FIFO_CHECK_MAGIC(fifo);

	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_mark_non_readable");

	if (!fifo->read_poll) {
		XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
		return;
	}

	/* Get the absolute address of the poll group for reading from its
	 * relative address
	 */
	poll_group = XDP2_ADDR_XLAT(fifo->read_poll_addr_xlat_num,
				    fifo->read_poll);

	poll_num = fifo->read_poll_num;

	XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);

	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_mark_non_readable");
	xdp2_bitmap64_unset(poll_group->fifos_ready, poll_num);
	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

/* Clear a FIFO as being readable
 *
 * Takes the poll group mutex
 */
static inline void xdp2_fifo_clear_readable(
		struct xdp2_fifo_poll_group *poll_group, unsigned int poll_num)
{
	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_clear_readable");
	xdp2_bitmap64_unset(poll_group->fifos_ready, poll_num);
	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

/* Make a FIFO writable, This is called from an dequeue operation.
 *
 * Takes FIFO mutex. May take poll group mutex.
 */
static inline void xdp2_fifo_make_writable(struct xdp2_fifo *fifo)
{
	struct xdp2_fifo_poll_group *poll_group;
	bool do_signal = false;
	unsigned int poll_num;

	XDP2_FIFO_CHECK_MAGIC(fifo);

	if (fifo->producer_cond)
		XDP2_LOCKS_COND_SIGNAL(fifo->producer_cond);

	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_make_writable");

	if (!fifo->write_poll) {
		XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
		return;
	}

	/* Get the absolute address of the poll group for writing from its
	 * relative address
	 */
	poll_group = XDP2_ADDR_XLAT(fifo->write_poll_addr_xlat_num,
				    fifo->write_poll);

	poll_num = fifo->write_poll_num;

	XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);

	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_make_writable");

	if (xdp2_bitmap64_isset(poll_group->fifos_mask, poll_num)) {
		xdp2_bitmap64_set(poll_group->fifos_ready, poll_num);
		do_signal = true;
	}

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);

	if (do_signal)
		XDP2_LOCKS_COND_SIGNAL(poll_group->cond);
}

/* Mark FIFO as not writable, This is called from an enqueue operation when
 * FIFO becomes full.
 *
 * Takes FIFO mutex. May take poll group mutex.
 */
static inline void xdp2_fifo_mark_non_writable(struct xdp2_fifo *fifo)
{
	struct xdp2_fifo_poll_group *poll_group;
	unsigned int poll_num;

	XDP2_FIFO_CHECK_MAGIC(fifo);

	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_mark_non_writable");

	if (!fifo->write_poll) {
		XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
		return;
	}

	/* Get the absolute address of the poll group for writing from its
	 * relative address
	 */
	poll_group = XDP2_ADDR_XLAT(fifo->write_poll_addr_xlat_num,
				    fifo->write_poll);

	poll_num = fifo->write_poll_num;

	XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);

	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_make_non_writable");
	xdp2_bitmap64_unset(poll_group->fifos_ready, poll_num);
	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

static inline __u64 xdp2_fifo_sw_get_ready(
		struct xdp2_fifo_poll_group *poll_group, unsigned int num)
{
	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	return poll_group->fifos_ready[num];
}

/* Poll a poll group for readable or writable FIFOs in the software
 * implementation
 *
 * Returns FIFO poll number in [0..63] for a readable FIFO, a FIFO poll number
 * in [64..127] for writable FIFO, or a value >= 128 if no FIFOs are ready.
 * If a writable FIFO is return, i.e. a value in the range [64..127] then
 * the caller can subtract sixty-four to derive a canonical FIFO poll number
 *
 * Takes the poll group mutex
 */
static inline unsigned int __xdp2_fifo_sw_poll(
		struct xdp2_fifo_poll_group *poll_group,
		bool wait, char *_file, int _line)
{
	unsigned int v;
	__u64 fifos[XDP2_POLL_GROUP_NUM_FIFOS_WORDS];
	int i;

	__XDP2_POLL_GROUP_CHECK_MAGIC(poll_group, _file, _line);
	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "xdp2_fifo_poll");

	/* Be explicit here since rw_fifos is volatile and we don't want a
	 * type mismatch in arguments of xdp2_bitmap64_and
	 */
	for (i = 0; i < XDP2_POLL_GROUP_NUM_FIFOS_WORDS; i++)
		fifos[i] = poll_group->fifos_ready[i];

	xdp2_bitmap64_destsrc_and(fifos, poll_group->fifos_mask, 0,
				  XDP2_POLL_GROUP_MAX_FIFOS);

	v = xdp2_bitmap64_find_roll(fifos, poll_group->start_pos,
				    XDP2_POLL_GROUP_MAX_FIFOS);
	if (v < XDP2_POLL_GROUP_MAX_FIFOS) {
		poll_group->start_pos = (v + 1) % XDP2_POLL_GROUP_MAX_FIFOS;

		XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);

		return v;
	}

	if (!wait) {
		XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
		return XDP2_POLL_GROUP_MAX_FIFOS;
	}

	/* Need to wait */
	do {
		XDP2_LOCKS_COND_WAIT(poll_group->cond, &poll_group->mutex);

		for (i = 0; i < XDP2_POLL_GROUP_NUM_FIFOS_WORDS; i++)
			fifos[i] = poll_group->fifos_ready[i];

		xdp2_bitmap64_destsrc_and(fifos, poll_group->fifos_mask, 0,
					  XDP2_POLL_GROUP_MAX_FIFOS);

		v = xdp2_bitmap64_find_roll(fifos, poll_group->start_pos,
					    XDP2_POLL_GROUP_MAX_FIFOS);
	} while (v >= XDP2_POLL_GROUP_MAX_FIFOS);

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);

	return v;
}

#define xdp2_fifo_sw_poll(POLL_GROUP, WAIT)				\
		__xdp2_fifo_sw_poll(POLL_GROUP, WAIT, __FILE__, __LINE)

/* Poll a poll group for readable or writable FIFOs in the hardware
 * implementation
 *
 * Returns FIFO poll number in [0..63] for a readable FIFO, a FIFO poll number
 * in [64..127] for writable FIFO, or a value >= 128 if no FIFOs are ready.
 * If a writable FIFO is return, i.e. a value in the range [64..127] then
 * the caller can subtract sixty-four to derive a canonical FIFO poll number
 */

/* Perform a pseudo poll by just returning the FIFOs that are set in the
 * mask of the poll group. There is no check whether the FIFOs are actually
 * ready, so in most case these are just going to result in spurious polls.
 * Effectively, this checks each FIFO in the poll group mask independently
 * and can be used in cases where proper poll functions are not supported.
 * Note that this is inherently a non-blocking call
 *
 * Doesn't take any mutexes
 */
static inline unsigned int xdp2_fifo_poll_no_poll(
				struct xdp2_fifo_poll_group *poll_group)
{
	__u64 v;

	/* Return the next FIFO in the poll group masks. Caller will need to
	 * check if it is actually readable or writable
	 */
	v = xdp2_bitmap64_find_roll(poll_group->fifos_mask,
				    poll_group->start_pos,
				    XDP2_POLL_GROUP_MAX_FIFOS);
	if (v < XDP2_POLL_GROUP_MAX_FIFOS)
		poll_group->start_pos = (v + 1) % XDP2_POLL_GROUP_MAX_FIFOS;

	return v;
}

/* Poll a poll group for readable or writable FIFOs
 *
 * Returns FIFO poll number in [0..63] for a readable FIFO, a FIFO poll number
 * in [64..127] for writable FIFO, or a value >= 128 if no FIFOs are ready.
 * If a writable FIFO is return, i.e. a value in the range [64..127] then
 * the caller can subtract sixty-four to derive a canonical FIFO poll number
 *
 * Takes poll group mutex in called function
 */
static inline unsigned int __xdp2_fifo_poll(
		struct xdp2_fifo_poll_group *poll_group, bool wait,
		char *_file, int _line)
{
	__u64 v;

	if (wait)
		__XDP2_FIFO_POLL_GROUP_BUMP_COUNT(poll_group,
						   num_wait_polls);
	else
		__XDP2_FIFO_POLL_GROUP_BUMP_COUNT(poll_group,
						   num_no_wait_polls);

	if (poll_group->fifo_no_poll) {
		if (wait)
			XDP2_WARN("Poll called with wait argument set to "
				  "true, but poll group is perform no-poll. "
				  "Ignoring wait flag");

		v = xdp2_fifo_poll_no_poll(poll_group);
	} else {
		v = __xdp2_fifo_sw_poll(poll_group, wait, _file, _line);
	}

	if (v < XDP2_POLL_GROUP_MAX_FIFOS / 2) {
		__XDP2_FIFO_POLL_GROUP_BUMP_COUNT(poll_group, num_ret_rd_fifo);
		poll_group->active = true;
	} else if (v < XDP2_POLL_GROUP_MAX_FIFOS) {
		__XDP2_FIFO_POLL_GROUP_BUMP_COUNT(poll_group, num_ret_wr_fifo);
		poll_group->active = true;
	} else {
		__XDP2_FIFO_POLL_GROUP_BUMP_COUNT(poll_group, num_ret_no_fifo);
	}

	return v;
}

#define xdp2_fifo_poll(POLL_GROUP, WAIT)				\
		__xdp2_fifo_poll(POLL_GROUP, WAIT, __FILE__, __LINE__)

/* Enqueue a message on a fifo using software implementation
 *
 * num argument contains the number of double word (__u64 units) to
 * enqueue from messagep pointer
 *
 * Returns true if message was successfully enqueued, returns false if the
 * fifo is full and wait is set to false
 *
 * Takes FIFO mutex
 */
static inline bool ___xdp2_fifo_sw_enqueue(struct xdp2_fifo *fifo,
					   unsigned int num, __u64 *messagep,
					   bool wait, char *_file, int _lineo)
{
	bool was_empty = false;

	__XDP2_FIFO_CHECK_MAGIC(fifo, _file, _lineo);

	XDP2_ASSERT(num <= fifo->ent_size,
		    "FIFO enqueue: enqueue %u dwords is greater than "
		    "FIFO element size %u", num, fifo->ent_size);

	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_enqueue");

	if (xdp2_fifo_sw_is_full(fifo)) {
		__XDP2_FIFO_BUMP_PROD_COUNT(fifo, blocked);

		if (!wait) {
			XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
			return false;
		}

		XDP2_ASSERT(fifo->producer_cond, "Waiting on FIFO enqueue "
			    "but cond is not set for FIFO");

		/* Wait until there's space on the queue */
		__XDP2_FIFO_BUMP_PROD_COUNT(fifo, waits);

		__XDP2_FIFO_BUMP_INC_NUM_WAITERS(fifo);

		while (1) {
			XDP2_LOCKS_COND_WAIT(fifo->producer_cond,
					      &fifo->mutex);

			if (!xdp2_fifo_sw_is_full(fifo))
				break;

			/* Spurious wakeup */
			__XDP2_FIFO_BUMP_PROD_COUNT(fifo, spurious_wakeups);
		}
		__XDP2_FIFO_BUMP_PROD_COUNT(fifo, after_wait);

		__XDP2_FIFO_BUMP_DEC_NUM_WAITERS(fifo);
	}

	/* Do the work of queuing */

	if (fifo->producer == fifo->consumer) {
		__XDP2_FIFO_BUMP_PROD_COUNT(fifo, saw_empty);

		was_empty = true;
	}

	memcpy(&fifo->queue[fifo->producer * fifo->ent_size],
	       messagep, num * sizeof(__u64));

	fifo->producer = (fifo->producer + 1) % fifo->num_ents;

	if (fifo->stats) {
		struct xdp2_fifo_stats *stats = FIFO_FIELD_XLAT(fifo, stats);
		unsigned int num_enqueued = xdp2_fifo_sw_num_in_queue(fifo);

		if (num_enqueued > stats->producer.max_enqueued)
			stats->producer.max_enqueued = num_enqueued;
	}

	if (fifo->consumer == fifo->producer) {
		unsigned int num_enqueued;

		__XDP2_FIFO_BUMP_PROD_COUNT(fifo, queue_full);

		fifo->queue_full = true;
		XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
		xdp2_fifo_mark_non_writable(fifo);

		/* After releasing the lock, but before calling
		 * xdp2_fifo_mark_non_writable, it is possible that the
		 * consumer may dequeue such that xdp2_fifo_mark_non_writable
		 * is not correct since there may now be space on the FIFO
		 * such that it is writable. Check for this by reading
		 * num_in_queue under the FIFO lock and if the FIFO is now
		 * writable call xdp2_fifo_make_writable
		 */
		XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
				     "xdp2_fifo_enqueue check non-writable");
		num_enqueued = xdp2_fifo_sw_num_in_queue(fifo);
		XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
		if (num_enqueued < fifo->low_water_mark)
			xdp2_fifo_make_writable(fifo);
	} else {
		XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
	}

	/* Release fifo lock before calling xdp2_fifo_make_readable per mutex
	 * locking order for fifo's. Note that another dequeue operation may
	 * occur without the lock such that the fifo is no longer readable.
	 * The worst affect for make the fifo readable in this case should
	 * be a spurious wakeuo
	 */
	if (was_empty)
		xdp2_fifo_make_readable(fifo);

	return true;
}

#define __xdp2_fifo_sw_enqueue(FIFO, NUM, MESSAGEP, WAIT)		\
	___xdp2_fifo_sw_enqueue(FIFO, NUM, MESSAGEP, WAIT, __FILE__, __LINE__)

#define xdp2_fifo_sw_enqueue(FIFO, MESSAGE, WAIT) ({			\
	__u64 _v = MESSAGE;						\
									\
	(__xdp2_fifo_sw_enqueue(FIFO, 1, &v, WAIT));			\
})

/* Enqueue a message on a fifo
 *
 * Returns true if message was successfully enqueued, returns false if the
 * fifo is full and wait is set to false
 *
 * Called function takes FIFO mutex
 */
static inline bool ___xdp2_fifo_enqueue(struct xdp2_fifo *fifo,
					unsigned int num, __u64 *messagep,
					bool wait, char *_file, int _line)
{
	bool ret;

	__XDP2_FIFO_BUMP_PROD_COUNT(fifo, requests);

	ret = ___xdp2_fifo_sw_enqueue(fifo, num, messagep, wait, _file, _line);

	if (ret)
		__XDP2_FIFO_BUMP_NUM_ENQUEUED(fifo);
	else
		__XDP2_FIFO_BUMP_PROD_COUNT(fifo, fails);

	return ret;
}

#define __xdp2_fifo_enqueue(FIFO, NUM, MESSAGEP, WAIT)			\
	___xdp2_fifo_enqueue(FIFO, NUM, MESSAGEP, WAIT, __FILE__, __LINE__)

#define xdp2_fifo_enqueue(FIFO, MESSAGE, WAIT) ({			\
	__u64 _v = MESSAGE;						\
									\
	__xdp2_fifo_enqueue(FIFO, 1, &_v, WAIT);			\
})

/* Enqueue a message on a fifo with no failure allowed
 *
 * Error condition if the FIFO is full (do assert in software case, backend
 * can assert for hardware FIFOs)
 *
 * Called function takes FIFO mutex
 */
static inline void ___xdp2_fifo_enqueue_nocheck(struct xdp2_fifo *fifo,
						unsigned int num,
						__u64 *messagep,
						char *_file, int _line)
{
	bool ret;

	__XDP2_FIFO_BUMP_PROD_COUNT(fifo, requests);

	ret = ___xdp2_fifo_sw_enqueue(fifo, num, messagep, false, _file, _line);

	XDP2_ASSERT(ret, "Enqueue on a \"no check\" FIFO failed");

	__XDP2_FIFO_BUMP_NUM_ENQUEUED(fifo);
}

#define __xdp2_fifo_enqueue_nocheck(FIFO, NUM, MESSAGEP)		\
		___xdp2_fifo_enqueue_nocheck(FIFO, NUM, MESSAGEP,	\
					      __FILE__, __LINE__)

#define xdp2_fifo_enqueue_nocheck(FIFO, MESSAGE) do {			\
	__u64 v = MESSAGE;						\
									\
	__xdp2_fifo_enqueue_nocheck(FIFO, 1, &v);			\
} while (0)

/* Signal a poll group number as being readable or writable
 *
 * Takes the poll group mutex
 */
static inline void __xdp2_poll_group_signal(struct xdp2_fifo_poll_group
								*poll_group,
					    unsigned int poll_num,
					    bool writable)
{
	bool wakeup = false;

	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			      "__xdp2_poll_group_signal");
	if (!xdp2_bitmap64_isset(poll_group->fifos_ready, poll_num)) {
		xdp2_bitmap64_set(poll_group->fifos_ready, poll_num);
		wakeup = true;
	}

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);

	if (wakeup)
		XDP2_LOCKS_COND_SIGNAL(poll_group->cond);
}

/* Signal a poll group number as being readable */
static inline void xdp2_poll_group_signal_readable(
		struct xdp2_fifo_poll_group *poll_group,
		unsigned int poll_num)
{
	__xdp2_poll_group_signal(poll_group, poll_num, false);
}

/* Signal a poll group number as being writable */
static inline void xdp2_poll_group_signal_writable(
		struct xdp2_fifo_poll_group *poll_group,
		unsigned int poll_num)
{
	__xdp2_poll_group_signal(poll_group, poll_num, true);
}

/* Unsignal a poll group number as being readable or writable
 *
 * Takes the poll group mutex
 */
static inline void __xdp2_poll_group_unsignal(
		struct xdp2_fifo_poll_group *poll_group,
		unsigned int poll_num, bool writable)
{
	XDP2_POLL_GROUP_CHECK_MAGIC(poll_group);

	XDP2_FIFO_MUTEX_LOCK(&poll_group->mutex, "Poll group mutex in "
			     "__xdp2_poll_group_signal");

	xdp2_bitmap64_unset(poll_group->fifos_ready, poll_num);

	XDP2_FIFO_MUTEX_UNLOCK(&poll_group->mutex);
}

/* Unsignal a poll group number as being readable */
static inline void xdp2_poll_group_unsignal_readable(
		struct xdp2_fifo_poll_group *poll_group,
		unsigned int poll_num)
{
	__xdp2_poll_group_unsignal(poll_group, poll_num, false);
}

/* Unsignal a poll group number as being writable */
static inline void xdp2_poll_group_unsignal_writable(
		struct xdp2_fifo_poll_group *poll_group,
		unsigned int poll_num)
{
	__xdp2_poll_group_unsignal(poll_group, poll_num, true);
}

/* Make a fifo non-readable after dequeue. This is called in two instances:
 *   1) The fifo became empty after the dequeue
 *   2) The fifo was already empty and no wait was on (this is the case of
 *	doing a dequeue after a pool and the fifo was actually drained
 *	before the non-blocking fifo read occurred
 *
 * Takes the FIFO mutex
 */
static void xdp2_fifo_make_non_readable_after_dequeue(struct xdp2_fifo *fifo)
{
	xdp2_fifo_mark_non_readable(fifo);

	/* Check again under mutex lock if fifo became readable */

	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_make_non_readable_after_dequeue");

	if (!xdp2_fifo_sw_is_empty(fifo)) {
		/* Queue became readable after we dropped the mutex lock. Note
		 * that after release the fifo mutex, it's possible that a
		 * dequeue operation occurs and again the fifo is non-readable--
		 * we don't do anything special for this case, the worst affect
		 * is a spurious wakeup in fifo poll
		 */
		XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);

		xdp2_fifo_make_readable(fifo);
	} else {
		XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
	}
}

/* Dequeue or a message and return the value in messagep using software
 * implementation
 *
 * num argument contains the number of double word (__u64 units) to
 * dequeue into messagep pointer
 *
 * Returns true if a message was successfully dequeued and thus messagep
 * contents are valid, returns false if the fifo is empty and wait is set
 * to false
 *
 * Takes the FIFO mutex
 */
static inline bool ___xdp2_fifo_sw_dequeue(struct xdp2_fifo *fifo,
					   unsigned int num,
					   __u64 *messagep, bool wait,
					   char *_file, int _line)
{
	unsigned int num_enqueued;
	bool signal_producer = 0;
	bool non_readable;

	__XDP2_FIFO_CHECK_MAGIC(fifo, _file, _line);

	XDP2_ASSERT(num <= fifo->ent_size,
		    "FIFO dequeue: dequeue %u dwords is greater than "
		    "FIFO element size %u", num, fifo->ent_size);

	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex,
			     "Fifo mutex in __xdp2_fifo_sw_dequeue");

	if (xdp2_fifo_sw_is_empty(fifo)) {
		/* The fifo is empty */

		if (!wait) {
			XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);

			/* We did a non-blocking wait but there was nothing to
			 * read. Ensure the fifo is non-readable for poll so we
			 * don't spin on an empty fifo marked readable
			 */
			xdp2_fifo_make_non_readable_after_dequeue(fifo);

			return false;
		}

		XDP2_ASSERT(fifo->consumer_cond, "Waiting on FIFO dequeue "
			    "but cond is not set for FIFO");

		/* Wait until the queue is not empty */
		__XDP2_FIFO_BUMP_CONS_COUNT(fifo, waits);

		while (1) {
			XDP2_LOCKS_COND_WAIT(fifo->consumer_cond,
					      &fifo->mutex);
			if (!xdp2_fifo_sw_is_empty(fifo))
				break;
			/* Spurious wakeup */
			__XDP2_FIFO_BUMP_CONS_COUNT(fifo, spurious_wakeups);
		}
		__XDP2_FIFO_BUMP_CONS_COUNT(fifo, after_wait);
	}

	/* Have a message to dequeue */

	memcpy(messagep, &fifo->queue[fifo->consumer * fifo->ent_size],
	       num * sizeof(__u64));

	fifo->consumer = (fifo->consumer + 1) % fifo->num_ents;

	non_readable = (fifo->producer == fifo->consumer);

	/* Don't call xdp2_fifo_sw_num_in_queue here since queue_full is
	 * not properly set yet
	 */
	num_enqueued = (fifo->producer - fifo->consumer + fifo->num_ents) %
								fifo->num_ents;
	if (fifo->stats) {
		struct xdp2_fifo_stats *stats = FIFO_FIELD_XLAT(fifo, stats);

		if (num_enqueued < stats->consumer.min_enqueued)
			stats->consumer.min_enqueued = num_enqueued;
	}

	if (fifo->queue_full) {
		/* Check if queue occupancy has fallen below the low water
		 * mark
		 */
		if (num_enqueued < fifo->low_water_mark) {
			__XDP2_FIFO_BUMP_CONS_COUNT(fifo, unblocked);

			fifo->queue_full = false;

			XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);

			/* After releasing the lock, it's possible an enqueue
			 * operation may happen such that the fifo is again
			 * full. The worst effect of this should be a spurious
			 * wakeup
			 */
			xdp2_fifo_make_writable(fifo);

			signal_producer = true;
		} else {
			XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
		}
	} else {
		XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
	}

	if (non_readable)
		xdp2_fifo_make_non_readable_after_dequeue(fifo);

	if (signal_producer && fifo->producer_cond)
		XDP2_LOCKS_COND_SIGNAL(fifo->producer_cond);

	return true;
}

#define __xdp2_fifo_sw_dequeue(FIFO, NUM, MESSAGEP, WAIT)		\
	___xdp2_fifo_sw_dequeue(FIFO, NUM, MESSAGEP, WAIT, __FILE__,	\
				__LINE__)

#define xdp2_fifo_sw_dequeue(FIFO, MESSAGEP, WAIT)			\
	__xdp2_fifo_sw_dequeue(FIFO, 1, MESSAGEP, WAIT)

/* Dequeue a message and return the value in messagep using hardware FIFOs
 *
 * Returns true if a message was successfully dequeued and thus messagep
 * contents are valid, returns false if the fifo is empty and wait is set
 * to false
 */

/* Dequeue a message and return the value in messagep
 *
 * Returns true if a message was successfully dequeued and thus messagep
 * contents are valid, returns false if the fifo is empty and wait is set
 * to false
 *
 * Called function takes the FIFO mutex
 */
static inline bool ___xdp2_fifo_dequeue(struct xdp2_fifo *fifo,
					unsigned int num, __u64 *messagep,
					bool wait, char *_file, int _line)
{
	bool ret;

	ret = ___xdp2_fifo_sw_dequeue(fifo, num, messagep, wait, _file, _line);

	if (ret)
		__XDP2_FIFO_BUMP_NUM_DEQUEUED(fifo);
	else
		__XDP2_FIFO_BUMP_CONS_COUNT(fifo, fails);

	return ret;
}

#define __xdp2_fifo_dequeue(FIFO, NUM, MESSAGEP, WAIT)			\
	___xdp2_fifo_dequeue(FIFO, NUM, MESSAGEP, WAIT, __FILE__, __LINE__)

#define xdp2_fifo_dequeue(FIFO, MESSAGEP, WAIT)			\
	__xdp2_fifo_dequeue(FIFO, 1, MESSAGEP, WAIT)

/* Initialize the FIFO. The consumer and producer variables are initialized
 * in separate calls. This allows the common FIFO structure to first be
 * initialized, and then the two parties each initialize their part of the
 * FIFO
 *
 * Initializes the FIFO mutex
 */
static inline void __xdp2_fifo_init(struct xdp2_fifo *fifo,
				    unsigned int num_ents,
				    unsigned int ent_size,
				    unsigned int low_water_mark,
				    struct xdp2_fifo_stats *stats,
				    int addr_xlat_num)
{
	memset(fifo, 0, sizeof(*fifo));

	fifo->num_ents = num_ents;
	fifo->ent_size = ent_size;
	fifo->low_water_mark = low_water_mark;

	/* Save pointer to stats structure as a relative address */
	if (stats) {
		memset(stats, 0, sizeof(*stats));
		fifo->stats = XDP2_ADDR_REV_XLAT(addr_xlat_num, stats);
	}

	fifo->magic_num = XDP2_FIFO_MAGIG_NUM;
	fifo->addr_xlat_num = addr_xlat_num;

	XDP2_LOCKS_MUTEX_INIT(&fifo->mutex);

	fifo->initted = true;
}

static inline void xdp2_fifo_init(struct xdp2_fifo *fifo,
				  unsigned int num_ents,
				  unsigned int low_water_mark,
				  struct xdp2_fifo_stats *stats)
{
	__xdp2_fifo_init(fifo, num_ents, 1, low_water_mark, stats,
			 XDP2_ADDR_XLAT_NO_XLAT);
}

/* Return the FIFO allocation size given the number of elements in the
 * FIFO and the size of each entry
 */
static inline size_t xdp2_fifo_size(unsigned int num_ents,
				    unsigned int ent_size)
{
	struct xdp2_fifo *fifo;

	return sizeof(*fifo) + num_ents * ent_size * sizeof(fifo->queue[0]);
}

static inline void xdp2_fifo_init_fifos(struct xdp2_fifo *fifo,
					struct xdp2_fifo_stats *stats,
					unsigned int num, __u8 ent_size,
					unsigned int limit,
					unsigned int low_water_mark,
					int addr_xlat_num)
{
	int i;

	for (i = 0; i < num; i++) {
		__xdp2_fifo_init(fifo, limit, ent_size, low_water_mark,
				 stats ? &stats[i] : NULL, addr_xlat_num);

		fifo = xdp2_add_len_to_ptr(fifo,
					   xdp2_fifo_size(limit, ent_size));
	}
}

#define __XDP2_FIFO_SIZE(LIMIT, SIZE)				\
	(sizeof(struct xdp2_fifo) + (LIMIT) * (SIZE) * sizeof(__u64))

#define XDP2_FIFO_SIZE(LIMIT)					\
	__XDP2_FIFO_SIZE(LIMIT, 1)

/* Allocate memory and initialize a FIFO */
static inline struct xdp2_fifo *__xdp2_fifo_create(
		unsigned int num_ents, unsigned int ent_size,
		unsigned int low_water_mark, struct xdp2_fifo_stats *stats,
		int addr_xlat_num)
{
	struct xdp2_fifo *fifo;

	fifo = calloc(1, __XDP2_FIFO_SIZE(num_ents, ent_size));
	if (!fifo)
		return NULL;

	__xdp2_fifo_init(fifo, num_ents, ent_size, low_water_mark, stats,
			 addr_xlat_num);

	return fifo;
}

static inline struct xdp2_fifo *xdp2_fifo_create(
		unsigned int num_ents, unsigned int low_water_mark,
		struct xdp2_fifo_stats *stats)
{
	return __xdp2_fifo_create(num_ents, 1, low_water_mark, stats,
				  XDP2_ADDR_XLAT_NO_XLAT);
}

/* Initialize the producer side of a FIFO.
 *
 * Takes the FIFO mutex
 */
static inline bool __xdp2_fifo_init_producer(struct xdp2_fifo *fifo,
					     XDP2_LOCKS_COND_T *cond,
					     bool report_check)
{
	XDP2_FIFO_CHECK_MAGIC(fifo);

	/* fifo_init must have been called */
	XDP2_ASSERT(fifo->initted, "Fifo structure not initialized");

	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_init_producer");

	if (fifo->producer_cond) {
		XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
		if (report_check)
			XDP2_WARN("Producer side of FIFO already initialized");
		return false;
	}

	fifo->producer_cond = cond;

	XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);

	return true;
}

static inline bool xdp2_fifo_init_producer(struct xdp2_fifo *fifo,
					   XDP2_LOCKS_COND_T *cond)
{
	return __xdp2_fifo_init_producer(fifo, cond, true);
}

static inline bool xdp2_fifo_init_producer_nowarn(struct xdp2_fifo *fifo,
						  XDP2_LOCKS_COND_T *cond)
{
	return __xdp2_fifo_init_producer(fifo, cond, false);
}

/* Uninitialize the producer side of a FIFO.
 *
 * Takes the FIFO mutex
 */
static inline void xdp2_fifo_uninit_producer(struct xdp2_fifo *fifo,
					     XDP2_LOCKS_COND_T *cond)
{
	XDP2_FIFO_CHECK_MAGIC(fifo);

	/* fifo_init must have been called */
	XDP2_ASSERT(fifo->initted, "Fifo structure not initialized");

	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_uninit_producer");

	if (cond == fifo->producer_cond)
		fifo->producer_cond = NULL;

	XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
}

/* Initialize the consumer side of a FIFO.
 *
 * Takes the FIFO mutex
 */
static inline bool __xdp2_fifo_init_consumer(struct xdp2_fifo *fifo,
					     XDP2_LOCKS_COND_T *cond,
					     bool report_check)
{
	XDP2_FIFO_CHECK_MAGIC(fifo);

	/* fifo_init must have been called */
	XDP2_ASSERT(fifo->initted, "Fifo structure not initialized");

	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_init_consumer");

	if (fifo->consumer_cond) {
		XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
		if (report_check)
			XDP2_WARN("Consumer side of FIFO already initialized");
		return false;
	}

	fifo->consumer_cond = cond;

	XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);

	return true;
}

static inline bool xdp2_fifo_init_consumer(struct xdp2_fifo *fifo,
					   XDP2_LOCKS_COND_T *cond)
{
	return __xdp2_fifo_init_consumer(fifo, cond, true);
}

static inline bool xdp2_fifo_init_consumer_nowarn(struct xdp2_fifo *fifo,
						  XDP2_LOCKS_COND_T *cond)
{
	return __xdp2_fifo_init_consumer(fifo, cond, false);
}

/* Uninitialize the consumer side of a FIFO.
 *
 * Takes the FIFO mutex
 */
static inline void xdp2_fifo_uninit_consumer(struct xdp2_fifo *fifo,
					     XDP2_LOCKS_COND_T *cond)
{
	XDP2_FIFO_CHECK_MAGIC(fifo);

	/* fifo_init must have been called */
	XDP2_ASSERT(fifo->initted, "Fifo structure not initialized");

	XDP2_FIFO_MUTEX_LOCK(&fifo->mutex, "Fifo mutex in "
			     "xdp2_fifo_uninit_consumer");

	if (cond == fifo->consumer_cond)
		fifo->consumer_cond = NULL;

	XDP2_FIFO_MUTEX_UNLOCK(&fifo->mutex);
}

void xdp2_fifo_dump_one_fifo(const struct xdp2_fifo *fifo,
			     const char *name, int id1, int id2, void *cli);

void xdp2_fifo_dump_fifos(const struct xdp2_fifo *fifo,
			  unsigned int num_fifos,
			  unsigned int limit, const char *name,
			  int major_index, bool active_only, void *cli);

void xdp2_fifo_dump_fifos_add_base(const struct xdp2_fifo *fifo,
				   unsigned int num_fifos,
				   unsigned int limit, const char *name,
				   int major_index, bool active_only,
				   void *cli);

void xdp2_fifo_dump_one_poll_group(const struct xdp2_fifo_poll_group
								*poll_group,
				   const char *name, int id1, int id2,
				   void *cli);

void xdp2_fifo_dump_poll_groups(const struct xdp2_fifo_poll_group *poll_group,
				unsigned int num_poll_groups, const char *name,
				int major_index, bool active_only, void *cli);

#endif /* __XDP2_FIFO_H__ */
