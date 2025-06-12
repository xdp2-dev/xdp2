/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD *
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

#ifndef __XDP2_ACCELERATOR_H__
#define __XDP2_ACCELERATOR_H__

/* Definitions for XDP2 accelerators */

#include "xdp2/utility.h"
#include "xdp2/pmacro.h"
#include "xdp2/switch.h"

enum xdp2_pipeline_type_enum {
	XDP2_PIPELINE_D,
	XDP2_PIPELINE_P,
	XDP2_PIPELINE_X,
};

typedef void (*xdp2_pipeline_common_t)(void *input, size_t input_size,
				       void *output, size_t output_size,
				       unsigned int *error,
				       unsigned int *error_stage, void **args);

struct xdp2_accelerator {
	ssize_t (*handler_dd)(__u8 *ibytes, size_t ibytes_size,
			     __u8 *obytes, size_t obytes_size,
			     size_t *ibytes_consumed, void *arg);
	int (*handler_dp)(__u8 *ibytes, size_t bytes_size,
			  void **opkts, unsigned int opkts_cnt,
			  size_t *ibytes_consumed, void *arg);
	void (*handler_dx)(__u8 *ibytes, size_t bytes_size,
			   size_t *ibytes_consumed, void *arg);
	ssize_t (*handler_pd)(void **ipkts, unsigned int ipkts_cnt,
			      __u8 *obytes, size_t obytes_size,
			      unsigned int *ipkts_consumed, void *arg);
	int (*handler_pp)(void **ipkts, unsigned int ipkts_cnt,
			  void **opkts, unsigned int opkts_cnt,
			  unsigned int *ipkts_consumed, void *arg);
	void (*handler_px)(void **ipkts, unsigned int ipkts_cnt,
			   unsigned int *ipkts_consumed, void *arg);
	void *priv;
};

#define XDP2_PIPELINE_MAX_STAGES 10

#define XDP2_PIPE_DEFAULT_SIZE_D (1 << 11)
#define XDP2_PIPE_DEFAULT_SIZE_P 256
#define XDP2_PIPE_DEFAULT_SIZE_X 0

/* Pipe structure */
struct xdp2_pipe {
	unsigned int cons_ptr;
	unsigned int prod_ptr;
	bool pipe_full;
	unsigned int size;
	union {
		__u8 data[0];
		void *pkts[0];
	};
};


/* Check pipe for legal pointers */
static void __xdp2_pipe_check_pipe(struct xdp2_pipe *pipe)
{
	XDP2_ASSERT(pipe->cons_ptr < pipe->size,
		    "Consumer pointer out of bounds %u >= %u for pipe %p",
		    pipe->cons_ptr, pipe->size, pipe);
	XDP2_ASSERT(pipe->prod_ptr < pipe->size,
		    "Producer pointer out of bounds %u >= %u for pipe %p",
		    pipe->prod_ptr, pipe->size, pipe);
	XDP2_ASSERT(!pipe->pipe_full || pipe->cons_ptr == pipe->prod_ptr,
		    "Bad full condition for pipe %p\n", pipe);
}

/* Return amount of available space in the pipe to the right of the
 * producer pointer
 */
static inline size_t __xdp2_pipe_empty_right(struct xdp2_pipe *pipe)
{
	__xdp2_pipe_check_pipe(pipe);

	if (pipe->pipe_full)
		return 0;

	if (pipe->prod_ptr >= pipe->cons_ptr)
		return pipe->size - pipe->prod_ptr;

	return pipe->cons_ptr - pipe->prod_ptr;
}

/* Return number of object in pipe to the right of the consumer pointer */
static inline size_t __xdp2_pipe_filled_right(struct xdp2_pipe *pipe)
{
	__xdp2_pipe_check_pipe(pipe);

	if (pipe->pipe_full)
		return pipe->size - pipe->cons_ptr;

	if (pipe->prod_ptr >= pipe->cons_ptr)
		return pipe->prod_ptr - pipe->cons_ptr;

	return pipe->size - pipe->cons_ptr;
}

/* Check if pipe is empty */
static inline bool __xdp2_pipe_empty(struct xdp2_pipe *pipe)
{
	__xdp2_pipe_check_pipe(pipe);

	return (pipe->cons_ptr == pipe->prod_ptr) && !pipe->pipe_full;
}

/* Check if pipe is full */
static inline bool __xdp2_pipe_full(struct xdp2_pipe *pipe)
{
	__xdp2_pipe_check_pipe(pipe);

	return pipe->pipe_full;
}

/* Return number of objects in a pipe */
static inline size_t __xdp2_pipe_occupancy(struct xdp2_pipe *pipe)
{
	__xdp2_pipe_check_pipe(pipe);

	if (pipe->pipe_full)
		return pipe->size;

	return (pipe->prod_ptr - pipe->cons_ptr + pipe->size) % pipe->size;
}

/* Return number of available objects in a pipe */
static inline size_t __xdp2_pipe_avail(struct xdp2_pipe *pipe)
{
	return pipe->size - __xdp2_pipe_occupancy(pipe);
}

/* Pipeline stage structure */
struct xdp2_pipeline_stage {
	enum xdp2_pipeline_type_enum type;
	const struct xdp2_accelerator *accel;
	struct xdp2_pipe *pipe;
	size_t pipe_size;
};

/* Pipeline structure */
struct xdp2_pipeline {
	unsigned int num_stages;
	struct xdp2_pipeline_stage stages[XDP2_PIPELINE_MAX_STAGES];
} XDP2_ALIGN_SECTION;

XDP2_DEFINE_SECTION(xdp2_pipeline_section, struct xdp2_pipeline);

/* Prototypes for frontend functions to run a pipeline. Function names have
 * format __xdp2_run_pipeline_XY where X describes the input to the pipeline
 * as a block of data (X=d) or a list of packets (X=p). Y describes the output
 * of the pipeline as a block of data (Y=d) or a list of packets (Y=p)
 */

size_t __xdp2_run_pipeline_d(struct xdp2_pipeline *pline, void *input,
			     size_t input_size, void *output,
			     size_t output_size, unsigned int *error,
			     unsigned int *error_stage, void **args);

size_t __xdp2_run_pipeline_p(struct xdp2_pipeline *pline, void **ipkts,
			     unsigned int ipkts_cnt, void *output,
			     size_t output_size, unsigned int *error,
			     unsigned int *error_stage, void **args);

/* Frontend function to run a pipeline with data input and data output */
static inline size_t __xdp2_run_pipeline_dd(struct xdp2_pipeline *pline,
					    __u8 *ibytes, size_t ibytes_len,
					    __u8 *obytes, size_t obytes_size,
					    unsigned int *error,
					    unsigned int *error_stage,
					    void **args)
{
	return __xdp2_run_pipeline_d(pline, (void *)ibytes, ibytes_len,
				     (void *)obytes, obytes_size, error,
				     error_stage, args);
}

/* Frontend function to run a pipeline with data input and packet output */
static inline unsigned int __xdp2_run_pipeline_dp(struct xdp2_pipeline *pline,
						  __u8 *ibytes,
						  size_t ibytes_len,
						  void **opkts,
						  unsigned int opkts_cnt,
						  unsigned int *error,
						  unsigned int *error_stage,
						  void **args)
{
	return (unsigned int)__xdp2_run_pipeline_d(pline, (void *)ibytes,
						   ibytes_len, (void *)opkts,
						   (size_t)opkts_cnt, error,
						   error_stage, args);
}

/* Frontend function to run a pipeline with data input and no output */
static inline void __xdp2_run_pipeline_dx(struct xdp2_pipeline *pline,
					  __u8 *ibytes, size_t ibytes_len,
					   unsigned int *error,
					   unsigned int *error_stage,
					   void **args)
{
	(void)__xdp2_run_pipeline_d(pline, (void *)ibytes, ibytes_len, NULL, 0,
				    error, error_stage, args);
}

/* Frontend function to run a pipeline with packet input and data output */
static inline size_t __xdp2_run_pipeline_pd(struct xdp2_pipeline *pline,
					    void **ipkts,
					    unsigned int ipkts_cnt,
					    __u8 *obytes, size_t obytes_size,
					    unsigned int *error,
					    unsigned int *error_stage,
					    void **args)
{
	return __xdp2_run_pipeline_p(pline, (void *)ipkts, (size_t)ipkts_cnt,
				     (void *)obytes, obytes_size, error,
				     error_stage, args);
}

/* Frontend function to run a pipeline with packet input and packet output */
static inline unsigned int __xdp2_run_pipeline_pp(struct xdp2_pipeline *pline,
						  void **ipkts,
						  unsigned int ipkts_cnt,
						  void **opkts,
						  unsigned int opkts_cnt,
						  unsigned int *error,
						  unsigned int *error_stage,
						  void **args)
{
	return (unsigned int)__xdp2_run_pipeline_p(pline, (void *)ipkts,
						   (size_t)ipkts_cnt,
						   (void *)opkts,
						   (size_t)opkts_cnt, error,
						   error_stage, args);
}

/* Frontend function to run a pipeline with packet input and no output */
static inline void __xdp2_run_pipeline_px(struct xdp2_pipeline *pline,
					  void **ipkts, unsigned int ipkts_cnt,
					  unsigned int *error,
					  unsigned int *error_stage,
					  void **args)
{
	(void)__xdp2_run_pipeline_p(pline, (void *)ipkts, (size_t)ipkts_cnt,
				    NULL, 0, error, error_stage, args);
}

int xdp2_init_pipeline(struct xdp2_pipeline *pline);

int xdp2_init_pipelines(void);

/* Macros to make frontend pipeline functions */

#define __XDP2_PIPELINE_MAKE_FUNC_DD(NAME)				\
static inline size_t NAME(__u8 *ibytes, size_t ibytes_size,		\
			  __u8 *obytes, size_t obytes_size,		\
			  unsigned int *error,				\
			  unsigned int *error_stage, void **args)	\
{									\
	return __xdp2_run_pipeline_dd(&XDP2_JOIN2(NAME, _struct),	\
				      ibytes, ibytes_size, obytes,	\
				      obytes_size, error, error_stage,	\
				      args);				\
}

#define __XDP2_PIPELINE_MAKE_FUNC_DP(NAME)				\
static inline unsigned int NAME(__u8 *ibytes, size_t ibytes_size,	\
				void **opkts, size_t opkts_cnt,		\
				unsigned int *error,			\
				unsigned int *error_stage, void **args)	\
{									\
	return __xdp2_run_pipeline_dp(&XDP2_JOIN2(NAME, _struct),	\
				      ibytes, ibytes_size, opkts,	\
				      opkts_cnt, error, error_stage,	\
				      args);				\
}

#define __XDP2_PIPELINE_MAKE_FUNC_DX(NAME)				\
static inline void NAME(__u8 *ibytes, size_t ibytes_size,		\
			void *null_out, unsigned int null_out_len,	\
		       unsigned int *error,				\
		       unsigned int *error_stage, void **args)		\
{									\
	__xdp2_run_pipeline_dx(&XDP2_JOIN2(NAME, _struct), ibytes,	\
			       ibytes_size, error, error_stage, args);	\
}

#define __XDP2_PIPELINE_MAKE_FUNC_PD(NAME)				\
static inline size_t NAME(void **ipkts, unsigned int ipkts_cnt,		\
			  __u8 *obytes, size_t obytes_size,		\
			  unsigned int *error,				\
			  unsigned int *error_stage, void **args)	\
{									\
	return __xdp2_run_pipeline_pd(&XDP2_JOIN2(NAME, _struct),	\
				      ipkts, ipkts_cnt, obytes,		\
				      obytes_size, error, error_stage,	\
				      args);				\
}

#define __XDP2_PIPELINE_MAKE_FUNC_PP(NAME)				\
static inline unsigned int NAME(void **ipkts, unsigned int ipkts_cnt,	\
				void **opkts, unsigned int opkts_cnt,	\
				unsigned int *error,			\
				unsigned int *error_stage, void **args)	\
{									\
	return __xdp2_run_pipeline_pp(&XDP2_JOIN2(NAME, _struct),	\
				      ipkts, ipkts_cnt, opkts,		\
				      opkts_cnt, error, error_stage,	\
				      args);				\
}

#define __XDP2_PIPELINE_MAKE_FUNC_PX(NAME)				\
static inline void NAME(void **ipkts, unsigned int ipkts_cnt,		\
			void *null_out, unsigned int null_out_len,	\
			unsigned int *error, unsigned int *error_stage,	\
			void **args)					\
{									\
	__xdp2_run_pipeline_px(&XDP2_JOIN2(NAME, _struct), ipkts,	\
			       ipkts_cnt, error, error_stage, args);	\
}

/* Macros to make pipeline structures. There are two variants:
 * XDP2_PIPELINE and XDP2_PIPELINE_SZ
 *
 * XDP2_PIPELINE specifies a pipeline with format:
 *
 *	XDP2_PIPELINE(P|D, <accel>, P|D, <accel>, P|D, ..., <accel>, P|D)
 *
 * For example: XDP2_PIPELINE(test2, P, pl_add_2, D, pl_add_1, D)
 *
 * XDP2_PIPELINE_SZ allows the pipe size to be set for each stage. If has
 * format:
 *
 *	XDP2_PIPELINE_SZ(P|D, <accel>, (P|D, <size>), <accel>, (P|D, <size>),
 *			 <accel>, ..., <accel>, (P|D, <size>), <accel>, P|D)
 *
 * For example:
 *
 *	XDP2_PIPELINE_SZ(test2sz, D, pl_add_2, (D, 8888), pl_add_1, P)
 */

#define __XDP2_PIPELINE_PREAMBLE(NAME, NUM_STAGES)			\
static struct xdp2_pipeline XDP2_JOIN2(NAME, _struct)			\
	XDP2_SECTION_ATTR(xdp2_pipeline_section) __unused() = {		\
	.num_stages = NUM_STAGES,

#define __XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE) };		\
	__XDP2_PIPELINE_MAKE_FUNC_##HTYPE##TTYPE(NAME)

#define __XDP2_PIPELINE_STAGE1(NUM, ACCEL, TYPE, SIZE)			\
	.stages[NUM].accel = &ACCEL,					\
	.stages[NUM].type = XDP2_JOIN2(XDP2_PIPELINE_, TYPE),		\
	.stages[NUM].pipe_size = SIZE,

#define __XDP2_PIPELINE_STAGE(NUM, ACCEL, TYPE)				\
	__XDP2_PIPELINE_STAGE1(NUM, ACCEL, TYPE,			\
			       XDP2_JOIN2(XDP2_PIPE_DEFAULT_SIZE_, TYPE))

#define __XDP2_PIPELINE_STAGE_TPAIR(NUM, ACCEL, TYPE)			\
	__XDP2_PIPELINE_STAGE1(NUM, ACCEL,				\
			       XDP2_GET_POS_ARG2_1 TYPE,		\
			       XDP2_GET_POS_ARG2_2 TYPE)

#define __XDP2_PIPELINE_6(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2, TTYPE)	\
	__XDP2_PIPELINE_PREAMBLE(NAME, 2)				\
	__XDP2_PIPELINE_STAGE(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE(1, ACCEL_2, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_8(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
			  TYPE_2, ACCEL_3, TTYPE)			\
	__XDP2_PIPELINE_PREAMBLE(NAME, 3)				\
	__XDP2_PIPELINE_STAGE(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE(2, ACCEL_3, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_10(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
			   TYPE_2, ACCEL_3, TYPE_3, ACCEL_4, TTYPE)	\
	__XDP2_PIPELINE_PREAMBLE(NAME, 4)				\
	__XDP2_PIPELINE_STAGE(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE(3, ACCEL_4, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_12(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
			   TYPE_2, ACCEL_3, TYPE_3, ACCEL_4, TYPE_4,	\
			   ACCEL_5, TTYPE)				\
	__XDP2_PIPELINE_PREAMBLE(NAME, 5)				\
	__XDP2_PIPELINE_STAGE(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE(3, ACCEL_4, TYPE_4)			\
	__XDP2_PIPELINE_STAGE(4, ACCEL_5, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_14(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
			   TYPE_2, ACCEL_3, TYPE_3, ACCEL_4, TYPE_4,	\
			   ACCEL_5, TYPE_5, ACCEL_6, TTYPE)		\
	__XDP2_PIPELINE_PREAMBLE(NAME, 6)				\
	__XDP2_PIPELINE_STAGE(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE(3, ACCEL_4, TYPE_4)			\
	__XDP2_PIPELINE_STAGE(4, ACCEL_5, TYPE_5)			\
	__XDP2_PIPELINE_STAGE(5, ACCEL_6, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_16(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
			   TYPE_2, ACCEL_3, TYPE_3, ACCEL_4, TYPE_4,	\
			   ACCEL_5, TYPE_5, ACCEL_6, TYPE_6, ACCEL_7,	\
			   TTYPE)					\
	__XDP2_PIPELINE_PREAMBLE(NAME, 7)				\
	__XDP2_PIPELINE_STAGE(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE(3, ACCEL_4, TYPE_4)			\
	__XDP2_PIPELINE_STAGE(4, ACCEL_5, TYPE_5)			\
	__XDP2_PIPELINE_STAGE(5, ACCEL_6, TYPE_6)			\
	__XDP2_PIPELINE_STAGE(6, ACCEL_7, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_18(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
			   TYPE_2, ACCEL_3, TYPE_3, ACCEL_4, TYPE_4,	\
			   ACCEL_5, TYPE_5, ACCEL_6, TYPE_6, ACCEL_7,	\
			   TYPE_7, ACCEL_8, TTYPE)			\
	__XDP2_PIPELINE_PREAMBLE(NAME, 8)				\
	__XDP2_PIPELINE_STAGE(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE(3, ACCEL_4, TYPE_4)			\
	__XDP2_PIPELINE_STAGE(4, ACCEL_5, TYPE_5)			\
	__XDP2_PIPELINE_STAGE(5, ACCEL_6, TYPE_6)			\
	__XDP2_PIPELINE_STAGE(6, ACCEL_7, TYPE_7)			\
	__XDP2_PIPELINE_STAGE(7, ACCEL_8, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_20(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
			   TYPE_2, ACCEL_3, TYPE_3, ACCEL_4, TYPE_4,	\
			   ACCEL_5, TYPE_5, ACCEL_6, TYPE_6, ACCEL_7,	\
			   TYPE_7, ACCEL_8, TYPE_8, ACCEL_9, TTYPE)	\
	__XDP2_PIPELINE_PREAMBLE(NAME, 9)				\
	__XDP2_PIPELINE_STAGE(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE(3, ACCEL_4, TYPE_4)			\
	__XDP2_PIPELINE_STAGE(4, ACCEL_5, TYPE_5)			\
	__XDP2_PIPELINE_STAGE(5, ACCEL_6, TYPE_6)			\
	__XDP2_PIPELINE_STAGE(6, ACCEL_7, TYPE_7)			\
	__XDP2_PIPELINE_STAGE(7, ACCEL_8, TYPE_8)			\
	__XDP2_PIPELINE_STAGE(8, ACCEL_9, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_22(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
			   TYPE_2, ACCEL_3, TYPE_3, ACCEL_4, TYPE_4,	\
			   ACCEL_5, TYPE_5, ACCEL_6, TYPE_6, ACCEL_7,	\
			   TYPE_7, ACCEL_8, TYPE_8, ACCEL_9, TYPE_9,	\
			   ACCEL_10, TTYPE)				\
	__XDP2_PIPELINE_PREAMBLE(NAME, 10)				\
	__XDP2_PIPELINE_STAGE(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE(3, ACCEL_4, TYPE_4)			\
	__XDP2_PIPELINE_STAGE(4, ACCEL_5, TYPE_5)			\
	__XDP2_PIPELINE_STAGE(5, ACCEL_6, TYPE_6)			\
	__XDP2_PIPELINE_STAGE(6, ACCEL_7, TYPE_7)			\
	__XDP2_PIPELINE_STAGE(7, ACCEL_8, TYPE_8)			\
	__XDP2_PIPELINE_STAGE(8, ACCEL_9, TYPE_9)			\
	__XDP2_PIPELINE_STAGE(9, ACCEL_10, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_TPAIR_6(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
				TTYPE)					\
	__XDP2_PIPELINE_PREAMBLE(NAME, 2)				\
	__XDP2_PIPELINE_STAGE_TPAIR(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE(1, ACCEL_2, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_TPAIR_8(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
				TYPE_2,	ACCEL_3, TTYPE)			\
	__XDP2_PIPELINE_PREAMBLE(NAME, 3)				\
	__XDP2_PIPELINE_STAGE_TPAIR(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE_TPAIR(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE(2, ACCEL_3, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_TPAIR_10(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
				 TYPE_2, ACCEL_3, TYPE_3, ACCEL_4,	\
				 TTYPE)					\
	__XDP2_PIPELINE_PREAMBLE(NAME, 4)				\
	__XDP2_PIPELINE_STAGE_TPAIR(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE_TPAIR(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE_TPAIR(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE(3, ACCEL_4, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_TPAIR_12(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
				 TYPE_2, ACCEL_3, TYPE_3, ACCEL_4,	\
				 TYPE_4, ACCEL_5, TTYPE)		\
	__XDP2_PIPELINE_PREAMBLE(NAME, 5)				\
	__XDP2_PIPELINE_STAGE_TPAIR(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE_TPAIR(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE_TPAIR(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE_TPAIR(3, ACCEL_4, TYPE_4)			\
	__XDP2_PIPELINE_STAGE(4, ACCEL_5, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_TPAIR_14(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
				 TYPE_2, ACCEL_3, TYPE_3, ACCEL_4,	\
				 TYPE_4, ACCEL_5, TYPE_5, ACCEL_6,	\
				 TTYPE)					\
	__XDP2_PIPELINE_PREAMBLE(NAME, 6)				\
	__XDP2_PIPELINE_STAGE_TPAIR(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE_TPAIR(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE_TPAIR(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE_TPAIR(3, ACCEL_4, TYPE_4)			\
	__XDP2_PIPELINE_STAGE_TPAIR(4, ACCEL_5, TYPE_5)			\
	__XDP2_PIPELINE_STAGE(5, ACCEL_6, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_TPAIR_16(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
				 TYPE_2, ACCEL_3, TYPE_3, ACCEL_4,	\
				 TYPE_4, ACCEL_5, TYPE_5, ACCEL_6,	\
				 TYPE_6, ACCEL_7, TTYPE)		\
	__XDP2_PIPELINE_PREAMBLE(NAME, 7)				\
	__XDP2_PIPELINE_STAGE_TPAIR(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE_TPAIR(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE_TPAIR(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE_TPAIR(3, ACCEL_4, TYPE_4)			\
	__XDP2_PIPELINE_STAGE_TPAIR(4, ACCEL_5, TYPE_5)			\
	__XDP2_PIPELINE_STAGE_TPAIR(5, ACCEL_6, TYPE_6)			\
	__XDP2_PIPELINE_STAGE(6, ACCEL_7, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_TPAIR_18(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
				 TYPE_2, ACCEL_3, TYPE_3, ACCEL_4,	\
				 TYPE_4, ACCEL_5, TYPE_5, ACCEL_6,	\
				 TYPE_6, ACCEL_7, TYPE_7, ACCEL_8,	\
				 TTYPE)					\
	__XDP2_PIPELINE_PREAMBLE(NAME, 8)				\
	__XDP2_PIPELINE_STAGE_TPAIR(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE_TPAIR(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE_TPAIR(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE_TPAIR(3, ACCEL_4, TYPE_4)			\
	__XDP2_PIPELINE_STAGE_TPAIR(4, ACCEL_5, TYPE_5)			\
	__XDP2_PIPELINE_STAGE_TPAIR(5, ACCEL_6, TYPE_6)			\
	__XDP2_PIPELINE_STAGE_TPAIR(6, ACCEL_7, TYPE_7)			\
	__XDP2_PIPELINE_STAGE(7, ACCEL_8, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_TPAIR_20(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
				 TYPE_2, ACCEL_3, TYPE_3, ACCEL_4,	\
				 TYPE_4, ACCEL_5, TYPE_5, ACCEL_6,	\
				 TYPE_6, ACCEL_7, TYPE_7, ACCEL_8,	\
				 TYPE_8, ACCEL_9, TTYPE)		\
	__XDP2_PIPELINE_PREAMBLE(NAME, 9)				\
	__XDP2_PIPELINE_STAGE_TPAIR(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE_TPAIR(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE_TPAIR(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE_TPAIR(3, ACCEL_4, TYPE_4)			\
	__XDP2_PIPELINE_STAGE_TPAIR(4, ACCEL_5, TYPE_5)			\
	__XDP2_PIPELINE_STAGE_TPAIR(5, ACCEL_6, TYPE_6)			\
	__XDP2_PIPELINE_STAGE_TPAIR(6, ACCEL_7, TYPE_7)			\
	__XDP2_PIPELINE_STAGE_TPAIR(7, ACCEL_8, TYPE_8)			\
	__XDP2_PIPELINE_STAGE(8, ACCEL_9, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE_TPAIR_22(NAME, HTYPE, ACCEL_1, TYPE_1, ACCEL_2,	\
				 TYPE_2, ACCEL_3, TYPE_3, ACCEL_4,	\
				 TYPE_4, ACCEL_5, TYPE_5, ACCEL_6,	\
				 TYPE_6, ACCEL_7, TYPE_7, ACCEL_8,	\
				 TYPE_8, ACCEL_9, TYPE_9, ACCEL_10,	\
				 TTYPE)					\
	__XDP2_PIPELINE_PREAMBLE(NAME, 10)				\
	__XDP2_PIPELINE_STAGE_TPAIR(0, ACCEL_1, TYPE_1)			\
	__XDP2_PIPELINE_STAGE_TPAIR(1, ACCEL_2, TYPE_2)			\
	__XDP2_PIPELINE_STAGE_TPAIR(2, ACCEL_3, TYPE_3)			\
	__XDP2_PIPELINE_STAGE_TPAIR(3, ACCEL_4, TYPE_4)			\
	__XDP2_PIPELINE_STAGE_TPAIR(4, ACCEL_5, TYPE_5)			\
	__XDP2_PIPELINE_STAGE_TPAIR(5, ACCEL_6, TYPE_6)			\
	__XDP2_PIPELINE_STAGE_TPAIR(6, ACCEL_7, TYPE_7)			\
	__XDP2_PIPELINE_STAGE_TPAIR(7, ACCEL_8, TYPE_8)			\
	__XDP2_PIPELINE_STAGE_TPAIR(8, ACCEL_9, TYPE_9)			\
	__XDP2_PIPELINE_STAGE(9, ACCEL_10, TTYPE)			\
	__XDP2_PIPELINE_POSTAMBLE(NAME, HTYPE, TTYPE)

#define __XDP2_PIPELINE2(NUM, ...)					\
	__XDP2_PIPELINE_##NUM(__VA_ARGS__)

#define __XDP2_PIPELINE(NUM, ...)					\
	__XDP2_PIPELINE2(NUM, __VA_ARGS__)

#define XDP2_PIPELINE(...)						\
	__XDP2_PIPELINE(XDP2_PMACRO_NARGS(__VA_ARGS__), __VA_ARGS__)

#define __XDP2_PIPELINE2_TPAIR(NUM, ...)				\
	__XDP2_PIPELINE_TPAIR_##NUM(__VA_ARGS__)

#define __XDP2_PIPELINE_TPAIR(NUM, ...)					\
	__XDP2_PIPELINE2_TPAIR(NUM, __VA_ARGS__)

#define XDP2_PIPELINE_SZ(...)						\
	__XDP2_PIPELINE_TPAIR(XDP2_PMACRO_NARGS(__VA_ARGS__), __VA_ARGS__)

#endif /* __XDP2_ACCELERATOR_H__ */
