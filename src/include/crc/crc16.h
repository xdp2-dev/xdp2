#ifndef CRC16_H
#define CRC16_H

#include <linux/types.h>

/* crc16.h */

/* Return the CRC-16 of buf[0..len-1] */
__u16 crc16(const char *buf, int len);

#endif    /* CRC16_H */
