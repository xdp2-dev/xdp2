// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
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

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "imethod.h"
#include "omethod.h"
#include "xdp2/utility.h"
#include "test-parser-out.h"
#include "test-parser-core.h"

extern const char *__progname;

#define MAXPKT 65536

/* pktdata and pktlen are non-static for the sake of code in compat.c
 * that fakes stuff that in a live kernel would come from an skb.
 */
unsigned char pktdata[MAXPKT] __defaligned();
ssize_t pktlen;
static int repeat = 1;
static unsigned int coreflags;
static struct imethod *imethod;
static void *imarg;
static struct omethod *omethod;
static void *omarg;
static struct test_parser_core *core;
static void *carg;
static unsigned int pktnum;

/* Read packets.  This just calls on the input method. */
static int readpkt(void)
{
	enum test_parser_rprv rv;
	size_t len;

	if (!imethod) {
		fprintf(stderr,
			"%s: no input method specified (use -h for help)\n",
			__progname);
		exit(-1);
	}
	while (1) {
		rv = (*imethod->readpkt)(imarg, &pktdata[0], MAXPKT, &len);
		pktlen = len;
		switch (rv) {
		case PARSER_TEST_RP_GOOD:
			if ((pktlen < 0) || (pktlen > MAXPKT)) {
				fprintf(stderr,
					"%s: input method is broken (packet "
					"size %lu isn't in 0..%u)\n",
					__progname, pktlen, MAXPKT);
				exit(-1);
			}
			return (1);
		case PARSER_TEST_RP_OVF:
			fprintf(stderr,
				"%s: input packet overflows buffer - packet "
				"ignored\n", __progname);
			continue;
		case PARSER_TEST_RP_EOF:
			return 0;
		case PARSER_TEST_RP_ERR:
			continue;
		default:
			fprintf(stderr,
				"%s: %s readpkt method returned bad status "
				"%d\n", __progname, imethod->name, (int)rv);
			exit(-1);
		}
	}
}

static long long _time;

static void processpkt(void)
{
	struct test_parser_out out;
	const char *coreerr;

	if (!core) {
		fprintf(stderr,
			"%s: no computation core specified (use -h for help)\n",
			__progname);
		exit(-1);
	}
	if (!omethod) {
		fprintf(stderr,
			"%s: no output method specified (use -h for help)\n",
			__progname);
		exit(-1);
	}
	if (pktlen < 14) {
		fprintf(stderr, "Length %lu - too small for Ethernet\n",
			pktlen);
		return;
	}
	pktnum++;
	(*omethod->pre) (omarg, &pktdata[0], pktlen, pktnum);
	coreerr = (*core->process) (carg, &pktdata[0], pktlen, &out, coreflags,
			      &_time);
	(*omethod->post) (omarg, coreerr, &out);
}

static void set_repeat(const char *name)
{
	long liv;
	char *ep;
	int iv;

	liv = strtol(name, &ep, 0);
	if (ep == name) {
		fprintf(stderr, "%s: no number found in `%s'\n", __progname,
			name);
		exit(-1);
	}
	if (*ep) {
		fprintf(stderr, "%s: junk after number in `%s'\n", __progname,
			name);
		exit(-1);
	}
	iv = liv;
	if ((iv != liv) || (iv < 1)) {
		fprintf(stderr, "%s: number `%ld' out of range\n", __progname,
			liv);
		exit(-1);
	}
	repeat = iv;
}

static void set_imethod(const char *name)
{
	const char *comma;
	int nl, el, i;

	comma = index(name, ',');
	nl = comma ? comma - name : strlen(name);

	if ((nl == 4) && !bcmp(name, "list", 4)) {
		for (i = 0; imethods[i]; i++)
			printf("%s\n", imethods[i]->name);

		exit(0);
	}

	if ((nl == 4) && !bcmp(name, "help", 4)) {
		if (!comma) {
			printf
			    ("You need to specify what input method you want "
			     "help on, as in\n"
			     "        -i help,%s\n", imethods[0]->name);
			exit(0);
		}
		for (i = 0; imethods[i]; i++) {
			if (!strcmp(imethods[i]->name, comma + 1)) {
				(*imethods[i]->help) ();
				exit(0);
			}
		}
		fprintf(stderr,
			"%s: unknown input method `%s' (use -h for help)\n",
			__progname, comma + 1);
		exit(0);
	}

	for (i = 0; imethods[i]; i++) {
		el = strlen(imethods[i]->name);
		if ((el == nl) && !bcmp(name, imethods[i]->name, nl)) {
			imethod = imethods[i];
			imarg = (*imethod->init) (comma ? comma + 1 : 0);
			return;
		}
	}

	fprintf(stderr, "%s: unknown input method `%s' (use -h for help)\n",
		__progname, name);

	exit(-1);
}

static void set_omethod(const char *name)
{
	const char *comma;
	int nl, el, i;

	comma = index(name, ',');
	nl = comma ? comma - name : strlen(name);

	if ((nl == 4) && !strncmp(name, "list", 4)) {
		for (i = 0; omethods[i]; i++)
			printf("%s\n", omethods[i]->name);
		exit(0);
	}

	if ((nl == 4) && !strncmp(name, "help", 4)) {
		if (!comma) {
			printf("You need to specify what output method you "
			       "want help on, as in\n"
			       "        -o help,%s\n", omethods[0]->name);
			exit(0);
		}
		for (i = 0; omethods[i]; i++) {
			if (!strcmp(omethods[i]->name, comma + 1)) {
				(*omethods[i]->help) ();
				exit(0);
			}
		}
		fprintf(stderr,
			"%s: unknown output method `%s' (use -h for help)\n",
			__progname, comma + 1);
		exit(0);
	}

	for (i = 0; omethods[i]; i++) {
		el = strlen(omethods[i]->name);
		if ((el == nl) && !strncmp(name, omethods[i]->name, nl)) {
			omethod = omethods[i];
			omarg = (*omethod->init) (comma ? comma + 1 : 0);
			return;
		}
	}
	fprintf(stderr, "%s: unknown output method `%s' (use -h for help)\n",
		__progname, name);

	exit(-1);
}

static void set_core(const char *name)
{
	const char *comma;
	int nl;
	int el;
	int i;

	comma = index(name, ',');
	nl = comma ? comma - name : strlen(name);
	if ((nl == 4) && !strncmp(name, "list", 4)) {
		for (i = 0; cores[i]; i++)
			printf("%s\n", cores[i]->name);
		exit(0);
	}

	if ((nl == 4) && !strncmp(name, "help", 4)) {
		if (!comma) {
			printf("You need to specify what core you want help "
			       "on, as in\n"
			       "        -o help,%s\n", cores[0]->name);
			exit(0);
		}
		for (i = 0; cores[i]; i++) {
			if (!strcmp(cores[i]->name, comma + 1)) {
				(*cores[i]->help) ();
				exit(0);
			}
		}
		fprintf(stderr, "%s: unknown core `%s' (use -h for help)\n",
			__progname, comma + 1);
		exit(0);
	}

	for (i = 0; cores[i]; i++) {
		el = strlen(cores[i]->name);
		if ((el == nl) && !bcmp(name, cores[i]->name, nl)) {
			core = cores[i];
			carg = (*core->init) (comma ? comma + 1 : 0);
			return;
		}
	}

	fprintf(stderr, "%s: unknown core `%s' (use -h for help)\n", __progname,
		name);

	exit(-1);
}

static void show_help(void)
{
	fprintf(stderr,
		"Usage: %s [options]\n"
		"Options can be:\n"
		"-h      Show this help\n"
		"-N      Suppress the actual parser call.\n"
		"-H      Compute/print metadata hashes.\n"
		"-v      show computation cost\n"
		"-d      enable debug messages\n"
		"-n N    Repeat each input packet a total of N times "
		"(default 1)\n"
		"-i NAME[,ARGS]\n"
		"        Use input method NAME; ARGS is an optional string "
		"which is\n"
		"        passed to the input method, which will presumably "
		"use it.\n"
		"        If NAME is `list', lists possible input methods "
		"(to stdout).\n"
		"        If NAME is `help', gives help on method named ARGS, "
		"as in\n"
		"                -i help,%s\n"
		"        For `list' and `help', does not start after "
		"printing.\n"
		"-o NAME[,ARGS]\n"
		"        Use output method NAME; ARGS is an optional string "
		"which is\n"
		"        passed to the output method, which will presumably "
		"use it.\n"
		"        If NAME is `list', lists possible output methods "
		"(to stdout).\n"
		"        If NAME is `help', gives help on method named ARGS, "
		"as in.\n"
		"                -o help,%s\n"
		"        For `list' and `help', does not start after "
		"printing.\n"
		"-c NAME[,ARGS]\n"
		"        Use computation core NAME; ARGS is an optional "
		"string which is\n"
		"        passed to the core module, which will presumably use "
		"it.\n"
		"        If NAME is `list', lists possible cores (to stdout).\n"
		"        If NAME is `help', gives help on core named ARGS, "
		"as in.\n"
		"                -o help,%s\n"
		"        For `list' and `help', does not start after "
		"printing.\n",
		__progname, imethods[0]->name, omethods[0]->name,
		cores[0]->name);
}

static void usage(char *progname)
{
	fprintf(stderr, "Usage: %s [-NHvd] [-n <number>] [-i <type>[,<arg>]] "
		"[-o <type>[,<arg>]] [-c <core>]\n", progname);

	exit(-1);
}

#define ARGS "n:NHi:o:c:hvd"

static struct option long_options[] = {
	{ "number", required_argument, 0, 'n' },
	{ "nocore", no_argument, 0, 'N' },
	{ "hash", no_argument, 0, 'H' },
	{ "input", required_argument, 0, 'i' },
	{ "output", required_argument, 0, 'o' },
	{ "core", required_argument, 0, 'c' },
	{ "verbose", no_argument, 0, 'v' },
	{ "debug", no_argument, 0, 'd' },
	{ NULL, 0, 0, 0 },
};

static void handleargs(int argc, char **argv)
{
	int option_index = 0;
	int c;

	while ((c = getopt_long(argc, argv, ARGS, long_options,
				&option_index)) != EOF) {
		switch (c) {
		case 'n':
			set_repeat(optarg);
			break;
		case 'N':
			coreflags |= CORE_F_NOCORE;
			break;
		case 'H':
			coreflags |= CORE_F_HASH;
			break;
		case 'v':
			coreflags |= CORE_F_VERBOSE;
			break;
		case 'd':
			coreflags |= CORE_F_DEBUG;
			break;
		case 'i':
			set_imethod(optarg);
			break;
		case 'o':
			set_omethod(optarg);
			break;
		case 'c':
			set_core(optarg);
			break;
		case 'h':
			show_help();
			exit(0);
		default:
			usage(argv[0]);
			exit(-1);
		}
	}
}

int main(int argc, char **argv)
{
	long long old_time = _time;
	long long avg = 0;
	int j = 0;

	handleargs(argc, argv);

	while (readpkt()) {
		int i;

		old_time = _time;
		_time = 0;

		for (i = repeat; i > 0; i--)
			processpkt();

		++j;
		avg = (_time / repeat);
		if (coreflags & CORE_F_VERBOSE)
			printf("Packet %d (repeated %d): avg %lld ns/p "
			       "%lld Mpps\n", j, repeat, avg,
							avg ? 1000 / avg : 0);
		_time += old_time;
	}

	avg = (_time / (j*repeat));
	if (coreflags & CORE_F_VERBOSE)
		printf("Total avg %lld ns/packet %lld Mpps\n", avg,
			avg ? 1000 / avg : 0);

	return 0;
}
