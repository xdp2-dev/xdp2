/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD *
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

#ifndef __XDP2_UTILITY_H__
#define __XDP2_UTILITY_H__

/* Main API definitions for XDP2
 *
 * Definitions and functions for XDP2 library.
 */

#ifndef __KERNEL__
#include <arpa/inet.h>
#include <ctype.h>
#include <err.h>
#include <linux/types.h>
#include <linux/byteorder/little_endian.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#else
/* To get ARRAY_SIZE, container_of, etc. */
#include <linux/kernel.h>
#endif /* __KERNEL__ */

#include "cli/cli.h"

#include "xdp2/compiler_helpers.h"

/* Utilities that work in kernel or userspace */

/* Define the common ARRAY_SIZE macro if it's not already defined */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

/* Define the common container_of macro if it's not already defined */
#ifndef container_of
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member)*__mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member));	\
})

#endif

#define XDP2_CACHELINE_SIZE 64

#define XDP2_STRING_IT(X) #X

#define XDP2_DEPAIR2(...) __VA_ARGS__
#define XDP2_DEPAIR(X) XDP2_DEPAIR2 X

#define XDP2_JOIN2(A, B) A ## B
#define XDP2_JOIN3(A, B, C) A ## B ## C
#define XDP2_JOIN4(A, B, C, D) A ## B ## C ## D
#define XDP2_JOIN5(A, B, C, D, E) A ## B ## C ## D ## E
#define XDP2_JOIN6(A, B, C, D, E, F) A ## B ## C ## D ## E ## F
#define XDP2_JOIN7(A, B, C, D, E, F, G) A ## B ## C ## D ## E ## F ## G

/* Helper macros to extract positional arguments by number */

#define XDP2_GET_POS_ARG2_1(ARG1, ARG2) ARG1
#define XDP2_GET_POS_ARG2_2(ARG1, ARG2) ARG2

#define XDP2_GET_POS_ARG3_1(ARG1, ARG2, ARG3) ARG1
#define XDP2_GET_POS_ARG3_2(ARG1, ARG2, ARG3) ARG2
#define XDP2_GET_POS_ARG3_3(ARG1, ARG2, ARG3) ARG3

#define XDP2_GET_POS_ARG4_1(ARG1, ARG2, ARG3, ARG4) ARG1
#define XDP2_GET_POS_ARG4_2(ARG1, ARG2, ARG3, ARG4) ARG2
#define XDP2_GET_POS_ARG4_3(ARG1, ARG2, ARG3, ARG4) ARG3
#define XDP2_GET_POS_ARG4_4(ARG1, ARG2, ARG3, ARG4) ARG4

#define XDP2_GET_POS_ARG5_1(ARG1, ARG2, ARG3, ARG4, ARG5) ARG1
#define XDP2_GET_POS_ARG5_2(ARG1, ARG2, ARG3, ARG4, ARG5) ARG2
#define XDP2_GET_POS_ARG5_3(ARG1, ARG2, ARG3, ARG4, ARG5) ARG3
#define XDP2_GET_POS_ARG5_4(ARG1, ARG2, ARG3, ARG4, ARG5) ARG4
#define XDP2_GET_POS_ARG5_5(ARG1, ARG2, ARG3, ARG4, ARG5) ARG5

#define XDP2_GET_POS_ARG6_1(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) ARG1
#define XDP2_GET_POS_ARG6_2(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) ARG2
#define XDP2_GET_POS_ARG6_3(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) ARG3
#define XDP2_GET_POS_ARG6_4(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) ARG4
#define XDP2_GET_POS_ARG6_5(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) ARG5
#define XDP2_GET_POS_ARG6_6(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) ARG6

#define XDP2_GET_POS_ARG7_1(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) ARG1
#define XDP2_GET_POS_ARG7_2(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) ARG2
#define XDP2_GET_POS_ARG7_3(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) ARG3
#define XDP2_GET_POS_ARG7_4(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) ARG4
#define XDP2_GET_POS_ARG7_5(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) ARG5
#define XDP2_GET_POS_ARG7_6(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) ARG6
#define XDP2_GET_POS_ARG7_7(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) ARG7

static inline bool xdp2_is_power_of_two(unsigned long long x)
{
	return x && (!(x & (x - 1)));
}

static inline unsigned long long xdp2_round_pow_two(unsigned long long x)
{
	unsigned long long ret = 1;

	if (!x)
		return 1;

	x = (x - 1) * 2;

	while (x >>= 1)
		ret <<= 1;

	return ret;
}

static inline unsigned int xdp2_get_log(unsigned long long x)
{
	unsigned int ret = 0;

	while ((x >>= 1))
		ret++;

	return ret;
}

static inline unsigned int xdp2_get_log_round_up(unsigned long long x)
{
	unsigned long long orig = x;
	unsigned int ret = 0;

	while ((x >>= 1))
		ret++;

	if (orig % (1ULL << ret))
		ret++;

	return ret;
}

#define xdp2_max(a, b)						\
({								\
	__typeof__(a) _a = (a);					\
	__typeof__(b) _b = (b);					\
	_a > _b ? _a : _b;					\
})

#define xdp2_min(a, b)						\
({								\
	__typeof__(a) _a = (a);					\
	__typeof__(b) _b = (b);					\
	_a < _b ? _a : _b;					\
})

#if __BYTE_ORDER == __BIG_ENDIAN
#define XDP2_ENDIAN_BIG true
#define XDP2_ENDIAN_LITTLE false
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define XDP2_ENDIAN_BIG false
#define XDP2_ENDIAN_LITTLE true
#else
#error "Cannot determine endianness"
#endif

#ifndef htonll
#if __BYTE_ORDER == __BIG_ENDIAN
#define htonll(x) (x)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define htonll(x) (((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#else
#error "Cannot determine endianness"
#endif
#endif

#ifndef ntohll
#if __BYTE_ORDER == __BIG_ENDIAN
#define ntohll(x) (x)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define ntohll(x) (((unsigned long long)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#else
#error "Cannot determine endianness"
#endif
#endif

#define xdp2_ntohl24(x) (ntohl(x) >> 8)

#define XDP2_SWAP(a, b) do {					\
	typeof(a) __tmp = (a); (a) = (b); (b) = __tmp;		\
} while (0)

#define __XDP2_COMBINE1(X, Y, Z) X##Y##Z
#define __XDP2_COMBINE(X, Y, Z) __XDP2_COMBINE1(X, Y, Z)

#ifdef __COUNTER__
#define XDP2_UNIQUE_NAME(PREFIX, SUFFIX)				\
			__XDP2_COMBINE(PREFIX, __COUNTER__, SUFFIX)
#else
#define XDP2_UNIQUE_NAME(PREFIX, SUFFIX)				\
			__XDP2_COMBINE(PREFIX, __LINE__, SUFFIX)
#endif

/* Debug error and warning macros that call errx and warnx. If XDP2_NO_DEBUG
 * is set then macros are null definitons
 */

#ifdef XDP2_NO_DEBUG

#define XDP2_WARN(...)
#define XDP2_ERR(RET, ...)
#define XDP2_WARN_ONCE(...)
#define XDP2_ASSERT(...)

#elif defined(__KERNEL__)

#define XDP2_WARN(...)
#define XDP2_ERR(RET, ...)
#define XDP2_WARN_ONCE(...)
#define XDP2_ASSERT(...)

#elif defined(__KERNEL__)

#define XDP2_WARN(...)
#define XDP2_ERR(RET, ...)
#define XDP2_WARN_ONCE(...)

#else

#define XDP2_WARN(...) warnx(__VA_ARGS__)
#define XDP2_ERR(RET, ...) errx((RET), __VA_ARGS__)
#define XDP2_WARN_ONCE(...) do {					\
	static bool warned;						\
									\
	if (!warned) {							\
		XDP2_WARN(__VA_ARGS__);					\
		warned = true;						\
	}								\
} while (0)

#define XDP2_ASSERT(COND, ...) do {					\
	if (!(COND))							\
		XDP2_ERR(-1, __VA_ARGS__);				\
} while (0)

#endif

#define XDP2_BUILD_BUG_ON(condition)					\
	typedef char static_assertion_##__LINE__[(condition) ? 1 : -1]

#ifndef __KERNEL__

/* Userspace only defines */

#define XDP2_NSEC_PER_SEC 1000000000

static inline void
xdp2_timespec_add_nsec(struct timespec *r, const struct timespec *a, __u64 b)
{
	r->tv_sec = a->tv_sec + (b / XDP2_NSEC_PER_SEC);
	r->tv_nsec = a->tv_nsec + (b % XDP2_NSEC_PER_SEC);

	if (r->tv_nsec >= XDP2_NSEC_PER_SEC) {
		r->tv_sec++;
		r->tv_nsec -= XDP2_NSEC_PER_SEC;
	} else if (r->tv_nsec < 0) {
		r->tv_sec--;
		r->tv_nsec += XDP2_NSEC_PER_SEC;
	}
}

#define XDP2_NULL_TERM_COLOR "\033[0m"

#define __XDP2_PRINT_COLOR(STD, COLOR, ...) do {		\
	const char *_color = COLOR;				\
	char *_buffer;						\
								\
	if (asprintf(&_buffer, __VA_ARGS__) < 0) {		\
		fprintf(STD, "asprintf failed\n");		\
		break;						\
	}							\
	if (_color)						\
		fprintf(STD, "%s%s%s", _color, _buffer,		\
			  XDP2_NULL_TERM_COLOR);		\
	else							\
		fprintf(STD, "%s", _buffer);			\
} while (0)

#define XDP2_MAX_TERM_COLORS 6

#define XDP2_TERM_COLOR_RED	"\033[1;31m"
#define XDP2_TERM_COLOR_GREEN	"\033[1;32m"
#define XDP2_TERM_COLOR_YELLOW	"\033[1;33m"
#define XDP2_TERM_COLOR_BLUE	"\033[1;34m"
#define XDP2_TERM_COLOR_MAGENTA	"\033[1;35m"
#define XDP2_TERM_COLOR_CYAN	"\033[1;36m"

static inline const char *xdp2_print_color_select_text(const char *text)
{
	if (!strcmp(text, "red"))
		return XDP2_TERM_COLOR_RED;
	if (!strcmp(text, "green"))
		return XDP2_TERM_COLOR_GREEN;
	if (!strcmp(text, "yellow"))
		return XDP2_TERM_COLOR_YELLOW;
	if (!strcmp(text, "blue"))
		return XDP2_TERM_COLOR_BLUE;
	if (!strcmp(text, "magenta"))
		return XDP2_TERM_COLOR_MAGENTA;
	if (!strcmp(text, "cyan"))
		return XDP2_TERM_COLOR_CYAN;
	return "";
}

static inline const char *xdp2_print_color_select(int sel)
{
	if (sel < 0)
		return NULL;

	sel %= XDP2_MAX_TERM_COLORS;
	switch (sel) {
	case 0:
		return XDP2_TERM_COLOR_RED;
	case 1:
		return XDP2_TERM_COLOR_GREEN;
	case 2:
		return XDP2_TERM_COLOR_YELLOW;
	case 3:
		return XDP2_TERM_COLOR_BLUE;
	case 4:
		return XDP2_TERM_COLOR_MAGENTA;
	case 5:
		return XDP2_TERM_COLOR_CYAN;
	default:
		return NULL;
	}
}

#define __XDP2_CLI_PRINT(OUT, STD, COLOR, ...) do {		\
	const char *color = COLOR;					\
	char *buffer;						\
								\
	if (asprintf(&buffer, __VA_ARGS__) < 0) {		\
		cli_print(OUT, "asprintf failed\n");		\
		break;						\
	}							\
	if (OUT) {						\
		if (color)					\
			cli_print(OUT, "%s%s%s", color, buffer,	\
				  XDP2_NULL_TERM_COLOR);	\
		else						\
			cli_print(OUT, "%s", buffer);		\
	} else {						\
		if (color)					\
			fprintf(STD, "%s%s%s", color, buffer,	\
				  XDP2_NULL_TERM_COLOR);	\
		else						\
			fprintf(STD, "%s", buffer);		\
	}							\
} while (0)

#define XDP2_CLI_PRINT(OUT, ...)				\
	__XDP2_CLI_PRINT(OUT, stdout, NULL, __VA_ARGS__)

#define XDP2_CLI_PRINT_COLOR(OUT, COLOR, ...)			\
	__XDP2_CLI_PRINT(OUT, stdout, COLOR, __VA_ARGS__)

#define XDP2_CLI_PRINT_COLOR_SELECT(OUT, NUM, ...)		\
	__XDP2_CLI_PRINT(OUT, stdout,				\
		xdp2_print_color_select(NUM), __VA_ARGS__)

static inline char *xdp2_getline(void)
{
	char *line = (char *)malloc(100), *linep = line;
	size_t lenmax = 100, len = lenmax;
	int c;

	if (line == NULL)
		return NULL;

	for (;;) {
		char *linen;

		c = fgetc(stdin);
		if (c == '\r')
			continue;

		if (c == EOF)
			break;

		if (--len == 0) {
			ptrdiff_t diff = line - linep;

			len = lenmax;
			linen = (char *)realloc(linep, lenmax *= 2);

			if (linen == NULL) {
				free(linep);
				return NULL;
			}
			line = linen + diff;
			linep = linen;
		}

		if ((*line++ = c) == '\n')
			break;
	}
	*line = '\0';
	return linep;
}

#endif /* __KERNEL__ */

#define XDP2_PRINT_COLOR(COLOR, ...)				\
	__XDP2_PRINT_COLOR(stdout, COLOR, __VA_ARGS__)

#define XDP2_PRINT_COLOR_SEL(SEL, ...)				\
	XDP2_PRINT_COLOR(xdp2_print_color_select(SEL), __VA_ARGS__)

#define XDP2_ROUND_POW_TWO(v) (1 +					\
	(((((((((v) - 1) | (((v) - 1) >> 0x10) |			\
	      (((v) - 1) | (((v) - 1) >> 0x10) >> 0x08)) |		\
	     ((((v) - 1) | (((v) - 1) >> 0x10) |			\
	      (((v) - 1) | (((v) - 1) >> 0x10) >> 0x08)) >> 0x04))) |	\
	   ((((((v) - 1) | (((v) - 1) >> 0x10) |			\
	      (((v) - 1) | (((v) - 1) >> 0x10) >> 0x08)) |		\
	     ((((v) - 1) | (((v) - 1) >> 0x10) |			\
	      (((v) - 1) | (((v) - 1) >> 0x10) >> 0x08)) >>		\
						0x04))) >> 0x02))) |	\
	 ((((((((v) - 1) | (((v) - 1) >> 0x10) |			\
	      (((v) - 1) | (((v) - 1) >> 0x10) >> 0x08)) |		\
	     ((((v) - 1) | (((v) - 1) >> 0x10) |			\
	      (((v) - 1) | (((v) - 1) >> 0x10) >> 0x08)) >> 0x04))) |	\
	   ((((((v) - 1) | (((v) - 1) >> 0x10) |			\
	      (((v) - 1) | (((v) - 1) >> 0x10) >> 0x08)) |		\
	     ((((v) - 1) | (((v) - 1) >> 0x10) |			\
	      (((v) - 1) | (((v) - 1) >> 0x10) >> 0x08)) >>		\
					0x04))) >> 0x02))) >> 0x01))))

static inline unsigned long xdp2_round_up(unsigned long x, unsigned int r)
{
	unsigned int diff = x % r;

	return diff ? x + (r - diff) : x;
}

static inline unsigned long xdp2_round_up_div(unsigned long x, unsigned int r)
{
	return (x + (r - 1)) / r;
}

#define	__XDP2_LOG_1(n) (((n) >= 2ULL) ? 1 : 0)
#define	__XDP2_LOG_2(n) (((n) >= 1ULL << 2) ?				\
		(2 + __XDP2_LOG_1((n) >> 2)) :	__XDP2_LOG_1(n))
#define	__XDP2_LOG_4(n) (((n) >= 1ULL << 4) ?				\
		(4 + __XDP2_LOG_2((n) >> 4)) : __XDP2_LOG_2(n))
#define	__XDP2_LOG_8(n) (((n) >= 1ULL << 8) ?				\
		(8 + __XDP2_LOG_4((n) >> 8)) : __XDP2_LOG_4(n))
#define	__XDP2_LOG_16(n) (((n) >= 1ULL << 16) ?			\
		(16 + __XDP2_LOG_8((n) >> 16)) : __XDP2_LOG_8(n))

#define	XDP2_LOG_8BITS(n) (((n) >= 1ULL << 4) ?			\
		(4 + __XDP2_LOG_2((n) >> 4)) : __XDP2_LOG_2(n))

#define	XDP2_LOG_16BITS(n) (((n) >= 1ULL << 8) ?			\
		(8 + __XDP2_LOG_4((n) >> 8)) : __XDP2_LOG_4(n))

#define	XDP2_LOG_32BITS(n) (((n) >= 1ULL << 16) ?			\
		(16 + __XDP2_LOG_8((n) >> 16)) : __XDP2_LOG_8(n))

#define	XDP2_LOG_64BITS(n) (((n) >= 1ULL << 32) ?			\
		(32 + __XDP2_LOG_16((n) >> 32)) : __XDP2_LOG_16(n))

static inline bool xdp2_line_is_whitespace(const char *line)
{
	for (; *line != '\0'; line++)
		if (!isspace(*line))
			return false;

	return true;
}

#define XDP2_BITS_TO_WORDS64(VAL) ((((VAL) - 1) / 64) + 1)

#define XDP2_BITS_TO_BYTES(NUM) ((NUM) / 8)

#define __XDP2_MAKE_SEQNO_DIFF(NAME, NBITS, TYPE)			\
	static inline TYPE XDP2_JOIN3(xdp2_seqno, NAME, _diff)(		\
						TYPE x, TYPE y) {	\
		return x - y;						\
	}								\
									\
	static inline bool XDP2_JOIN3(xdp2_seqno, NAME, _lt)(		\
						TYPE x, TYPE y) {	\
		return XDP2_JOIN3(xdp2_seqno, NAME, _diff)(x, y) >	\
			1ULL << (NBITS - 1);				\
	}								\
									\
	static inline bool XDP2_JOIN3(xdp2_seqno, NAME, _gte)(		\
						TYPE x, TYPE y) {	\
		return !XDP2_JOIN3(xdp2_seqno, NAME, _lt)(x, y);	\
	}								\
									\
	static inline bool XDP2_JOIN3(xdp2_seqno, NAME, _gt)(		\
						TYPE x, TYPE y) {	\
		return XDP2_JOIN3(xdp2_seqno, NAME, _diff)(y, x) >	\
			1ULL << (NBITS - 1);				\
	}								\
									\
	static inline bool XDP2_JOIN3(xdp2_seqno, NAME, _lte)(		\
						TYPE x, TYPE y) {	\
		return !XDP2_JOIN3(xdp2_seqno, NAME, _gt)(x, y);	\
	}								\
									\
	static inline bool XDP2_JOIN3(xdp2_seqno, NAME, _in_wind)(	\
					TYPE v, TYPE base, TYPE last) {	\
		return XDP2_JOIN3(xdp2_seqno, NAME, _gte)(v, base) &&	\
		       XDP2_JOIN3(xdp2_seqno, NAME, _lt)(v, last);	\
	}

__XDP2_MAKE_SEQNO_DIFF(8, 8, __u8)
__XDP2_MAKE_SEQNO_DIFF(16, 16, __u16)
__XDP2_MAKE_SEQNO_DIFF(32, 32, __u32)
__XDP2_MAKE_SEQNO_DIFF(64, 64, __u64)
__XDP2_MAKE_SEQNO_DIFF(_ul, __BITS_PER_LONG, unsigned long)

#define XDP2_BIT(N) (1ULL << (N))

#endif /* __XDP2_UTILITY_H__ */
