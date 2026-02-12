/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2026 XDPnet Inc.
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

#ifndef __XDP2_SHM_H__
#define __XDP2_SHM_H__

/* Definitions and functions for shared memory functions in the XDP2 library */

#include <fcntl.h>
#include <linux/types.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int xdp2_shm_open_file(const char *name, size_t size, off_t offset,
		       bool create, size_t *fsize);
void *xdp2_shm_map(const char *file, size_t size, off_t offset, bool create,
		   bool zero_it);

void *xdp2_shm_map_make(const char *file, size_t size, off_t offset,
			bool set_xlat, size_t shm_size, size_t conf_size,
			char *shm_name, unsigned int shm_type,
			unsigned int shm_addr_xlat, bool *zero_it);

static inline void *xdp2_shm_map_existing(char *file, size_t size,
					  off_t offset)
{
	void *p;
	int fd;

	fd = open(file, O_RDWR);
	if (fd < 0)
		return NULL;

	p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
	if (!p) {
		close(fd);
		return NULL;
	}

	close(fd);

	return p;
}

#endif /* __XDP2_SHM_H__ */
