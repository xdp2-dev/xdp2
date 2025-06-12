/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
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

/* DD tests */

MAKE_PIPELINE((test2csz_dd, 3), D, pl_add_2, (P, 1), pl_add_1, D)
MAKE_PIPELINE((test2sz_dd, 3), D, pl_add_2, (D, 8888), pl_add_1, D)
MAKE_PIPELINE((test3sz_dd, 5), D, pl_add_2, (P, 100), pl_add_1, (D, 9000),
	      pl_add_2, D)
MAKE_PIPELINE((test4sz_dd, 6), D, pl_add_2, (D, 100000), pl_add_1, (P, 20),
	      pl_add_2, (D, 22222), pl_add_1, D)
MAKE_PIPELINE((test5sz_dd, 8), D, pl_add_2, (P, 10), pl_add_1, (P, 10),
	      pl_add_2, (P, 10), pl_add_1, (D, 100), pl_add_2, D)
MAKE_PIPELINE((test6sz_dd, 9), D, pl_add_2, (D, 1111), pl_add_1, (P, 3),
	      pl_add_2, (D, 1000000), pl_add_1, (P, 8), pl_add_2, (P, 4),
	      pl_add_1, D)
MAKE_PIPELINE((test7sz_dd, 11), D, pl_add_2, (D, 1024), pl_add_1, (D, 100),
	      pl_add_2, (D, 1024), pl_add_1, (D, 100), pl_add_2, (D, 1024),
	      pl_add_1, (D, 100), pl_add_2, D)
MAKE_PIPELINE((test8sz_dd, 12), D, pl_add_2, (P, 1), pl_add_1, (D, 111),
	      pl_add_2, (P, 10), pl_add_1, (D, 128000), pl_add_2, (P, 10),
	      pl_add_1, (D, 99999), pl_add_2, (P, 10), pl_add_1, D)
MAKE_PIPELINE((test9sz_dd, 14), D, pl_add_2, (D, 4001), pl_add_1, (D, 4000),
	      pl_add_2, (D, 3999), pl_add_1, (D, 3998), pl_add_2, (D, 3997),
	      pl_add_1, (D, 3996), pl_add_2, (D, 3995), pl_add_1, (D, 3994),
	      pl_add_2, D)
MAKE_PIPELINE((test10sz_dd, 15), D, pl_add_2, (P, 1), pl_add_1, (D, 88888),
	      pl_add_2, (P, 1), pl_add_1, (D, 128), pl_add_2, (P, 1),
	      pl_add_1, (D, 6666), pl_add_2, (P, 1), pl_add_1, (D, 444),
	      pl_add_2, (P, 1), pl_add_1, D)

MAKE_PIPELINE((test10Ksz_dd, 15), D, pl_add_2, (P, 1), pl_add_1, (D, 1),
	      pl_add_2, (P, 1), pl_add_1, (D, 1), pl_add_2, (P, 1),
	      pl_add_1, (D, 1), pl_add_2, (P, 1), pl_add_1, (D, 1),
	      pl_add_2, (P, 1), pl_add_1, D)

MAKE_PIPELINE((test10Ksz_dp, 15), D, pl_add_2, (P, 1), pl_add_1, (D, 1),
	      pl_add_2, (P, 1), pl_add_1, (D, 1), pl_add_2, (P, 1),
	      pl_add_1, (D, 1), pl_add_2, (P, 1), pl_add_1, (D, 1),
	      pl_add_2, (P, 1), pl_add_1, P)

MAKE_PIPELINE((test10Ksz_pp, 15), P, pl_add_2, (P, 1), pl_add_1, (D, 1),
	      pl_add_2, (P, 1), pl_add_1, (D, 1), pl_add_2, (P, 1),
	      pl_add_1, (D, 1), pl_add_2, (P, 1), pl_add_1, (D, 1),
	      pl_add_2, (P, 1), pl_add_1, P)

MAKE_PIPELINE((test10Ksz_pd, 15), P, pl_add_2, (P, 1), pl_add_1, (D, 1),
	      pl_add_2, (P, 1), pl_add_1, (D, 1), pl_add_2, (P, 1),
	      pl_add_1, (D, 1), pl_add_2, (P, 1), pl_add_1, (D, 1),
	      pl_add_2, (P, 1), pl_add_1, D)

MAKE_PIPELINE_NOSZ((test2, 3), D, pl_add_2, D, pl_add_1, D)
MAKE_PIPELINE_NOSZ((test3, 5), D, pl_add_2, D, pl_add_1, D, pl_add_2, D)
MAKE_PIPELINE_NOSZ((test4, 6), D, pl_add_2, D, pl_add_1, D, pl_add_2, D,
		   pl_add_1, D)
MAKE_PIPELINE_NOSZ((test5, 8), D, pl_add_2, D, pl_add_1, D, pl_add_2, D,
		   pl_add_1, D, pl_add_2, D)
MAKE_PIPELINE_NOSZ((test6, 9), D, pl_add_2, D, pl_add_1, D, pl_add_2, D,
		    pl_add_1, D, pl_add_2, D, pl_add_1, D)
MAKE_PIPELINE_NOSZ((test7, 11), D, pl_add_2, D, pl_add_1, D, pl_add_2, D,
		   pl_add_1, D, pl_add_2, D, pl_add_1, D, pl_add_2, D)
MAKE_PIPELINE_NOSZ((test8, 12), D, pl_add_2, D, pl_add_1, D, pl_add_2, D,
		   pl_add_1, D, pl_add_2, D, pl_add_1, D, pl_add_2, D,
		   pl_add_1, D)
MAKE_PIPELINE_NOSZ((test9, 14), D, pl_add_2, D, pl_add_1, D, pl_add_2, D,
		   pl_add_1, D, pl_add_2, D, pl_add_1, D, pl_add_2, D, pl_add_1,
		   D, pl_add_2, D);
MAKE_PIPELINE_NOSZ((test10, 15), D, pl_add_2, D, pl_add_1, D, pl_add_2, D,
		   pl_add_1, D, pl_add_2, D, pl_add_1, D, pl_add_2, D, pl_add_1,
		   D, pl_add_2, D, pl_add_1, D)

MAKE_PIPELINE((test2sz_pd, 3), P, pl_add_1, (P, 1), pl_add_2, D)
MAKE_PIPELINE((test3sz_pd, 4), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, D)
MAKE_PIPELINE((test4sz_pd, 6), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, D)
MAKE_PIPELINE((test5sz_pd, 7), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, D)
MAKE_PIPELINE((test6sz_pd, 9), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, D)
MAKE_PIPELINE((test7sz_pd, 10), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, D)
MAKE_PIPELINE((test8sz_pd, 12), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, (P, 50), pl_add_2, D)
MAKE_PIPELINE((test9sz_pd, 13), P, pl_add_1, (P, 50), pl_add_2, (P, 1),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, D)
MAKE_PIPELINE((test10sz_pd, 15), P, pl_add_2, (P, 100), pl_add_1, (D, 88888),
	      pl_add_2, (P, 1), pl_add_1, (D, 128), pl_add_2, (P, 1),
	      pl_add_1, (D, 6666), pl_add_2, (P, 1), pl_add_1, (D, 444),
	      pl_add_2, (P, 1), pl_add_1, D)
MAKE_PIPELINE((test10bsz_pd, 5), P, pl_add_2, (D, 88888),
	      pl_add_2, (P, 10), pl_add_1, D)
MAKE_PIPELINE((test4asz_pd, 6), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, D)
MAKE_PIPELINE((test5asz_pd, 7), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (D, 50), pl_add_2, (D, 5),
	 pl_add_1, D)
MAKE_PIPELINE((test6asz_pd, 8), P, pl_add_1, (D, 50), pl_add_2, (D, 100),
	      pl_add_1, (D, 5), pl_add_1, (D, 50), pl_add_1, (D, 50),
	      pl_add_2, D)
MAKE_PIPELINE((test10asz_pd, 14), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (D, 500), pl_add_1, (D, 50), pl_add_2, (D, 5),
	      pl_add_1, (D, 50), pl_add_1, D)

MAKE_PIPELINE((test2sz_pp, 3), P, pl_add_1, (P, 1), pl_add_2, P)
MAKE_PIPELINE((test3sz_pp, 4), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, P)
MAKE_PIPELINE((test4sz_pp, 6), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, P)
MAKE_PIPELINE((test5sz_pp, 7), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, P)
MAKE_PIPELINE((test6sz_pp, 9), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, P)
MAKE_PIPELINE((test7sz_pp, 10), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, P)
MAKE_PIPELINE((test8sz_pp, 12), P, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, (P, 50), pl_add_2, P)
MAKE_PIPELINE((test9sz_pp, 13), P, pl_add_1, (P, 50), pl_add_2, (P, 1),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, P)
MAKE_PIPELINE((test10sz_pp, 15), P, pl_add_2, (P, 100), pl_add_1, (D, 88888),
	      pl_add_2, (P, 1), pl_add_1, (D, 128), pl_add_2, (P, 1),
	      pl_add_1, (D, 6666), pl_add_2, (P, 1), pl_add_1, (D, 444),
	      pl_add_2, (P, 1), pl_add_1, P)

MAKE_PIPELINE((test2sz_dp, 3), D, pl_add_1, (P, 1), pl_add_2, P)
MAKE_PIPELINE((test3sz_dp, 4), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, P)
MAKE_PIPELINE((test4sz_dp, 6), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, P)
MAKE_PIPELINE((test5sz_dp, 7), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, P)
MAKE_PIPELINE((test6sz_dp, 9), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, P)
MAKE_PIPELINE((test7sz_dp, 10), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, P)
MAKE_PIPELINE((test8sz_dp, 12), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, (P, 50), pl_add_2, P)
MAKE_PIPELINE((test9sz_dp, 13), D, pl_add_1, (P, 50), pl_add_2, (P, 1),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, P)
MAKE_PIPELINE((test10sz_dp, 15), D, pl_add_2, (P, 100), pl_add_1, (D, 88888),
	      pl_add_2, (P, 1), pl_add_1, (D, 128), pl_add_2, (P, 1),
	      pl_add_1, (D, 6666), pl_add_2, (P, 1), pl_add_1, (D, 444),
	      pl_add_2, (P, 1), pl_add_1, P)

MAKE_PIPELINE((test2sz_dx, 3), D, pl_add_1, (P, 1), pl_add_2, P)
MAKE_PIPELINE((test3sz_dx, 4), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, X)
MAKE_PIPELINE((test4sz_dx, 6), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, X)
MAKE_PIPELINE((test5sz_dx, 7), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, X)
MAKE_PIPELINE((test6sz_dx, 9), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, X)
MAKE_PIPELINE((test7sz_dx, 10), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, X)
MAKE_PIPELINE((test8sz_dx, 12), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, (P, 50), pl_add_2, X)
MAKE_PIPELINE((test9sz_dx, 13), D, pl_add_1, (P, 50), pl_add_2, (P, 1),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, X)
MAKE_PIPELINE((test10sz_dx, 15), D, pl_add_2, (P, 100), pl_add_1, (D, 88888),
	      pl_add_2, (P, 1), pl_add_1, (D, 128), pl_add_2, (P, 1),
	      pl_add_1, (D, 6666), pl_add_2, (P, 1), pl_add_1, (D, 444),
	      pl_add_2, (P, 1), pl_add_1, X)

MAKE_PIPELINE((test2sz_px, 3), D, pl_add_1, (P, 1), pl_add_2, P)
MAKE_PIPELINE((test3sz_px, 4), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, X)
MAKE_PIPELINE((test4sz_px, 6), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, X)
MAKE_PIPELINE((test5sz_px, 7), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, X)
MAKE_PIPELINE((test6sz_px, 9), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, X)
MAKE_PIPELINE((test7sz_px, 10), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, X)
MAKE_PIPELINE((test8sz_px, 12), D, pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, (P, 50), pl_add_2, X)
MAKE_PIPELINE((test9sz_px, 13), D, pl_add_1, (P, 50), pl_add_2, (P, 1),
	      pl_add_1, (P, 50), pl_add_2, (P, 5), pl_add_1, (P, 50),
	      pl_add_2, (P, 5), pl_add_1, (P, 50), pl_add_2, (P, 5),
	      pl_add_1, X)
MAKE_PIPELINE((test10sz_px, 15), D, pl_add_2, (P, 100), pl_add_1, (D, 88888),
	      pl_add_2, (P, 1), pl_add_1, (D, 128), pl_add_2, (P, 1),
	      pl_add_1, (D, 6666), pl_add_2, (P, 1), pl_add_1, (D, 444),
	      pl_add_2, (P, 1), pl_add_1, X)

