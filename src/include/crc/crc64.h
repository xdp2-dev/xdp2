#ifndef CRC64_H
#define CRC64_H

#include <linux/types.h>
#include <stdint.h>

void crc64_init(void);
__u64 crc64(__u64 crc, const void *s, size_t l);
__u64 crc64_dsa(__u64 crc, const void *s, size_t l);

__u64 crc16(const void *s, size_t l);

#ifdef REDIS_TEST
int crc64Test(int argc, char *argv[], int flags);
#endif

#endif
