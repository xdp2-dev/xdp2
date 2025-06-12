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

/* Test for XDP2 accelerators
 *
 * Run: ./test_accel [ -c <test-count> ] [ -v <verbose> ]
 *		     [ -I <report-interval> ][ -C <cli_port_num> ]
 *		     [-R] [ -t <test> ] [ -B <byte-count> ]
 */

#include <errno.h>
#include <getopt.h>
#include <time.h>

#include "xdp2/accelerator.h"
#include "xdp2/pvbuf.h"
#include "xdp2/utility.h"

#define MAX_IPKTS 100
#define MAX_OPKTS 100
#define MAX_BYTES 10000000
#define CONSEC_NO_OUT 10000000
#define NUM_TESTS 100000

/* Set the block size divisor to 95, that seems to be the maximum value
 * without getting output packets stuck
 */
#define BLOCK_SIZE_DIV 95

xdp2_paddr_t input_pkt[MAX_IPKTS];

xdp2_paddr_t big_input_pkt;

static int verbose;

struct iterate_struct {
	size_t offset; /* Offset for skipping bytes */
	size_t len; /* Output length remaining */
	__u8 *obytes; /* Out bytes pointer */
	ssize_t olen; /* Output length of bytes */
	unsigned int add_val; /* Add value for each byte */
};

struct arg_struct {
	size_t offset; /* Start offset in a hold packet */
	xdp2_paddr_t hold_packet; /* Help packet for inpute */
	size_t block_size; /* Block size */
	unsigned int targ_val; /* Check value base */
	int tnum; /* Test number */
	size_t all_count; /* Total byte count for reporting */
	int base; /* Index in whole output */
	unsigned int consec_no_out; /* Track if pipeline is stuck */
};

struct iterate_check_struct {
	__u8 val; /* Next check value */
	int tnum; /* Test number */
	int base; /* Index in whole output */
	size_t all_count; /* Total byte count for reporting */
};

/* Test case counts */
struct pipeline_counts {
	unsigned long long used;
};

struct pipeline_counts *pipeline_counts;

enum test_types {
	TEST_DD,
	TEST_DP,
	TEST_DX,
	TEST_PP,
	TEST_PD,
	TEST_PX,
};

struct my_pipeline_s {
	void *pfunc;
	int value;
	const char *name;
	enum test_types type;
};

#if 0
static bool print_iterate(void *priv, __u8 *data, size_t len)
{
	int i;

	for (i = 0; i < len; i++)
		printf("D: %u\n", data[i]);
	return true;
}

static void print_packet(xdp2_paddr_t pkt)
{
	xdp2_pvbuf_iterate(pkt, print_iterate, NULL);
}
#endif

static bool zero_iterate(void *priv, __u8 *data, size_t len)
{
	memset(data, 0, len);

	return true;
}

static void zero_packets(void)
{
	int i;

	for (i = 0; i < MAX_IPKTS; i++)
		xdp2_pvbuf_iterate(input_pkt[i], zero_iterate, NULL);

	xdp2_pvbuf_iterate(big_input_pkt, zero_iterate, NULL);
}

static bool init_iterate(void *priv, __u8 *data, size_t len)
{
	struct iterate_struct *ist = priv;
	__u8 v = ist->add_val;
	int i;

	for (i = 0; i < len; v++, i++)
		data[i] = v;

	ist->add_val = v;

	return true;
}

static void init_packet(xdp2_paddr_t pkt)
{
	struct iterate_struct ist = {};

	xdp2_pvbuf_iterate(pkt, init_iterate, &ist);
}

static void iadd(__u8 *ibytes, __u8 *obytes, size_t len,
		 unsigned int v)
{
	int i;

	for (i = 0; i < len; i++)
		obytes[i] = ibytes[i] + v;
}

/* Check a range of bytes for the expected value */
static void check_expected(size_t all_count, int byte_count, int base,
			   __u8 *data, __u8 val, int tnum)
{
	int i;

	for (i = 0; i < byte_count; i++) {
		if (data[i] == (__u8)(val + i))
			continue;

		printf("Mismatch test: %u, byte count: %lu, "
		       "#%u: Got %u expected %u\n", tnum,
		       all_count, i + base, data[i], (__u8)(val + i));
		exit(1);
	}
}

static bool check_iterate(void *priv, __u8 *data, size_t len)
{
	struct iterate_check_struct *ist = priv;

	check_expected(ist->all_count, len, ist->base, data,
		       ist->val, ist->tnum);

	ist->val = (__u8)(ist->val + len);
	ist->base += len;

	return true;
}

/* Check packet data for the expected value */
static size_t check_expected_pkts(size_t all_count, xdp2_paddr_t *pkts,
				  int pkt_count, __u8 val, int tnum,
				  size_t base)
{
	struct iterate_check_struct ist;
	int i;

	ist.val = val;
	ist.tnum = tnum;
	ist.base = base;
	ist.all_count = all_count;

	for (i = 0; i < pkt_count; i++)
		if (pkts[i])
			xdp2_pvbuf_iterate(pkts[i], check_iterate, &ist);

	return ist.base - base;
}

/* DD handlers */

static ssize_t add_dd(__u8 *ibytes, size_t ibytes_size,
		      __u8 *obytes, size_t obytes_size,
		      size_t *ibytes_consumed, void *arg, unsigned int val)
{
	struct arg_struct *marg = (struct arg_struct *)arg;
	ssize_t len = xdp2_min(ibytes_size, obytes_size);

	if (!obytes_size) {
		marg->consec_no_out++;
		if (marg->consec_no_out == CONSEC_NO_OUT)
			XDP2_WARN("DD pipeline is stuck on output");
		*ibytes_consumed = 0;
		return 0;
	}

	marg->consec_no_out = 0;

	iadd(ibytes, obytes, len, val);

	*ibytes_consumed = len;

	return len;
}

static ssize_t add_1_dd(__u8 *ibytes, size_t ibytes_size,
			__u8 *obytes, size_t obytes_size,
			size_t *ibytes_consumed, void *arg)
{
	return add_dd(ibytes, ibytes_size, obytes, obytes_size,
		      ibytes_consumed, arg, 1);
}

static ssize_t add_2_dd(__u8 *ibytes, size_t ibytes_size,
			__u8 *obytes, size_t obytes_size,
			size_t *ibytes_consumed, void *arg)
{
	return add_dd(ibytes, ibytes_size, obytes, obytes_size,
		      ibytes_consumed, arg, 2);
}

/* PD handlers */

static bool add_iterate_pd(void *priv, __u8 *data, size_t orig_len)
{
	struct iterate_struct *ist = priv;
	size_t len;

	if (!orig_len) {
		/* The original length is zero, nothing to do just return
		 * true to continue to next pbuf
		 */
		return true;
	}

	if (ist->offset) {
		/* We're skipping bytes, probably processing a held packet */
		if (orig_len <= ist->offset) {
			/* Still haven't seen enough bytes */
			ist->offset -= orig_len;
			return true;
		}

		data += ist->offset;
		orig_len -= ist->offset;

		ist->offset = 0;
	}

	if (!ist->len) {
		/* There's no output space available, return false as
		 * we need to hold packet until some space opens up
		 */
		return false;
	}

	len = xdp2_min(ist->len, orig_len); /* We know this is non-zero */

	iadd((__u8 *)data, ist->obytes, len, ist->add_val);
	ist->obytes += len;
	ist->olen += len;
	ist->len -= len;

	if (!ist->len && len != orig_len) {
		/* We consumed all the output space and still have input
		 * to process
		 */
		return false;
	}

	return true;
}

static ssize_t add_pd(void **ipkts, unsigned int ipkts_cnt,
		      __u8 *obytes, size_t obytes_size,
		      unsigned int *pkts_processed, void *arg,
		      unsigned int val)
{
	struct arg_struct *marg = (struct arg_struct *)arg;
	xdp2_paddr_t *pkt = (xdp2_paddr_t *)ipkts;
	struct iterate_struct ist = { .add_val = val };
	size_t all_len = 0;

	*pkts_processed = 0;

	ist.len = obytes_size;
	ist.obytes = obytes;

	if (marg->hold_packet) {
		/* We have a leftofver packet */
		ist.offset = marg->offset;
		ist.olen = 0;
		if (xdp2_pvbuf_iterate(marg->hold_packet,
				       add_iterate_pd, &ist)) {
			/* Free packet we consumed */
			xdp2_pvbuf_free(marg->hold_packet);

			/* Finished held packet, clear hold_packet pointer */
			marg->hold_packet = 0;
			marg->offset = 0;
			all_len += ist.olen;
		} else {
			/* We still have to hold the packet, if some bytes
			 * we're output add into marg->offset
			 */
			marg->offset += ist.olen;

			marg->consec_no_out++;
			if (marg->consec_no_out == CONSEC_NO_OUT)
				XDP2_WARN("PD pipeline is stuck on "
					       "output");

			/* If we actually wrote some bytes return the
			 * number, if not return -EAGAIN to indicate that
			 * we need to be called again to finish the packet
			 */
			return ist.olen ? : -EAGAIN;
		}
	}

	if (!obytes_size) {
		marg->consec_no_out++;
		if (marg->consec_no_out == CONSEC_NO_OUT)
			XDP2_WARN("PD pipeline is stuck on output");
		return 0;
	}

	marg->consec_no_out = 0;

	if (!ipkts_cnt) {
		/* Caller should be signaling that pipeline stage is done.
		 * There's no held packet here so just return 0 and caller
		 * indicating we're done
		 */
		return all_len;
	}

	ist.offset = 0;

	while (ipkts_cnt) {
		ist.olen = 0;

		/* Either iterate we'll completely process the packet or
		 * it's going to be held. Either way the packet is consumed
		 * so increment packets processed
		 */
		(*pkts_processed)++;

		if (xdp2_pvbuf_iterate(*pkt, add_iterate_pd, &ist)) {
			/* All good proceed to next packet */

			/* Free packet we consumed */
			xdp2_pvbuf_free(*pkt);

			ipkts_cnt--;
			pkt++;
			all_len += ist.olen; /* Output bytes */
		} else {
			/* Didn't complete packet so it needs to be held */
			marg->offset = ist.olen;
			marg->hold_packet = *pkt;
			all_len += ist.olen; /* Output bytes */
			break;
		}
	}

	return all_len;
}

static ssize_t add_1_pd(void **ipkts, unsigned int ipkts_cnt,
			__u8 *obytes, size_t obytes_size,
			unsigned int *ipkts_processed, void *args)
{
	return add_pd(ipkts, ipkts_cnt, obytes, obytes_size,
		      ipkts_processed, args, 1);
}

static ssize_t add_2_pd(void **ipkts, unsigned int ipkts_cnt,
			__u8 *obytes, size_t obytes_size,
			unsigned int *pkts_processed, void *args)
{
	return add_pd(ipkts, ipkts_cnt, obytes, obytes_size,
		      pkts_processed, args, 2);
}

/* PP handlers */

static bool add_iterate_pp(void *priv, __u8 *data, size_t len)
{
	struct iterate_struct *ist = priv;

	iadd(data, data, len, ist->add_val);

	return true;
}


static int add_pp(void **ipkts, unsigned int ipkts_cnt,
		  void **opkts, unsigned int opkts_cnt,
		  unsigned int *pkts_processed, void *arg,
		  unsigned int val)
{
	struct arg_struct *marg = (struct arg_struct *)arg;
	xdp2_paddr_t *pkt = (xdp2_paddr_t *)ipkts;
	unsigned int processed = 0;
	struct iterate_struct ist = { .add_val = val };

	if (!ipkts_cnt) {
		*pkts_processed = 0;
		return 0;
	}

	if (!opkts_cnt) {
		marg->consec_no_out++;
		if (marg->consec_no_out == CONSEC_NO_OUT)
			XDP2_WARN("PP pipeline is stuck on output");
		*pkts_processed = 0;
		return 0;
	}

	marg->consec_no_out = 0;

	while (ipkts_cnt && opkts_cnt) {
		if (xdp2_pvbuf_iterate(*pkt, add_iterate_pp, &ist)) {
			*opkts = (void *)*pkt;
			opkts++;
			opkts_cnt--;
			pkt++;
			ipkts_cnt--;
			processed++;
		} else {
			XDP2_ERR(1, "Trouble");
		}
	}

	*pkts_processed = processed; /* Input packets processed */

	return processed; /* Output packets created */
}

static int add_1_pp(void **ipkts, unsigned int ipkts_cnt,
		    void **opkts, unsigned int opkts_cnt,
		    unsigned int *pkts_processed, void *arg)
{
	return add_pp(ipkts, ipkts_cnt, opkts, opkts_cnt, pkts_processed,
		      arg, 1);
}

static int add_2_pp(void **ipkts, unsigned int ipkts_cnt,
		    void **opkts, unsigned int opkts_cnt,
		    unsigned int *pkts_processed, void *arg)
{
	return add_pp(ipkts, ipkts_cnt, opkts, opkts_cnt, pkts_processed,
		      arg, 2);
}

/* DP handlers */

static ssize_t add_dp(__u8 *ibytes, size_t ibytes_size,
		      void **opkts, unsigned int opkts_cnt,
		      size_t *ibytes_consumed, void *arg, unsigned int val)
{
	struct arg_struct *marg = (struct arg_struct *)arg;
	size_t offset = 0, tlen, clen, olen = ibytes_size;
	struct iterate_struct ist = { .add_val = val };
	xdp2_paddr_t packet = XDP2_PADDR_NULL;
	struct xdp2_pvbuf *pvbuf;
	int out_pkts = 0;

	*ibytes_consumed = 0;

	if (!opkts_cnt) {
		marg->consec_no_out++;
		if (marg->consec_no_out == CONSEC_NO_OUT)
			XDP2_WARN("DP pipeline is stuck on output");
		return 0;
	}

	marg->consec_no_out = 0;

	if (!ibytes_size) {
		if (!marg->hold_packet || !opkts_cnt)
			return 0;

		/* Output the partially filled last packet */

		/* Trim length */
		xdp2_pvbuf_pop_trailers(marg->hold_packet,
					marg->block_size - marg->offset,
					false);

		XDP2_ASSERT(xdp2_pvbuf_iterate(marg->hold_packet,
					       add_iterate_pp, &ist),
			    "Bad iterate return value");

		*opkts = (void *)marg->hold_packet;
		opkts++;
		marg->hold_packet = XDP2_PADDR_NULL;

		return 1;
	}

	while (ibytes_size && opkts_cnt) {
		if (marg->hold_packet) {
			packet = marg->hold_packet;
			offset = marg->offset;
			marg->hold_packet = XDP2_PADDR_NULL;
		} else {
			packet = xdp2_pvbuf_alloc_params(marg->block_size, 0,
							 0, &pvbuf);
			XDP2_ASSERT(packet, "Packet alloc failed");
			offset = 0;
		}

		tlen = xdp2_min(ibytes_size, marg->block_size - offset);
		clen = xdp2_pvbuf_copy_data_to_pvbuf(packet, ibytes,
						     tlen, offset);
		XDP2_ASSERT(tlen == clen, "Bad pvbuf copy");

		ibytes += tlen;
		ibytes_size -= tlen;

		if (offset + tlen == marg->block_size) {
			/* Filled a packet, send it on */
			XDP2_ASSERT(xdp2_pvbuf_iterate(packet,
						       add_iterate_pp,
						       &ist),
				    "Bad iterate return value");
			*opkts = (void *)packet;
			opkts++;
			out_pkts++;
			offset = 0;
			opkts_cnt--;
		} else {
			/* Packet isn't filled yet, just increment offset */
			offset +=  tlen;
		}
	}

	if (offset) {
		/* have a partially filled packet, record it */
		marg->hold_packet = packet;
		marg->offset = offset;
	}

	*ibytes_consumed = olen - ibytes_size;

	return out_pkts;
}

static int add_1_dp(__u8 *ibytes, size_t ibytes_size,
		    void **opkts, unsigned int opkts_cnt,
		    size_t *bytes_processed, void *arg)
{
	return add_dp(ibytes, ibytes_size, opkts, opkts_cnt, bytes_processed,
		      arg, 1);
}

static int add_2_dp(__u8 *ibytes, size_t ibytes_size,
		    void **opkts, unsigned int opkts_cnt,
		    size_t *bytes_processed, void *arg)
{
	return add_dp(ibytes, ibytes_size, opkts, opkts_cnt, bytes_processed,
		      arg, 2);
}

/* DX handlers */

static void add_dx(__u8 *ibytes, size_t bytes_size,
		   size_t *ibytes_consumed, void *arg, unsigned int val)
{
	struct arg_struct *marg = (struct arg_struct *)arg;

	check_expected(marg->all_count, bytes_size, marg->base,
		       ibytes, marg->targ_val - val, marg->tnum);

	*ibytes_consumed = bytes_size;

	marg->targ_val += bytes_size;
	marg->base += bytes_size;
}

static void add_1_dx(__u8 *ibytes, size_t bytes_size,
		     size_t *ibytes_consumed, void *arg)
{
	add_dx(ibytes, bytes_size, ibytes_consumed, arg, 1);
}

static void add_2_dx(__u8 *ibytes, size_t bytes_size,
		     size_t *ibytes_consumed, void *arg)
{
	add_dx(ibytes, bytes_size, ibytes_consumed, arg, 2);
}

/* PX handlers */

static void add_px(void **ipkts, unsigned int ipkts_cnt,
		   unsigned int *pkts_processed, void *arg,
		   unsigned int val)
{
	struct arg_struct *marg = (struct arg_struct *)arg;
	xdp2_paddr_t *pkt = (xdp2_paddr_t *)ipkts;
	size_t ret;
	int i;

	if (!ipkts_cnt) {
		*pkts_processed = 0;
		return;
	}

	ret = check_expected_pkts(marg->all_count, pkt, ipkts_cnt,
				  marg->targ_val - val, marg->tnum,
				  marg->base);

	for (i = 0; i < ipkts_cnt; i++)
		xdp2_pvbuf_free(pkt[i]);

	*pkts_processed = ipkts_cnt; /* Input packets processed */

	marg->targ_val += ret;
	marg->base += ret;
}

static void add_2_px(void **ipkts, unsigned int ipkts_cnt,
		     unsigned int *ipkts_consumed, void *arg)
{
	add_px(ipkts, ipkts_cnt, ipkts_consumed, arg, 2);
}

static void add_1_px(void **ipkts, unsigned int ipkts_cnt,
		     unsigned int *ipkts_consumed, void *arg)
{
	add_px(ipkts, ipkts_cnt, ipkts_consumed, arg, 1);
}

/* Accelerator structures */

static const struct xdp2_accelerator pl_add_1 = {
	.handler_dd = add_1_dd,
	.handler_dp = add_1_dp,
	.handler_dx = add_1_dx,
	.handler_pp = add_1_pp,
	.handler_pd = add_1_pd,
	.handler_px = add_1_px,
};

static const struct xdp2_accelerator pl_add_2 = {
	.handler_dd = add_2_dd,
	.handler_dp = add_2_dp,
	.handler_dx = add_2_dx,
	.handler_pp = add_2_pp,
	.handler_pd = add_2_pd,
	.handler_px = add_2_px,
};

XDP2_DEFINE_SECTION(all_my_piplines, struct my_pipeline_s)

#define MAKE_ENUM(X, Y) XDP2_JOIN3(TEST_, X, Y)

#define FIRST_ARG(ARG, ...) ARG

#define MAKE_PIPELINE_STRUCT(NAME, VALUE, TYPE)				\
	struct my_pipeline_s XDP2_JOIN2(NAME, _x)			\
	    XDP2_SECTION_ATTR(all_my_piplines) = {			\
		.type = TYPE,						\
		.pfunc = NAME,						\
		.value = VALUE,						\
		.name = XDP2_STRING_IT(NAME),				\
	};

#define MAKE_PIPELINE2(...) XDP2_PIPELINE_SZ(__VA_ARGS__)

#define MAKE_PIPELINE(PAIR, ...)					\
	MAKE_PIPELINE2(XDP2_GET_POS_ARG2_1 PAIR, __VA_ARGS__)		\
	MAKE_PIPELINE_STRUCT(XDP2_GET_POS_ARG2_1 PAIR,			\
			     XDP2_GET_POS_ARG2_2 PAIR,			\
				MAKE_ENUM(FIRST_ARG(__VA_ARGS__),	\
				XDP2_PMACRO_LASTARG(__VA_ARGS__)))

#define MAKE_PIPELINE2_NOSZ(...) XDP2_PIPELINE(__VA_ARGS__)

#define MAKE_PIPELINE_NOSZ(PAIR, ...)					\
	MAKE_PIPELINE2_NOSZ(XDP2_GET_POS_ARG2_1 PAIR, __VA_ARGS__)	\
	MAKE_PIPELINE_STRUCT(XDP2_GET_POS_ARG2_1 PAIR,			\
			     XDP2_GET_POS_ARG2_2 PAIR,			\
				MAKE_ENUM(FIRST_ARG(__VA_ARGS__),	\
				XDP2_PMACRO_LASTARG(__VA_ARGS__)))

#include "test_cases.h"

static void set_targ_value(void **args, unsigned int val, size_t all_count,
			   unsigned int tnum)
{
	int i;

	for (i = 0; i < XDP2_PIPELINE_MAX_STAGES; i++) {
		struct arg_struct *marg = (struct arg_struct *)args[i];

		marg->targ_val = val;
		marg->all_count = all_count;
		marg->tnum = tnum;
	}
}

/* Run an instance with packets as input */
static void run_one_p_(__u8 *data, struct my_pipeline_s *pline, int tnum,
		       enum xdp2_pipeline_type_enum out_type,
		       unsigned int byte_count, unsigned int block_size_div)
{
	struct arg_struct all_args[XDP2_PIPELINE_MAX_STAGES];
	size_t offset = 0, retlen, base_length;
	void *args[XDP2_PIPELINE_MAX_STAGES];
	unsigned int error, error_stage;
	xdp2_pipeline_common_t zfunc;
	xdp2_paddr_t pkts[MAX_IPKTS];
	size_t block_size;
	int i, ipkts_cnt;

	memset(all_args, 0, sizeof(all_args));

	for (i = 0; i < XDP2_PIPELINE_MAX_STAGES; i++)
		args[i] = &all_args[i];

	if (!byte_count)
		byte_count = random() % MAX_BYTES;

	ipkts_cnt = (random() % (MAX_IPKTS - 1)) + 1;

	if (!block_size_div)
		block_size = (byte_count / BLOCK_SIZE_DIV) + 1;
	else
		block_size = (byte_count / block_size_div) + 1;

	for (i = 0; i < XDP2_PIPELINE_MAX_STAGES; i++)
		all_args[i].block_size = block_size;

	/* Clear the output memory block */
	memset(data, 0, byte_count);

	/* Intialize the big packet as 0, 1, 2, ..., 255, 0, 1, ... */
	init_packet(big_input_pkt);

	/* Determine a length for each packet */
	base_length = byte_count / ipkts_cnt;

	/* Clone the big packet to make a bunch of lttle packets whose
	 * length adds up to byte count. The offset is set such that the
	 * clone packets could be reassmbled to the big packet
	 */
	memset(pkts, 0, sizeof(pkts));
	for (i = 0; i < ipkts_cnt - 1; i++, byte_count -= base_length) {
		pkts[i] = xdp2_pvbuf_clone(big_input_pkt, offset,
					   base_length, &retlen);
		XDP2_ASSERT(pkts[i], "Clone failed");
		offset += retlen;
	}

	/* Last packet contains any residual */
	pkts[ipkts_cnt - 1] = xdp2_pvbuf_clone(big_input_pkt, offset,
					       byte_count, &retlen);
	XDP2_ASSERT(pkts[ipkts_cnt - 1], "Clone failed");

	offset += retlen;

	byte_count = offset;

	if (verbose >= 10)
		printf("Run P-in %s for %u byte count, %u packets, "
		       "%lu block-size (tnum %u)\n",
		       pline->name, byte_count, ipkts_cnt, block_size, tnum);

	switch (out_type) {
	case XDP2_PIPELINE_P:
		xdp2_paddr_t opkts[MAX_OPKTS];

		memset(opkts, 0, sizeof(opkts));

		zfunc = (xdp2_pipeline_common_t)pline->pfunc;
		zfunc((void **)pkts, ipkts_cnt, (void **)opkts, MAX_OPKTS,
		       &error, &error_stage, args);

		check_expected_pkts(byte_count, opkts, MAX_OPKTS, pline->value,
				    tnum, 0);

		for (i = 0; i < MAX_OPKTS; i++)
			if (opkts[i])
				xdp2_pvbuf_free(opkts[i]);
		break;

	case XDP2_PIPELINE_D:
		zfunc = (xdp2_pipeline_common_t)pline->pfunc;
		zfunc((void **)pkts, ipkts_cnt, data, byte_count, &error,
			 &error_stage, args);

		check_expected(byte_count, byte_count, 0, data,
			       pline->value, tnum);
		break;

	case XDP2_PIPELINE_X:
		set_targ_value(args, pline->value, byte_count, tnum);

		zfunc = (xdp2_pipeline_common_t)pline->pfunc;

		zfunc((void **)pkts, ipkts_cnt, (void **)opkts, MAX_OPKTS,
		      &error,
			 &error_stage, args);

		break;
	}
}

/* Run an instance with data as input */
static void run_one_d_(__u8 *data, struct my_pipeline_s *pline, int tnum,
		       enum xdp2_pipeline_type_enum out_type,
		       unsigned int byte_count, unsigned int block_size_div)
{
	struct arg_struct all_args[XDP2_PIPELINE_MAX_STAGES];
	void *args[XDP2_PIPELINE_MAX_STAGES];
	unsigned int error, error_stage;
	xdp2_pipeline_common_t zfunc;
	size_t block_size;
	int i;

	for (i = 0; i < XDP2_PIPELINE_MAX_STAGES; i++)
		args[i] = &all_args[i];

	memset(all_args, 0, sizeof(all_args));

	if (!byte_count)
		byte_count = random() % MAX_BYTES;

	if (block_size_div)
		block_size = (byte_count / block_size_div) + 1;
	else
		block_size = (byte_count / BLOCK_SIZE_DIV) + 1;

	for (i = 0; i < XDP2_PIPELINE_MAX_STAGES; i++)
		all_args[i].block_size = block_size;

	for (i = 0; i < byte_count; i++)
		data[i] = (__u8)i;

	if (verbose >= 10)
		printf("Run D-in %s for %u byte count, "
		       "%lu block-size (tnum %u)\n",
		       pline->name, byte_count, block_size, tnum);

	switch (out_type) {
	case XDP2_PIPELINE_P: {
		xdp2_paddr_t opkts[MAX_OPKTS];

		memset(opkts, 0, sizeof(opkts));

		zfunc = (xdp2_pipeline_common_t)pline->pfunc;
		zfunc(data, byte_count, opkts, MAX_OPKTS, &error,
		      &error_stage, args);

		check_expected_pkts(byte_count, opkts, MAX_OPKTS,
				    pline->value, tnum, 0);

		for (i = 0; i < MAX_OPKTS; i++)
			if (opkts[i])
				xdp2_pvbuf_free(opkts[i]);
		break;
	}

	case XDP2_PIPELINE_D:
		zfunc = (xdp2_pipeline_common_t)pline->pfunc;
		zfunc(data, byte_count, data, byte_count, &error,
		      &error_stage, args);

		check_expected(byte_count, byte_count, 0, data,
			       pline->value, tnum);
		break;

	case XDP2_PIPELINE_X:
		set_targ_value(args, pline->value, byte_count, tnum);

		zfunc = (xdp2_pipeline_common_t)pline->pfunc;
		zfunc(data, byte_count, data, byte_count, &error,
		      &error_stage, args);

		break;
	}
}

/* Run the tests */
static void run_all(unsigned long num_tests, const char *run_test,
		    size_t byte_count, size_t block_size, bool kill_tests,
		    int intv)
{
	unsigned int num_my_pipes = xdp2_section_array_size_all_my_piplines();
	struct my_pipeline_s *plines = xdp2_section_base_all_my_piplines();
	struct my_pipeline_s *pline = NULL;
	unsigned long i;
	int which = 0;
	__u8 *data;

	data = calloc(1, MAX_BYTES);
	if (!data)
		exit(1);

	if (run_test) {
		for (i = 0; i < num_my_pipes; i++) {
			pline = &plines[i];

			if (!strcmp(pline->name, run_test)) {
				which = i;
				break;
			}
		}

		if (i >= num_my_pipes)
			XDP2_ERR(1, "Unable to find pipline %s", run_test);
	}

	for (i = 0; i < num_tests; i++) {
		if (intv && !(i % intv) && i)
			printf("I: %lu\n", i);

		if (!run_test) {
			do {
				which = random() % num_my_pipes;
				pline = &plines[which];
			} while (!kill_tests && strchr(pline->name, 'K'));
		}

		pipeline_counts[which].used++;

		switch (pline->type) {
		case TEST_DD:
			run_one_d_(data, pline, which, XDP2_PIPELINE_D,
				   byte_count, block_size);
			break;
		case TEST_DP:
			run_one_d_(data, pline, which, XDP2_PIPELINE_P,
				   byte_count, block_size);
			break;
		case TEST_DX:
			run_one_d_(data, pline, which, XDP2_PIPELINE_X,
				   byte_count, block_size);
			break;
		case TEST_PD:
			run_one_p_(data, pline, which, XDP2_PIPELINE_D,
				   byte_count, block_size);
			break;
		case TEST_PP:
			run_one_p_(data, pline, which, XDP2_PIPELINE_P,
				   byte_count, block_size);
			break;
		case TEST_PX:
			run_one_p_(data, pline, which, XDP2_PIPELINE_X,
				   byte_count, block_size);
			break;
		default:
		}
	}
}

/* Allocate PVbufs */
struct xdp2_pbuf_init_allocator pbuf_allocs = {
	.obj[13].num_objs = 20000
};

struct xdp2_pvbuf_init_allocator pvbuf_allocs = {
	.obj[5].num_pvbufs = 1000000
};

static void make_pvbuf(void)
{
	struct xdp2_pvbuf *pvbuf;
	int i;

	xdp2_pvbuf_init(&pbuf_allocs, &pvbuf_allocs, false, false, NULL, NULL);

	for (i = 0; i < MAX_IPKTS; i++) {
		input_pkt[i] = xdp2_pvbuf_alloc_params(MAX_BYTES / MAX_IPKTS,
						       0, 0, &pvbuf);
		XDP2_ASSERT(input_pkt[i], "PVbuf alloc fail");
	}

	big_input_pkt = xdp2_pvbuf_alloc_params(MAX_BYTES, 0, 0, &pvbuf);
	XDP2_ASSERT(big_input_pkt, "PVbuf alloc fail");

	zero_packets();
}

/* CLI commands */

XDP2_PVBUF_SHOW_BUFFER_MANAGER_CLI();

XDP2_CLI_MAKE_NUMBER_SET(verbose, verbose);

static void show_pipelines(void *cli,
		struct xdp2_cli_thread_info *info, const char *arg)
{
	unsigned int num_my_pipes = xdp2_section_array_size_all_my_piplines();
	struct my_pipeline_s *plines = xdp2_section_base_all_my_piplines();
	int i;

	for (i = 0; i < num_my_pipes; i++) {
		struct my_pipeline_s *pline = &plines[i];

		XDP2_CLI_PRINT(cli, "%s: count %llu\n",
			       pline->name, pipeline_counts[i].used);
	}
}

XDP2_CLI_ADD_SHOW_CONFIG("pipelines", show_pipelines, 0xffff);

#define ARGS "c:v:I:C:Rt:B:b:kP:"

static void usage(char *prog)
{
	fprintf(stderr, "Usage: %s [ -c <test-count> ] [ -v <verbose> ]\n",
		prog);
	fprintf(stderr, "\t[ -I <report-interval> ][ -C <cli_port_num> ]\n");
	fprintf(stderr, "\t[-R] [ -t <test> ] [ -B <byte-count> ]\n");
	fprintf(stderr, "\t[ -b <block-size> ] [-k] [ -P <prompt-color> ]\n");

	exit(1);
}

int main(int argc, char *argv[])
{
	static struct xdp2_cli_thread_info cli_thread_info;
	size_t byte_count = 0, block_size = 0;
	int cli_port_num = 0, sleep_time = 0;
	unsigned long num_tests = NUM_TESTS;
	const char *prompt_color = NULL;
	bool random_seed = false;
	bool kill_tests = false;
	char *run_test = NULL;
	unsigned int intv = 0;
	int c;

	while ((c = getopt(argc, argv, ARGS)) != -1) {
		switch (c) {
		case 'c':
			num_tests = strtoull(optarg, NULL, 10);
			break;
		case 'v':
			verbose = strtoul(optarg, NULL, 10);
			break;
		case 'I':
			intv = strtoul(optarg, NULL, 10);
			break;
		case 'C':
			cli_port_num = strtoul(optarg, NULL, 10);
			break;
		case 'R':
			random_seed = true;
			break;
		case 't':
			run_test = optarg;
			break;
		case 'B':
			byte_count = strtoul(optarg, NULL, 10);
			break;
		case 'b':
			block_size = strtoul(optarg, NULL, 10);
			break;
		case 'k':
			kill_tests = true;
			break;
		case 'P':
			prompt_color = xdp2_print_color_select_text(optarg);
			break;
		default:
			usage(argv[0]);
		}
	}

	if (random_seed)
		srand(time(NULL));

	if (cli_port_num) {
		XDP2_CLI_SET_THREAD_INFO(cli_thread_info, cli_port_num,
			(.label = "test_accel",
			 .prompt_color = prompt_color,
			));

		xdp2_cli_start(&cli_thread_info);
	}
	make_pvbuf();

	xdp2_init_pipelines();

	pipeline_counts = calloc(xdp2_section_array_size_all_my_piplines(),
		sizeof(struct pipeline_counts));

	run_all(num_tests, run_test, byte_count, block_size, kill_tests, intv);

	sleep(sleep_time);
}
