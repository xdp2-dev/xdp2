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

/* Test for XDP2 switch
 *
 * Run: ./test_switch
 */

#include <linux/if_ether.h>
#include <linux/ipv6.h>
#include <linux/types.h>
#include <netinet/ip.h>
#include <stdio.h>

#include "xdp2/switch.h"

struct ethhdr ehdr;
struct ethhdr ehdr2;

int a, b, c;

int main(int argc, char *argv[])
{
	XDP2_SWITCH_START(&ehdr)
	XDP2_SWITCH_CASE_EQUAL(ehdr2) {
		printf("Hello world\n");
		break;
	}
	XDP2_SWITCH_CASE_EQUAL_CONST(({ 0x0, 0x11, 0x11, 0x11, 0x11, 0x11 },
				      { 0x0, 0x11, 0x11, 0x11, 0x11, 0x22 },
				      ETH_P_IP)) {
		printf("Hello world\n");
		break;
	}
	XDP2_SWITCH_CASE_TERN(ehdr2, ehdr2) {
		printf("Hello world2\n");
		break;
	}
	XDP2_SWITCH_CASE_TERN_CONST(({ 0x0, 0x11, 0x11, 0x11, 0x11, 0x11 },
				     { 0x0, 0x11, 0x11, 0x11, 0x11, 0x22 },
				     ETH_P_IP),
				    ({ 0x0, 0x11, 0x11, 0x11, 0x11, 0x11 },
				     { 0x0, 0x11, 0x11, 0x11, 0x11, 0x22 },
				     ETH_P_IP)) {
		printf("Hello world33\n");
		break;
	}
	XDP2_SWITCH_CASE_PREFIX(ehdr2, 23)
		break;
	XDP2_SWITCH_CASE_PREFIX(ehdr2, 22) {
		printf("Hello world3\n");
		break;
	}
	XDP2_SWITCH_CASE_PREFIX_CONST(({ 0x0, 0x11, 0x11, 0x11, 0x11, 0x11 },
				       { 0x0, 0x11, 0x11, 0x11, 0x11, 0x22 },
				       ETH_P_IP), 43) {
		printf("Hello world44\n");
		break;
	}

	XDP2_SWITCH_DEFAULT() printf("No hits\n");
	XDP2_SWITCH_END();

	XDP2_SELECT_START(a, b, c)
	XDP2_SELECT_CASE(10,, 54) {
		printf("Hello world\n");
		break;
	}
	XDP2_SELECT_CASE(,, 54) {
		printf("Hello world 22\n");
		break;
	}
	XDP2_SELECT_CASE(10, 54) {
		printf("Part 2\n");
		break;
	}
	XDP2_SELECT_CASE_RANGE((1, 5), (10, 10), (20, 30)) {
		printf("Part 2\n");
		break;
	}
	XDP2_SELECT_CASE_RANGE((1, 5), (88,), (20, 30)) {
		printf("Part 2\n");
		break;
	}
	XDP2_SELECT_CASE_RANGE((1, 5), (,), (20, 30)) {
		printf("Part 2\n");
		break;
	}
	XDP2_SELECT_CASE_RANGE((1, 5), (,99), (20, 30)) {
		printf("Part 2\n");
		break;
	}
	XDP2_SELECT_CASE_MASK((1, 5), (10, 0x33), (20, 0x3f)) {
		printf("Part 2\n");
		break;
	}
	XDP2_SELECT_CASE_MASK((1, 5), (,), (20, 0x3f)) {
		printf("Part 2\n");
		break;
	}
	XDP2_SELECT_CASE_MASK_ANY(0x1, 0xff, 2) {
		printf("Part 2\n");
		break;
	}
	XDP2_SELECT_CASE_MASK_ANY(0x1, , 2) {
		printf("Part 2\n");
		break;
	}
	XDP2_SELECT_DEFAULT() printf("No hits\n");
	XDP2_SELECT_END();
}
