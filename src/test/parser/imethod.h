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

#ifndef _IMETHOD_H_16253721_
#define _IMETHOD_H_16253721_

/* The interface to input methods.
 *
 * The paradigm is that the main line calls the input method each time
 * it wants an input packet.  If the input method gets the packets
 * from a source that returns multiple packets at once, it has to
 * buffer them (which it can do with a private data structure, set up
 * by init and passed to readpkt).
 */

/* Returned values from readpkt methods.  The intended semantics are:
 *	PARSER_TEST_RP_GOOD		A good packet.
 *	PARSER_TEST_RP_OVF		A `packet' which overflows the buffer.
 *	PARSER_TEST_RP_EOF		No more input.
 *	PARSER_TEST_RP_ERR		Some error occurred.
 *  In the PARSER_TEST_RP_ERR case, it is the readpkt method's responsibility
 *  to print a suitable error message to stderr before returning.
 */
enum test_parser_rprv {
	PARSER_TEST_RP_GOOD = 1,
	PARSER_TEST_RP_OVF,
	PARSER_TEST_RP_EOF,
	PARSER_TEST_RP_ERR,
};

/* An input method.
 *
 * name is the text name, for the command line and error messages.
 *
 * help is expected to print help; this is used by -i help,<name>.
 *
 * init is passed the ARGS string from the command line, or nil if not
 * even the comma was given. It is expected to parse the parameters
 * and either return a void pointer which is passed to readpkt and
 * done calls, or complain to stderr and exit. (The void * is opaque
 * to everything except the input method). If the input method needs
 * to keep some kind of state - buffering, settings, whatever - the
 * way to do it is for init to allocate a private state struct and
 * return a pointer to it. readpkt and done then get that pointer
 * passed back to them.
 *
 * readpkt is called when the main program wants to read a(nother)
 * input packet. It is passed the cookie from init, the buffer
 * pointer, the size of the buffer, and an int * through which it
 * should store the actual size of the packet. (It is an error for
 * the size stored to be less than zero or greater than the buffer
 * size). It returns an enum rprv (see above) indicating what
 * kind of result it had; PARSER_TEST_RP_GOOD is the only one where it
 * is expected to provide packet data and length through the second and
 * fourth arguments, but it is harmless (if perhaps inefficient) for to
 * write through the packet buffer and length pointers in other cases,
 * provided of course that it does not write to packet data beyond the
 * specified maximum length.
 *
 * done is called to clean up before exiting. This is not, strictly,
 * necessary, since exiting cleans up most things. This exists both
 * in case there is something (like a temporary file in the
 * filesystem) that needs more cleaning up and to make it easier to
 * wrap this code in things that don't just exit when they're done.
 */
struct imethod {
	const char *name;
	void (*help)(void);
	void *(*init)(const char *args);
	enum test_parser_rprv (*readpkt)(void *pv, unsigned char *data,
					 size_t maxlen, size_t *lenp);
	void (*done)(void *pv);
};

#define IMETHOD_DECL(name)					\
	struct imethod imethod_##name = {			\
	#name,							\
	&in_##name##_help,					\
	&in_##name##_init,					\
	&in_##name##_readpkt,					\
	&in_##name##_done,					\
};

extern struct imethod *imethods[];

#endif
