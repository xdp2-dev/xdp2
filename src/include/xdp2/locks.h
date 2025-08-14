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

#ifndef __XDP2_LOCKS_H__
#define __XDP2_LOCKS_H__

#include "xdp2/utility.h"

enum xdp2_locks_select {
	XDP2_LOCKS_SELECT_PTHREADS,
	XDP2_LOCKS_SELECT_ATOMICS,
	XDP2_LOCKS_SELECT_DEFAULT = XDP2_LOCKS_SELECT_PTHREADS
};

extern enum xdp2_locks_select xdp2_locks_select;
extern unsigned int xdp2_locks_debug_count;
extern unsigned int atomic_lock_sleep;

void xdp2_locks_init_locks(void);

static inline enum xdp2_locks_select xdp2_locks_get_select(void)
{
	xdp2_locks_init_locks();
	return xdp2_locks_select;
}

/* Macro definitions for atomic locks variant */

#define XDP2_LOCKS_ATOMICS_COND_T uintptr_t
#define XDP2_LOCKS_ATOMICS_MUTEX_T				\
struct {							\
	volatile atomic_flag flag;				\
	char *last_locker;					\
	char *current_locker;					\
}
#define XDP2_LOCKS_ATOMICS_MUTEX_INIT(LOCK) atomic_flag_clear(LOCK)
#define XDP2_LOCKS_ATOMICS_MUTEX_LOCK(LOCK) do {		\
	while (atomic_flag_test_and_set(LOCK));			\
} while (0)
#define XDP2_LOCKS_ATOMICS_MUTEX_LOCK_DEBUG(LOCK, COUNT,	\
					     NAME) do {		\
	bool lock_failed = false;				\
	int i = 0;						\
								\
	while (atomic_flag_test_and_set(			\
				&(LOCK)->flag)) {		\
		if (++i == (COUNT))				\
			XDP2_WARN("fifo-locks: Attempting to "	\
				   "lock %s %u times, current "	\
				    "locker %s, last locker %s",\
				    NAME, i,			\
				    (LOCK)->current_locker,	\
				    (LOCK)->last_locker);	\
	}							\
	(LOCK)->current_locker = NAME;				\
	if (lock_failed)					\
		XDP2_WARN("fifo-locks: Lock %s obtained "	\
			   "after %u attempts",			\
			    NAME, i);				\
} while (0)
#define XDP2_LOCKS_ATOMICS_MUTEX_UNLOCK(LOCK)			\
	atomic_flag_clear(LOCK)

#define XDP2_LOCKS_ATOMICS_MUTEX_UNLOCK_DEBUG(LOCK) do {	\
	(LOCK)->last_locker = (LOCK)->current_locker;		\
	(LOCK)->current_locker = NULL;				\
	atomic_flag_clear(&(LOCK)->flag);			\
} while (0)
#define XDP2_LOCKS_ATOMICS_COND_SIGNAL(COND)
#define XDP2_LOCKS_ATOMICS_COND_WAIT(COND, LOCK) do {		\
	XDP2_LOCKS_ATOMICS_MUTEX_UNLOCK(LOCK);			\
	usleep(1000);						\
	XDP2_LOCKS_ATOMICS_MUTEX_LOCK(LOCK);			\
} while (0)
#define XDP2_LOCKS_ATOMICS_COND_INIT(COND)

/* Macro's for pthread locks variant */

#define XDP2_LOCKS_PTHREADS_COND_T pthread_cond_t
#define XDP2_LOCKS_PTHREADS_MUTEX_T				\
struct {							\
	pthread_mutex_t mutex;					\
	char *last_locker;					\
	char *current_locker;					\
}
#define XDP2_LOCKS_PTHREADS_MUTEX_INIT(LOCK) pthread_mutex_init(LOCK, NULL)
#define XDP2_LOCKS_PTHREADS_MUTEX_LOCK(LOCK) pthread_mutex_lock(LOCK)
#define XDP2_LOCKS_PTHREADS_MUTEX_LOCK_DEBUG(LOCK, COUNT,	\
					      NAME) do {	\
	bool lock_failed = false;				\
	int i = 0;						\
								\
	while (pthread_mutex_trylock(				\
			&(LOCK)->mutex)) {			\
		if (++i == (COUNT))				\
			XDP2_WARN("fifo-locks: Attempting to "	\
				   "lock %s %u times, current "	\
				    "locker %s, last locker %s",\
				    NAME, i,			\
				    (LOCK)->current_locker,	\
				    (LOCK)->last_locker);	\
	}							\
	(LOCK)->current_locker = NAME;				\
	if (lock_failed)					\
		XDP2_WARN("fifo-locks: Lock %s obtained "	\
			   "after %u attempts",			\
			   NAME, i);				\
} while (0)
#define XDP2_LOCKS_PTHREADS_MUTEX_UNLOCK(LOCK)			\
		pthread_mutex_unlock(LOCK)
#define XDP2_LOCKS_PTHREADS_MUTEX_UNLOCK_DEBUG(LOCK) do {	\
	(LOCK)->last_locker = (LOCK)->current_locker;		\
	(LOCK)->current_locker = NULL;				\
	pthread_mutex_unlock(&(LOCK)->mutex);			\
} while (0)

#define XDP2_LOCKS_PTHREADS_COND_SIGNAL(COND) pthread_cond_signal(COND)
#define XDP2_LOCKS_PTHREADS_COND_WAIT(COND, LOCK) pthread_cond_wait(COND, LOCK)
#define XDP2_LOCKS_PTHREADS_COND_INIT(COND) pthread_cond_init(COND, NULL)

/* Macro's for select variant (the variant is specified in xdp2_locks_select
 * which is set from environment variable XDP2_LOCKS_variant
 */

#define XDP2_LOCKS_SELECT_COND_T union xdp2_locks_select_cond
#define XDP2_LOCKS_SELECT_MUTEX_T union xdp2_locks_select_mutex
#define XDP2_LOCKS_SELECT_MUTEX_INIT(LOCK) do {				\
	xdp2_locks_init_locks();					\
	switch (xdp2_locks_select) {					\
	case XDP2_LOCKS_SELECT_PTHREADS:				\
		XDP2_LOCKS_PTHREADS_MUTEX_INIT(				\
					&(LOCK)->pthreads.mutex);	\
		break;							\
	case XDP2_LOCKS_SELECT_ATOMICS:					\
		XDP2_LOCKS_ATOMICS_MUTEX_INIT(&(LOCK)->atomic.flag);	\
		break;							\
	}								\
} while (0)

#define XDP2_LOCKS_SELECT_MUTEX_LOCK(LOCK) do {				\
	switch (xdp2_locks_select) {					\
	case XDP2_LOCKS_SELECT_PTHREADS:				\
		XDP2_LOCKS_PTHREADS_MUTEX_LOCK(				\
					&(LOCK)->pthreads.mutex);	\
		break;							\
	case XDP2_LOCKS_SELECT_ATOMICS:					\
		XDP2_LOCKS_ATOMICS_MUTEX_LOCK(&(LOCK)->atomic.flag);	\
		break;							\
	}								\
} while (0)

#define XDP2_LOCKS_SELECT_MUTEX_LOCK_DEBUG(LOCK, COUNT, NAME) do {	\
	switch (xdp2_locks_select) {					\
	case XDP2_LOCKS_SELECT_PTHREADS:				\
		XDP2_LOCKS_PTHREADS_MUTEX_LOCK_DEBUG(			\
				&(LOCK)->pthreads, COUNT, NAME);	\
		break;							\
	case XDP2_LOCKS_SELECT_ATOMICS:				\
		XDP2_LOCKS_ATOMICS_MUTEX_LOCK_DEBUG(			\
				&(LOCK)->atomic, COUNT, NAME);		\
		break;							\
	case XDP2_LOCKS_SELECT_LOCKSERVER:				\
		XDP2_LOCKS_LOCKSERVER_MUTEX_LOCK_DEBUG(		\
				&(LOCK)->lock_server,			\
				COUNT, NAME);				\
		break;							\
	}								\
} while (0)

#define XDP2_LOCKS_SELECT_MUTEX_UNLOCK(LOCK) do {			\
	switch (xdp2_locks_select) {					\
	case XDP2_LOCKS_SELECT_PTHREADS:				\
		XDP2_LOCKS_PTHREADS_MUTEX_UNLOCK(			\
					&(LOCK)->pthreads.mutex);	\
		break;							\
	case XDP2_LOCKS_SELECT_ATOMICS:				\
		XDP2_LOCKS_ATOMICS_MUTEX_UNLOCK(&(LOCK)->atomic.flag);	\
		break;							\
	}								\
} while (0)

#define XDP2_LOCKS_SELECT_MUTEX_UNLOCK_DEBUG(LOCK) do {		\
	switch (xdp2_locks_select) {					\
	case XDP2_LOCKS_SELECT_PTHREADS:				\
		XDP2_LOCKS_PTHREADS_MUTEX_UNLOCK_DEBUG(		\
						&(LOCK)->pthreads);	\
		break;							\
	case XDP2_LOCKS_SELECT_ATOMICS:				\
		XDP2_LOCKS_ATOMICS_MUTEX_UNLOCK_DEBUG(			\
						&(LOCK)->atomic);	\
		break;							\
	case XDP2_LOCKS_SELECT_LOCKSERVER:				\
		XDP2_LOCKS_LOCKSERVER_MUTEX_UNLOCK_DEBUG(		\
				&(LOCK)->lock_server);			\
		break;							\
	}								\
} while (0)

#define XDP2_LOCKS_SELECT_COND_SIGNAL(COND) do {			\
	switch (xdp2_locks_select) {					\
	case XDP2_LOCKS_SELECT_PTHREADS:				\
		XDP2_LOCKS_PTHREADS_COND_SIGNAL(&(COND)->cond);	\
		break;							\
	case XDP2_LOCKS_SELECT_ATOMICS:				\
		XDP2_LOCKS_ATOMICS_COND_SIGNAL(&(COND)->dummy);	\
		break;							\
	}								\
} while (0)

#define XDP2_LOCKS_SELECT_COND_WAIT(COND, LOCK) do {			\
	switch (xdp2_locks_select) {					\
	case XDP2_LOCKS_SELECT_PTHREADS:				\
		XDP2_LOCKS_PTHREADS_COND_WAIT(&(COND)->cond,		\
				       &(LOCK)->pthreads.mutex);	\
		break;							\
	case XDP2_LOCKS_SELECT_ATOMICS:				\
		XDP2_LOCKS_ATOMICS_COND_WAIT(&(COND)->dummy,		\
					      &(LOCK)->atomic.flag);	\
		break;							\
	}								\
} while (0)

#define XDP2_LOCKS_SELECT_COND_INIT(COND) do {				\
	switch (xdp2_locks_select) {					\
	case XDP2_LOCKS_SELECT_PTHREADS:				\
		XDP2_LOCKS_PTHREADS_COND_INIT(&(COND)->cond);		\
		break;							\
	case XDP2_LOCKS_SELECT_ATOMICS:				\
		XDP2_LOCKS_ATOMICS_COND_INIT(&(COND)->dummy);		\
		break;							\
	}								\
} while (0)

#ifdef XDP2_LOCKS_USE_PTHREADS

/* Using pthreads locks variant. Set XDP2_LOCKS_ macro's accordingly */

#include <pthread.h>

#define XDP2_LOCKS_COND_T XDP2_LOCKS_PTHREADS_COND_T
#define XDP2_LOCKS_MUTEX_T XDP2_LOCKS_PTHREADS_MUTEX_T
#define XDP2_LOCKS_MUTEX_INIT(LOCK)					\
		XDP2_LOCKS_PTHREADS_MUTEX_INIT(&(LOCK)->mutex)
#define XDP2_LOCKS_MUTEX_LOCK(LOCK)					\
		XDP2_LOCKS_PTHREADS_MUTEX_LOCK(&(LOCK)->mutex)
#define XDP2_LOCKS_MUTEX_LOCK_DEBUG(LOCK, COUNT, NAME)			\
		XDP2_LOCKS_PTHREADS_MUTEX_LOCK_DEBUG(			\
					&(LOCK)->mutex, COUNT, NAME)
#define XDP2_LOCKS_MUTEX_UNLOCK(LOCK)					\
		XDP2_LOCKS_PTHREADS_MUTEX_UNLOCK(&(LOCK)->mutex)
#define XDP2_LOCKS_MUTEX_UNLOCK_DEBUG(LOCK)				\
		XDP2_LOCKS_PTHREADS_MUTEX_UNLOCK_DEBUG(&(LOCK)->mutex)
#define XDP2_LOCKS_COND_SIGNAL(COND)					\
		XDP2_LOCKS_PTHREADS_COND_SIGNAL(COND)
#define XDP2_LOCKS_COND_WAIT(COND, LOCK)				\
		XDP2_LOCKS_PTHREADS_COND_WAIT(COND, &(LOCK)->mutex)
#define XDP2_LOCKS_COND_INIT(COND)					\
		XDP2_LOCKS_PTHREADS_COND_INIT(COND)

#elif defined(XDP2_LOCKS_USE_ATOMICS)

/* Using atomics locks varinant. Set XDP2_LOCKS_ macro's accordingly */

#include <stdatomic.h>

#define XDP2_LOCKS_COND_T XDP2_LOCKS_ATOMICS_COND_T
#define XDP2_LOCKS_MUTEX_T XDP2_LOCKS_ATOMICS_MUTEX_T
#define XDP2_LOCKS_MUTEX_INIT(LOCK)					\
		XDP2_LOCKS_ATOMICS_MUTEX_INIT(&(LOCK)->flag)
#define XDP2_LOCKS_MUTEX_LOCK(LOCK)					\
			XDP2_LOCKS_ATOMICS_MUTEX_LOCK(&(LOCK)->flag)
#define XDP2_LOCKS_MUTEX_LOCK_DEBUG(LOCK, COUNT, NAME)			\
		XDP2_LOCKS_ATOMICS_MUTEX_LOCK_DEBUG(			\
						&(LOCK)->flag, COUNT, NAME)
#define XDP2_LOCKS_MUTEX_UNLOCK(LOCK)					\
		XDP2_LOCKS_ATOMICS_MUTEX_UNLOCK(&(LOCK)->flag)
#define XDP2_LOCKS_MUTEX_UNLOCK_DEBUG(LOCK)				\
		XDP2_LOCKS_ATOMICS_MUTEX_UNLOCK_DEBUG(&(LOCK)->flag)
#define XDP2_LOCKS_COND_SIGNAL(COND)					\
		XDP2_LOCKS_ATOMICS_COND_SIGNAL(&(COND))
#define XDP2_LOCKS_COND_WAIT(COND, LOCK)				\
		XDP2_LOCKS_ATOMICS_COND_WAIT(&(COND), &(LOCK)->flag)
#define XDP2_LOCKS_COND_INIT(COND)					\
		XDP2_LOCKS_ATOMICS_COND_INIT(&(COND))
#define XDP2_LOCKS_GET_SELECT() XDP2_LOCKS_SELECT_ATOMICS

#else

/* If XDP2_LOCKS_USE_PTHREADS and XDP2_LOCKS_USE_ATOMICS aren't set then
 * use the select variant as the default
 */

#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <inttypes.h>

union xdp2_locks_select_mutex {
	XDP2_LOCKS_ATOMICS_MUTEX_T atomic;
	XDP2_LOCKS_PTHREADS_MUTEX_T pthreads;
};

union xdp2_locks_select_cond {
	uintptr_t dummy;
	pthread_cond_t cond;
};

/* Set the XDP2_LOCKS_* macro's */

#define XDP2_LOCKS_COND_T XDP2_LOCKS_SELECT_COND_T
#define XDP2_LOCKS_MUTEX_T XDP2_LOCKS_SELECT_MUTEX_T
#define XDP2_LOCKS_MUTEX_INIT(LOCK) XDP2_LOCKS_SELECT_MUTEX_INIT(LOCK)
#define XDP2_LOCKS_MUTEX_LOCK(LOCK) XDP2_LOCKS_SELECT_MUTEX_LOCK(LOCK)
#define XDP2_LOCKS_MUTEX_LOCK_DEBUG(LOCK, COUNT, NAME)			\
		XDP2_LOCKS_SELECT_MUTEX_LOCK_DEBUG(LOCK, COUNT, NAME)
#define XDP2_LOCKS_MUTEX_UNLOCK(LOCK) XDP2_LOCKS_SELECT_MUTEX_UNLOCK(LOCK)
#define XDP2_LOCKS_MUTEX_UNLOCK_DEBUG(LOCK)				\
		XDP2_LOCKS_SELECT_MUTEX_UNLOCK_DEBUG(LOCK)
#define XDP2_LOCKS_COND_SIGNAL(COND) XDP2_LOCKS_SELECT_COND_SIGNAL(COND)
#define XDP2_LOCKS_COND_WAIT(COND, LOCK)				\
		XDP2_LOCKS_SELECT_COND_WAIT(COND, LOCK)
#define XDP2_LOCKS_COND_INIT(COND) XDP2_LOCKS_SELECT_COND_INIT(COND)
#define XDP2_LOCKS_GET_SELECT() xdp2_locks_get_select()

#endif


#endif /* __XDP2_LOCKS_H__ */
