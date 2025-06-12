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

#ifndef _OMETHOD_H_8ffe52b5_
#define _OMETHOD_H_8ffe52b5_

/* The interface to output methods.
 *
 * The paradigm is that the output method is called both before and
 * after __skb_flow_dissect is called. The first call ("pre") is to
 * do anything that should be done with the raw packet data, such as
 * printing it or saving (some or all) of it. The second call
 * ("post") is to do anything that depends on __skb_flow_dissect's
 * return value or, for successful calls, the dissected data. If the
 * output method wants to keep some state, the way to do it is for the
 * init method to allocate a state struct, private to the output
 * method, fill it in, and return a pointer to it. This pointer is
 * passed back to the output method pre, post, and done calls.
 */

#include "test-parser-out.h"

/* An output method.
 *
 * name is the text name, for the command line and error messages.
 *
 * help is expected to print help; this is used by -o help,<name>.
 *
 * init is passed the ,ARGS string from the command line, or nil if not
 * even the comma was given.  It is expected to parse the parameters
 * and either return a void * pointer which is passed to pre, post,
 * and done calls, or complain to stderr and exit.  (The void * is
 * opaque to everything except the output method.)
 *
 * pre is called before calling __skb_flow_dissect.  It is passed the
 * raw packet data, as a pointer-and-length, and the packet serial
 * number (first packet is 1, second is 2, etc).
 *
 * post is called after __skb_flow_dissect returns.  It is passed the
 * error-string return value and a pointer to the OUT.  It is expected
 * to test the return value and do whatever is appropriate: complain
 * bout dissect failure, print the dissected values on success, etc.
 * The string is nil on success and an error message on failure.
 *
 * done is called to clean up before exiting.  This is not, strictly,
 * necessary, since exiting cleans up most things.  This exists both
 * in case there is something (like a temporary file in the
 * filesystem) that needs more cleaning up and to make it easier to
 * wrap this code in things that don't just exit when they're done.
 */
struct omethod {
	const char *name;
	void (*help)(void);
	void *(*init)(const char *args);
	void (*pre)(void *pv, const unsigned char *data, size_t len,
		    unsigned int ser);
	void (*post)(void *pv, const char *status,
		     const struct test_parser_out *out);
	void (*done)(void *pv);
};

#define OMETHOD_DECL(name)					\
	struct omethod omethod_##name = {			\
		#name,						\
		&out_##name##_help,				\
		&out_##name##_init,				\
		&out_##name##_pre,				\
		&out_##name##_post,				\
		&out_##name##_done,				\
	};

extern struct omethod *omethods[];

#endif
