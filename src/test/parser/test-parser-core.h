/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2020, 2021 by Mojatatu Networks.
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

#ifndef _CORE_H_151c7cfd_
#define _CORE_H_151c7cfd_

/* The interface to computation cores.
 *
 * The paradigm is that the main line calls the core to process each
 * packet.  There are ancillary methods to initialize and tear down
 * computation cores.
 */

#include "test-parser-out.h"

/* A computation core.
 *
 * name is the text name, for the command line and error messages.
 *
 * help is expected to print help; this is used by -c help,<name>.
 *
 * init is passed the ARGS string from the command line, or NULL if not
 * even the comma was given. It is expected to parse the parameters
 * and either return a void pointer which is passed to process and
 * done calls, or print to sdderr and exit. (The void * is opaque
 * to everything except the core). If the core needs to keep some
 * kind of state - buffering, settings, whatever - the way to do it is
 * for init to allocate a private state struct and return a pointer to
 * it. process and done then get that pointer passed back to them.
 *
 * process is called when the main program wants to process a(nother)
 * packet. It is passed the the cookie from init, the buffer pointer,
 * the packet length, the OUT it should put the results into, and some
 * flag bits.
 *
 * done is called to clean up before exiting. This is not, strictly,
 * necessary, since exiting cleans up most things. This exists both
 * in case there is something (like a temporary file in the
 * filesystem) that needs more cleaning up and to make it easier to
 * wrap this code in things that don't just exit when they're done.
 */
struct test_parser_core {
	const char *name;
	void (*help)(void);
	void *(*init)(const char *args);
	const char *(*process)(void *pv, void *data, size_t len,
			       struct test_parser_out *out, unsigned int flags,
			       long long *ptr);
#define CORE_F_NOCORE 0x1
#define CORE_F_HASH   0x2
#define CORE_F_VERBOSE   0x4
#define CORE_F_DEBUG	 0x8
	void (*done)(void *pv);
};

#define CORE_DECL(name)						\
	struct test_parser_core test_parser_core_##name = {	\
		#name,						\
		&core_##name##_help,				\
		&core_##name##_init,				\
		&core_##name##_process,				\
		&core_##name##_done,				\
	};

extern struct test_parser_core *cores[];

#endif
