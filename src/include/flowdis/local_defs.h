/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __FLOWDIS_LOCAL_DEFS_H__
#define __FLOWDIS_LOCAL_DEFS_H__

/* Glue, compiler stubs, miscellaneous definitions not included in other
 * include files that are needed to compile kernel code in userspace.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/ipv6.h>

#include "flowdis/build_bug.h"

/* Typedefs for __u* */
typedef __u8 u8;
typedef __u16 u16;
typedef __u32 u32;
typedef __u64 u64;

/* Kernel BUG_ON macro */
#define BUG_ON(X) assert(!(X))

/* Kernel configuration build macro, just stub to false */
#define IS_ENABLED(X) 0

/* Check alignment */
#define IS_ALIGNED(x, a)	(((x) & ((typeof(x))(a) - 1)) == 0)

/* Kernel compiler attributes */

#define __aligned(x)		__attribute__((__aligned__(x)))
#define __packed __attribute__((__packed__))

/* Field and offset macros */

#define offsetofend(TYPE, MEMBER) \
	(offsetof(TYPE, MEMBER) + sizeof_field(TYPE, MEMBER))

#define sizeof_field(TYPE, MEMBER) sizeof((((TYPE *)0)->MEMBER))

/* BIT macro */
#define BIT(nr)		((unsigned long)(1) << (nr))

/* Min and max */

#define min_t(type, x, y) ({			\
	type __min1 = (x);			\
	type __min2 = (y);			\
	__min1 < __min2 ? __min1: __min2; })

#define max_t(type, x, y) ({			\
	type __max1 = (x);			\
	type __max2 = (y);			\
	__max1 > __max2 ? __max1: __max2; })

/* Get random number functions */

static inline void get_random_bytes(void *buf, int nbytes)
{
	__u8 *bytes = buf;
	int i;

	for (i = 0; i < nbytes; i++)
		bytes[i] = rand();
}

static inline void net_get_random_once(void *buf, int nbytes)
{
	static bool done;

	if (done)
		return;

	done = true;
	get_random_bytes(buf, nbytes);
}

/**
 * swap - swap values of @a and @b
 * @a: first value
 * @b: second value
 */
#define swap(a, b) \
	do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)


/* Array definitions */

/* &a[0] degrades to a pointer: a different type from an array */
#define __must_be_array(a)      BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))

/**
 * ARRAY_SIZE - get the number of elements in array @arr
 * @arr: array to be sized
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

/* Stub out definitions */
#define unlikely(X) X
#define likely(X) X
#define __force
#define __read_mostly
#define EXPORT_SYMBOL(X)
#define EXPORT_SYMBOL_GPL(X)
#define __init
#define core_initcall(X)

#endif /*  __FLOWDIS_LOCAL_DEFS_H__ */
