/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
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

#ifndef __XDP2_BPF_COMPAT_H__
#define __XDP2_BPF_COMPAT_H__

/*
 * BPF compatibility header for network byte order functions.
 *
 * This header provides htons/ntohs/htonl/ntohl functions that work in both
 * BPF and userspace contexts. BPF programs cannot use libc's arpa/inet.h,
 * so we use libbpf's bpf_endian.h and map the standard names to BPF versions.
 */

#ifdef __bpf__
/* BPF context: use libbpf's endian helpers */
#include <bpf/bpf_endian.h>

/* BPF: Include linux/in.h for IPPROTO_* constants and linux/stddef.h for offsetof */
#include <linux/in.h>
#include <linux/stddef.h>

/* Map standard network byte order functions to BPF versions */
#ifndef htons
#define htons(x)	bpf_htons(x)
#endif
#ifndef ntohs
#define ntohs(x)	bpf_ntohs(x)
#endif
#ifndef htonl
#define htonl(x)	bpf_htonl(x)
#endif
#ifndef ntohl
#define ntohl(x)	bpf_ntohl(x)
#endif

#elif defined(__KERNEL__)
/* Kernel context: byte order functions come from linux headers */

#else
/* Userspace context: use standard arpa/inet.h */
#include <arpa/inet.h>

#endif /* __bpf__ / __KERNEL__ / userspace */

#endif /* __XDP2_BPF_COMPAT_H__ */
