// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2020, 2021 Tom Herbert
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

#include <errno.h>

#include "xdp2/accelerator.h"

/* Common code to check for an error in a produced vaiable and set
 * error and error stage pointers
 */
#define PRODUCED_ERR(PRODUCED, STAGE) ({				\
	bool v = (PRODUCED) < 0;					\
									\
	if (v && (PRODUCED) != -EAGAIN && error && !*error) {		\
		*error = -(PRODUCED);					\
		if (error_stage)					\
			*error_stage = (STAGE);				\
	}								\
									\
	v;								\
})

static void adjust_output_pipe(struct xdp2_pipe *opipe, ssize_t produced)
{
	size_t open_right = __xdp2_pipe_empty_right(opipe);

	if (produced <= 0)
		return;

	XDP2_ASSERT(produced <= open_right, "Bad dp produced: %lu > %lu",
		    produced, open_right);

	opipe->prod_ptr += produced;

	if (opipe->prod_ptr == opipe->size) {
		/* We hit the limit of the input pipe array. Reset producer
		 * pointer
		 */
		opipe->prod_ptr = 0;
	}

	if (opipe->prod_ptr == opipe->cons_ptr) {
		/* We know we wrote at least one byte so the pipe must be
		 * full if the consumer and producer pointers are equal
		 */
		opipe->pipe_full = true;
	}

	__xdp2_pipe_check_pipe(opipe);

}

/* Output from a data pipe (or source) to a data pipe */
static ssize_t output_to_pipe_d_(const struct xdp2_pipeline_stage *stage,
				 __u8 *ibytes, size_t ibytes_len,
				 size_t *consumed, void *arg,
				 enum xdp2_pipeline_type_enum out_type)
{
	const struct xdp2_accelerator *accel = stage->accel;
	struct xdp2_pipe *opipe = stage->pipe;
	size_t open_right = __xdp2_pipe_empty_right(opipe);
	ssize_t produced = 0;

	switch (out_type) {
	case XDP2_PIPELINE_D:
		/* Call handler for data to data pipe */
		produced = accel->handler_dd(ibytes, ibytes_len,
					     &opipe->data[opipe->prod_ptr],
					     __xdp2_pipe_empty_right(opipe),
					     consumed, arg);
		break;
	case XDP2_PIPELINE_P:
		produced = accel->handler_dp(ibytes, ibytes_len,
					     &opipe->pkts[opipe->prod_ptr],
					     open_right, consumed, arg);
		break;
	case XDP2_PIPELINE_X:
		accel->handler_dx(ibytes, ibytes_len, consumed, arg);
		return 0;
	}

	XDP2_ASSERT(*consumed <= ibytes_len, "Bad dp consumed: %lu > %lu",
	    *consumed, ibytes_len);

	adjust_output_pipe(opipe, produced);

	return produced;
}

/* Output from a packet pipe (or source) to a data pipe */
static ssize_t output_to_pipe_p_(const struct xdp2_pipeline_stage *stage,
				 void **ipkts, unsigned int ipkts_cnt,
				 unsigned int *consumed, void *arg,
				 enum xdp2_pipeline_type_enum out_type)
{
	const struct xdp2_accelerator *accel = stage->accel;
	struct xdp2_pipe *opipe = stage->pipe;
	ssize_t produced = 0;

	switch (out_type) {
	case XDP2_PIPELINE_D:
		/* Call handler for packet to data pipe */
		produced = accel->handler_pd(ipkts, ipkts_cnt,
					     &opipe->data[opipe->prod_ptr],
					     __xdp2_pipe_empty_right(opipe),
					     consumed, arg);
		break;

	case XDP2_PIPELINE_P:
		/* Call handler for packet to packet pipe */
		produced = accel->handler_pp(ipkts, ipkts_cnt,
					     &opipe->pkts[opipe->prod_ptr],
					     __xdp2_pipe_empty_right(opipe),
					     consumed, arg);
		break;
	case XDP2_PIPELINE_X:
		/* Call handler for packet to null pipe */
		accel->handler_px(ipkts, ipkts_cnt, consumed, arg);
		return 0;
	}

	adjust_output_pipe(opipe, produced);

	return produced;
}

/* Read objects from downstream pipe and output to upstream pipe */
static ssize_t output_stage(const struct xdp2_pipeline_stage *stage,
			    const struct xdp2_pipeline_stage *prev_stage,
			    void *arg)
{
	struct xdp2_pipe *ipipe = prev_stage->pipe;
	unsigned int pkts_consumed;
	__u8 *ibytes_addr;
	void **ipkts_addr;
	ssize_t produced;
	size_t consumed;

	XDP2_SELECT_START(prev_stage->type, stage->type)
	XDP2_SELECT_CASE(XDP2_PIPELINE_D, XDP2_PIPELINE_D)
		ibytes_addr = &ipipe->data[ipipe->cons_ptr];
		produced = output_to_pipe_d_(stage, ibytes_addr,
					     __xdp2_pipe_filled_right(ipipe),
					     &consumed, arg,
					     XDP2_PIPELINE_D);
		break;
	XDP2_SELECT_CASE(XDP2_PIPELINE_D, XDP2_PIPELINE_P)
		ibytes_addr = &ipipe->data[ipipe->cons_ptr];
		produced = (size_t)output_to_pipe_d_(stage, ibytes_addr,
				__xdp2_pipe_filled_right(ipipe),
				&consumed, arg, XDP2_PIPELINE_P);
		break;
	XDP2_SELECT_CASE(XDP2_PIPELINE_P, XDP2_PIPELINE_D)
		ipkts_addr = &ipipe->pkts[ipipe->cons_ptr];
		produced = output_to_pipe_p_(stage, ipkts_addr,
				(unsigned int)__xdp2_pipe_filled_right(ipipe),
				&pkts_consumed, arg, XDP2_PIPELINE_D);
		consumed = (size_t)pkts_consumed;
		break;
	XDP2_SELECT_CASE(XDP2_PIPELINE_P, XDP2_PIPELINE_P)
		ipkts_addr = &ipipe->pkts[ipipe->cons_ptr];
		produced = (size_t)output_to_pipe_p_(stage, ipkts_addr,
				(unsigned int)__xdp2_pipe_filled_right(ipipe),
				&pkts_consumed, arg, XDP2_PIPELINE_P);
		consumed = (size_t)pkts_consumed;
		break;
	XDP2_SELECT_DEFAULT()
		XDP2_ERR(1, "Bad stage state in output stage");
	XDP2_SELECT_END();

	if (consumed) {
		/* Move the consumer pointer (note that the producer pointer
		 * was adjusted in the output_to_pipe_XX functions)
		 */
		ipipe->cons_ptr += consumed;
		ipipe->pipe_full = false;
		if (ipipe->cons_ptr == ipipe->size)
			ipipe->cons_ptr = 0;
	}

	return produced;
}

/* Run one intermediate stage */
static bool run_intermediate_stage(const struct xdp2_pipeline_stage *stage,
				   const struct xdp2_pipeline_stage *prev_stage,
				   unsigned int stage_num, unsigned int *error,
				   unsigned int *error_stage, void *arg,
				   bool done)
{
	struct xdp2_pipe *ipipe = prev_stage->pipe;
	ssize_t ret;
	bool ldone;

	if (__xdp2_pipe_empty(ipipe) && !done) {
		/* Don't call the handler with a zero input length until
		 * before the pipe is done. This way when the handler sees
		 * the zero length input it knows that's the end of the
		 * possible input
		 */
		return false;
	}

	ldone = (__xdp2_pipe_empty(ipipe) && done);

	ret = output_stage(stage, prev_stage, arg);

	PRODUCED_ERR(ret, stage_num);

	/* The stage is done when the input pipe is empty and the handler
	 * return zero
	 */
	return (ldone && !ret);
}

/* Run all the intermediate stages in a pipeline. start is the first stage to
 * run. The done argument indicates the pipeline is be completed
 */
static int run_all_intermedate(struct xdp2_pipeline *pline, int start,
			       unsigned int *error, unsigned int *error_stage,
			       void **args, bool done)
{
	struct xdp2_pipeline_stage *stage;
	int i;

	for (i = start; i < pline->num_stages - 1; i++) {
		stage = &pline->stages[i];
		done = run_intermediate_stage(stage, &pline->stages[i - 1],
					      i, error, error_stage, args[i],
					      done);
		if (done) {
			/* An intermediate stage is done, advance start point
			 * to the next stage
			 */
			start++;
		}
	}

	/* Return the first intermediate stage that's not done */
	return start;
}

/* Read objects from downstream pipe and output sink */
static ssize_t output_last_stage(const struct xdp2_pipeline_stage *stage,
				 const struct xdp2_pipeline_stage *prev_stage,
				 void *output, size_t out_len, void *arg)
{
	const struct xdp2_accelerator *accel = stage->accel;
	struct xdp2_pipe *ipipe = prev_stage->pipe;
	unsigned int pkts_consumed;
	size_t consumed = 0;
	__u8 *ibytes_addr;
	void **ipkts_addr;
	ssize_t produced;

	XDP2_SELECT_START(prev_stage->type, stage->type)
	XDP2_SELECT_CASE(XDP2_PIPELINE_D, XDP2_PIPELINE_D)
		ibytes_addr = &ipipe->data[ipipe->cons_ptr];
		produced = accel->handler_dd(ibytes_addr,
				__xdp2_pipe_filled_right(ipipe),
				(__u8 *)output, out_len, &consumed, arg);
		XDP2_ASSERT(produced <= out_len,
			    "Last DD handler over-produced : %lu > %lu\n",
			    produced, out_len);
		break;
	XDP2_SELECT_CASE(XDP2_PIPELINE_D, XDP2_PIPELINE_P)
		ibytes_addr = &ipipe->data[ipipe->cons_ptr];
		produced = accel->handler_dp((__u8 *)ibytes_addr,
				__xdp2_pipe_filled_right(ipipe),
				(void **)output, (unsigned int)out_len,
				&consumed, arg);
		XDP2_ASSERT(produced <= out_len,
			    "Last DP handler over-produced : %lu > %lu\n",
			    produced, out_len);
		break;
	XDP2_SELECT_CASE(XDP2_PIPELINE_D, XDP2_PIPELINE_X)
		ibytes_addr = &ipipe->data[ipipe->cons_ptr];
		accel->handler_dx((__u8 *)ibytes_addr,
				  __xdp2_pipe_filled_right(ipipe),
				  &consumed, arg);
		produced = 0;
		break;
	XDP2_SELECT_CASE(XDP2_PIPELINE_P, XDP2_PIPELINE_D)
		ipkts_addr = &ipipe->pkts[ipipe->cons_ptr];
		produced = accel->handler_pd(ipkts_addr,
				(unsigned int)__xdp2_pipe_filled_right(ipipe),
				(__u8 *)output, out_len,
				&pkts_consumed, arg);
		XDP2_ASSERT(produced <= out_len,
			    "Last PD handler over-produced : %lu > %lu\n",
			    produced, out_len);
		consumed = (size_t)pkts_consumed;

		break;
	XDP2_SELECT_CASE(XDP2_PIPELINE_P, XDP2_PIPELINE_P)
		ipkts_addr = &ipipe->pkts[ipipe->cons_ptr];
		produced = (size_t)accel->handler_pp(ipkts_addr,
				(unsigned int)__xdp2_pipe_filled_right(ipipe),
				(void **)output, (unsigned int)out_len,
				&pkts_consumed, arg);
		XDP2_ASSERT(produced <= out_len,
			    "Last PP handler over-produced : %lu > %lu\n",
			    produced, out_len);
		consumed = (size_t)pkts_consumed;
		break;
	XDP2_SELECT_CASE(XDP2_PIPELINE_P, XDP2_PIPELINE_X)
		ipkts_addr = &ipipe->pkts[ipipe->cons_ptr];
		accel->handler_px(ipkts_addr,
				  (unsigned int)__xdp2_pipe_filled_right(ipipe),
				  &pkts_consumed, arg);
		consumed = (size_t)pkts_consumed;
		produced = 0;
		break;
	XDP2_SELECT_DEFAULT()
		XDP2_ERR(1, "Bad stage state in output_last_stage");
	XDP2_SELECT_END();

	if (consumed) {
		/* Move the consumer pointer (note that the producer pointer
		 * was adjusted in the output_to_pipe_XX functions)
		 */

		ipipe->cons_ptr += consumed;
		ipipe->pipe_full = false;
		if (ipipe->cons_ptr == ipipe->size)
			ipipe->cons_ptr = 0;
	}

	__xdp2_pipe_check_pipe(ipipe);

	return produced;
}

/* Run the last stage of a pipeline */
static ssize_t run_last_stage(const struct xdp2_pipeline *pline, void *output,
			      size_t out_len, void *arg, bool *done)
{
	const struct xdp2_pipeline_stage *stage =
			&pline->stages[pline->num_stages - 1];
	const struct xdp2_pipeline_stage *prev_stage =
			&pline->stages[pline->num_stages - 2];
	struct xdp2_pipe *ipipe = prev_stage->pipe;
	ssize_t produced;
	bool ldone;

	if (__xdp2_pipe_empty(ipipe) && !done) {
		/* Don't call the handler with a zero input length until
		 * before the pipe is done. This way when the handler sees
		 * the zero length input it knows that's the end of the
		 * possible input
		 */
		return 0;
	}

	ldone = __xdp2_pipe_empty(ipipe) && done;

	produced = output_last_stage(stage, prev_stage, output, out_len, arg);

	/* The stage is done when the input pipe is empty and the handler
	 * return zero
	 */
	*done = (ldone && !produced);

	return produced;
}

static size_t run_stages_after_first(struct xdp2_pipeline *pline,
				     void *output, size_t output_size,
				     bool *done, unsigned int *error,
				     unsigned int *error_stage, void **args)
{
	const struct xdp2_pipeline_stage *last_stage =
				&pline->stages[pline->num_stages - 1];
	ssize_t produced = 0;
	int not_done;

	for (not_done = 1; not_done < pline->num_stages - 1;) {
		not_done = run_all_intermedate(pline, not_done, error,
					       error_stage, args, true);

		produced = run_last_stage(pline, output, output_size,
					  args[pline->num_stages - 1], done);

		if (produced >= 0) {
			if (last_stage->type == XDP2_PIPELINE_P)
				output += produced * sizeof(void *);
			else
				output += produced;
			output_size -= produced;
		} else {
			PRODUCED_ERR(produced, pline->num_stages);
		}
	}

	/* Intermediate stages are done, finish last stage */
	for (*done = false; !*done; *done = true) {
		produced = run_last_stage(pline, output, output_size,
					  args[pline->num_stages - 1], done);
		if (produced >= 0) {
			if (last_stage->type == XDP2_PIPELINE_P)
				output += produced * sizeof(void *);
			else
				output += produced;
			output_size -= produced;
		} else {
			PRODUCED_ERR(produced, pline->num_stages);
		}
	}

	return output_size;
}

/* Run pipeline with a flat data block of some size as input. The pipeline
 * output may be and another data block or a set of packets. The return
 * value is the number of objects (bytes or packets) written to output
 */
size_t __xdp2_run_pipeline_d(struct xdp2_pipeline *pline, void *input,
			     size_t input_size, void *output,
			     size_t output_size, unsigned int *error,
			     unsigned int *error_stage, void **args)
{
	const struct xdp2_pipeline_stage *last_stage =
				&pline->stages[pline->num_stages - 1];
	const struct xdp2_pipeline_stage *stage = &pline->stages[0];
	void *null_args[XDP2_PIPELINE_MAX_STAGES] = {};
	size_t consumed, orig_osize = output_size;
	ssize_t produced = 0;
	bool done = false;

	args = args ? : null_args;

	/* Run all the stages in the pipeline in sequence and in a loop
	 * until we've exhusated the input size (XXX First stage is done)
	 */
	while (1) {
		/* Run first stage based on whether the upstream pipe
		 * is data or packet
		 */
		produced = output_to_pipe_d_(&pline->stages[0], input,
					     input_size, &consumed,
					     args[0], stage->type);

		if (!input_size && !produced) {
			/* All the input is consumed and the handler reported
			 * it's done
			 */
			break;
		}

		/* We need to run the stage once when all the input is
		 * consumed to signal that the stage is done
		 */
		input_size -= consumed;
		input += consumed;

		PRODUCED_ERR(produced, 0);

		/* Run intermediate stages */
		run_all_intermedate(pline, 1, error, error_stage,
				    args, false);

		/* Run last stage */
		produced = run_last_stage(pline, output, output_size,
					  args[pline->num_stages - 1], &done);
		if (produced >= 0) {
			if (last_stage->type == XDP2_PIPELINE_P)
				output += produced * sizeof(void *);
			else
				output += produced;
			output_size -= produced;
		} else {
			PRODUCED_ERR(produced, pline->num_stages);
		}
	}

	output_size = run_stages_after_first(pline, output, output_size,
					     &done, error, error_stage, args);

	return orig_osize - output_size;
}

/* Run pipeline with packets as input. The pipeline output may be and
 * another data block or a set of packets. The return value is the number
 * of objects (bytes or packets) written to output
 */
size_t __xdp2_run_pipeline_p(struct xdp2_pipeline *pline, void **ipkts,
			     unsigned int ipkts_cnt, void *output,
			     size_t output_size, unsigned int *error,
			     unsigned int *error_stage, void **args)
{
	const struct xdp2_pipeline_stage *last_stage =
				&pline->stages[pline->num_stages - 1];
	const struct xdp2_pipeline_stage *stage = &pline->stages[0];
	void *null_args[XDP2_PIPELINE_MAX_STAGES] = {};
	size_t orig_osize = output_size;
	unsigned int consumed;
	bool done = false;
	ssize_t produced;

	args = args ? : null_args;

	*error = 0;

	/* Run all the stages in the pipeline in sequence and in a loop
	 * until we've exhausted the input pac kets (XXX First stage is done)
	 */
	while (1) {
		/* Run first stage based on whether the upstream pipe
		 * is data or packet
		 */
		produced = output_to_pipe_p_(&pline->stages[0], ipkts,
					     ipkts_cnt, &consumed,
					     args[0], stage->type);
		if (!ipkts_cnt && !produced) {
			/* All the input is consumed and the handler reported
			 * it's done
			 */
			break;
		}

		/* We need to run the stage once when all the input is
		 * consumed to signal that the stage is done
		 */
		ipkts_cnt -= consumed;
		ipkts += consumed;

		PRODUCED_ERR(produced, 0);

		run_all_intermedate(pline, 1, error, error_stage,
				    args, false);

		produced = run_last_stage(pline, output, output_size,
					  args[pline->num_stages - 1], &done);

		if (produced >= 0) {
			if (last_stage->type == XDP2_PIPELINE_P)
				output += produced * sizeof(void *);
			else
				output += produced;
			output_size -= produced;
		} else {
			PRODUCED_ERR(produced, pline->num_stages);
		}
	}

	output_size = run_stages_after_first(pline, output, output_size,
					     &done, error, error_stage, args);

	return orig_osize - output_size;
}

/* Initialize as since pipeline */
int xdp2_init_pipeline(struct xdp2_pipeline *pline)
{
	struct xdp2_pipeline_stage *stage;
	size_t dsize;
	int i;

	for (i = 0; i < pline->num_stages - 1; i++) {
		stage = &pline->stages[i];

		switch (stage->type) {
		case XDP2_PIPELINE_D:
			dsize = stage->pipe_size;
			break;
		case XDP2_PIPELINE_P:
			dsize = stage->pipe_size * sizeof(__u64);
			break;
		default:
			XDP2_ERR(1, "Bad stage state in init_pipeline");
			XDP2_ERR(1, "Bad size");
		}

		/* Malloc the pipe memory */
		stage->pipe = malloc(sizeof(struct xdp2_pipe) + dsize);
		stage->pipe->size = stage->pipe_size;
	}

	return 0;
}

struct xdp2_pipeline __dummy_pipeline XDP2_SECTION_ATTR(xdp2_pipeline_section);

/* Initialize the pipelines in the section array */
int xdp2_init_pipelines(void)
{
	struct xdp2_pipeline *pline;
	int i, num_els, ret;

	num_els = xdp2_section_array_size_xdp2_pipeline_section();
	pline = xdp2_section_base_xdp2_pipeline_section();

	for (i = 0; i < num_els; i++, pline++) {
		if (!pline->num_stages)
			continue;
		ret = xdp2_init_pipeline(pline);
		if (ret < 0)
			return ret;
	}

	return 0;
}
