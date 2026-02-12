// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "xdp2/addr_xlat.h"
#include "xdp2/shm.h"
#include "xdp2/utility.h"

/* Open a shared memory file to be mmap'ed */
int xdp2_shm_open_file(const char *name, size_t size, off_t offset,
		       bool create, size_t *fsize)
{
	char c = 0;
	int fd;

	XDP2_ASSERT(!create || size, "SHM open file: create is set "
				     "adn size is zero");

	/* Create the shm file only for docker platform */
	fd = open(name, O_RDWR | O_SYNC);
	if (fd >= 0) {
		create = false;
	} else {
		if (create)
			fd = open(name, O_RDWR | O_CREAT, 0660);
		if (fd < 0) {
			XDP2_WARN("Open shm file: Unable to open %s: %s\n",
				  name, strerror(errno));
			return -1;
		}
	}

	if (create) {
		if (lseek(fd, offset + size - 1, SEEK_SET) < 0) {
			XDP2_WARN("Open shm file: lseek failed for "
				  "%s: %s\n", name, strerror(errno));
			return -1;
		}

		if (write(fd, &c, 1) < 0) {
			XDP2_WARN("Open shm file: write failed for "
				  "%s: %s\n", name, strerror(errno));
			return -1;
		}
	}

	/* Get the size of the file and return it. This can be used to
	 * check that we don't mmap beyond the end of the file
	 */
	offset = lseek(fd, 0L, SEEK_END);
	if (offset == -1UL)
		return -1;
	size = offset;
	if (!size) {
		struct stat st;

		/* Using lseek on the file didn't return the size (this seems
		 * to happen for device files). Try to get the size by doing
		 * a stat on the name instead
		 */
		if (fstat(fd, &st) < 0) {
			XDP2_WARN("Open shm file: fstat failed for "
				  "%s: %s\n", name, strerror(errno));
			return -1;
		}
		size = st.st_size;
	}

	*fsize = size;

	return fd;
}

/* mmap a shared memory file */
void *xdp2_shm_map(const char *file, size_t size, off_t offset, bool create,
		   bool zero_it)
{
	size_t fsize;
	void *p;
	int fd;

	if (!xdp2_offset_page_aligned(offset)) {
		XDP2_WARN("Mapping shared memory, offset is not page "
			  "aligned: %lu\n", offset);
		return NULL;
	}

	fd = xdp2_shm_open_file(file, size, offset, create, &fsize);
	if (fd < 0)
		return NULL;

	if (!size)
		size = fsize - offset;

	if (offset + size > fsize) {
		XDP2_WARN("Mapping shared memory, trying to mmap beyond "
			  "the end of the file: file size %lu mmap extent %lu",
			  fsize, offset + size);
		return NULL;
	}

	p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
	if (p == MAP_FAILED) {
		close(fd);
		XDP2_WARN("Mmap shared field %s failed with error: %s",
			  file, strerror(errno));
		return NULL;
	}

	if (zero_it)
		memset(p, 0, size);

	close(fd);

	return p;
}

/* Open an mmap the file for one instance of shared memory
 *   - If the size argument is non-zero and not -1UL then the shared memory
 *     exists with the specified size and needs to be initialized
 *   - If the size argument is -1UL then the shared memory file needs to be
 *     created
 *   - Else if the size argument is zero then the file is assumed to exist and
 *     the size is read from the shm header in the file
 */
void *xdp2_shm_map_make(const char *file, size_t size, off_t offset,
			bool set_xlat, size_t shm_size, size_t conf_size,
			char *shm_name, unsigned int shm_type,
			unsigned int shm_addr_xlat, bool *zero_it)
{
	bool create = false;
	void *shm;

	*zero_it = false;

	if (size) {
		if (conf_size) {
			if (size == -1UL) {
				/* Shared memory files needs to be created,
				 * size is derived from configuration
				 */
				size = conf_size;
				create = true;
			} else if (size < conf_size) {
				XDP2_WARN("Requested size for %s shared "
					  "memory is too small: %lu < %lu",
					  shm_name, size, conf_size);
				return NULL;
			} else if (size > conf_size) {
				XDP2_WARN("Requested size for %s shared "
					  "memory is greater than the "
					  "minimum needed: %lu > %lu",
					  shm_name, size, conf_size);
			}
		}
		*zero_it = true;
	} else {
		if (conf_size && conf_size > size) {
			XDP2_WARN("File size for %s shared memory is too "
				  "small compared to calculated size: "
				  "%lu < %lu", shm_name, size, conf_size);
			return NULL;
		}
	}

	shm = xdp2_shm_map(file, size, offset, create, zero_it);
	if (!shm) {
		XDP2_WARN("Mapping failed for %s shared memory: "
			  "file is %s, size is %lu, offset is %lu",
			  shm_name, file, size, offset);
		return NULL;
	}

	/* Set up address translation for the caller. xlat_func is true in a
	 * multi threaded case with a shared address space, false in the case
	 * of running in multiple independent processes each of which has its
	 * own address space
	 */
	if (set_xlat)
		xdp2_addr_xlat_set_base(shm_addr_xlat, shm);

	return shm;
}
