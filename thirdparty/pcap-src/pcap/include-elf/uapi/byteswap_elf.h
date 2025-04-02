/* byteswap.h

This file is part of Cygwin.

This software is a copyrighted work licensed under the terms of the
Cygwin license.  Please consult the file "CYGWIN_LICENSE" for
details. */

#ifndef __XDP2_BYTESWAP_H__
#define __XDP2_BYTESWAP_H__

#include <bits/byteswap_elf.h>

#define bswap_16(x) __bswap_16(x)
#define bswap_32(x) __bswap_32(x)
#define bswap_64(x) __bswap_64(x)

#endif /* _BYTESWAP_H */
