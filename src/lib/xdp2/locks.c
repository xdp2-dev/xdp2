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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "xdp2/locks.h"
#include "xdp2/utility.h"

enum xdp2_locks_select xdp2_locks_select = XDP2_LOCKS_SELECT_DEFAULT;
unsigned int xdp2_locks_debug_count = 1000000;
unsigned int xdp2_atomic_lock_sleep = 1000;

static void __xdp2_locks_init_locks(void)
{
	char *env;

	env = getenv("XDP2_LOCKS_debug_count");
	if (env)
		xdp2_locks_debug_count = strtoul(env, NULL, 10);

	env = getenv("XDP2_LOCKS_atomic_lock_sleep");
	if (env)
		xdp2_atomic_lock_sleep = strtoul(env, NULL, 10);

	env = getenv("XDP2_LOCKS_variant");
	if (!env)
		return;

	if (!strcmp(env, "pthreads"))
		xdp2_locks_select = XDP2_LOCKS_SELECT_PTHREADS;
	else if (!strcmp(env, "atomics"))
		xdp2_locks_select = XDP2_LOCKS_SELECT_ATOMICS;
	else
		XDP2_ERR(1, "XDP2 locks: Unknown variant %s "
			     "from environment variable", env);
}

void xdp2_locks_init_locks(void)
{
	pthread_once_t once_val = PTHREAD_ONCE_INIT;

	if (pthread_once(&once_val, __xdp2_locks_init_locks) != 0)
		XDP2_ERR(1, "XDP2 locks: Initialize XDP2 locks failed");
}
